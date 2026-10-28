#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""What CI found on a PR's commit, for the PR review to read (claude-code-review.yml).

    tools/ci_results.py REPO PR SHA       (GH_TOKEN set; needs the gh CLI)

Waits for every run of the Tests workflow on SHA to finish (the PR's opening and its label each start one), then
prints, as markdown: the PR's labels, each job's latest result (a job a later run skipped keeps its earlier result:
host and cost don't run again for a label), and the same-samples check's own summary (what differs, file by file).
Waits at most 25 minutes; whatever hasn't finished by then is reported as still running.
"""
import json
import subprocess
import sys
import time

WAIT_S, POLL_S, SETTLE_S = 25 * 60, 30, 60


def gh(*args):
    return subprocess.run(["gh", *args], capture_output=True, text=True, check=True).stdout


def runs(repo, sha):
    out = gh("run", "list", "--repo", repo, "--workflow", "Tests", "--commit", sha, "--limit", "20",
             "--json", "databaseId,status,conclusion,event,createdAt")
    return sorted(json.loads(out), key=lambda r: r["createdAt"])


def same_samples_text(repo, job_id):
    """The check's summary from its log: the '### Same samples' line and the table under it."""
    log = gh("api", f"repos/{repo}/actions/jobs/{job_id}/logs")
    lines, keep = [], False
    for raw in log.splitlines():
        text = raw.split(" ", 1)[1] if raw[:4].isdigit() and " " in raw else raw   # drop the timestamp
        if text.startswith("### Same samples"):
            keep = True
        if keep:
            if text.startswith("(both versions kept") or text.startswith("::error") or text.startswith("##["):
                break
            lines.append(text)
    return "\n".join(lines).strip()


def main():
    repo, pr, sha = sys.argv[1:4]
    time.sleep(SETTLE_S)                              # a label added just after opening starts its own run
    t0 = time.time()
    rs = runs(repo, sha)
    while (not rs or any(r["status"] != "completed" for r in rs)) and time.time() - t0 < WAIT_S:
        time.sleep(POLL_S)
        rs = runs(repo, sha)
    labels = [l["name"] for l in json.loads(gh("pr", "view", pr, "--repo", repo, "--json", "labels"))["labels"]]
    out = [f"Labels on the PR: {', '.join(labels) if labels else 'none'}", ""]
    if not rs:
        out.append("No run of the Tests workflow was found for this commit.")
        print("\n".join(out))
        return
    jobs = {}                                         # job name -> (its run, the job), the latest that ran
    for r in rs:
        js = json.loads(gh("run", "view", str(r["databaseId"]), "--repo", repo, "--json", "jobs"))["jobs"]
        for j in js:
            if j.get("conclusion") != "skipped" or j["name"] not in jobs:
                jobs[j["name"]] = (r, j)
    out += ["| Job | Result |", "|---|---|"]
    for name, (r, j) in sorted(jobs.items()):
        res = j.get("conclusion") or j.get("status") or "?"
        out.append(f"| {name} | {res if j.get('status') == 'completed' else 'still running'} |")
    if "same-samples" in jobs and jobs["same-samples"][1].get("status") == "completed":
        try:
            text = same_samples_text(repo, jobs["same-samples"][1]["databaseId"])
        except subprocess.CalledProcessError:
            text = ""
        out += ["", "Same samples, as it reported:", "", text or "(its summary couldn't be read from the log)"]
    print("\n".join(out))


if __name__ == "__main__":
    main()
