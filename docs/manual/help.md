---
title: Help
intro: When something doesn't work
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

Pick what's happening.

<ul class="doors">
<li><a href="#installing-and-updating"><img class="ic" src="../../web/manual/icons/download-simple.svg" alt=""><small>Getting it</small><b>Installing and updating</b><span>The web installer, a cable, keeping your sounds</span></a></li>
<li><a href="#an-install-doesn-t-take"><img class="ic" src="../../web/manual/icons/lifebuoy.svg" alt=""><small>Stuck</small><b>An install doesn't take</b><span>Nothing changes, an error, an FM-1 that won't start</span></a></li>
<li><a href="#sound-and-playing"><img class="ic" src="../../web/manual/icons/speaker-high.svg" alt=""><small>While playing</small><b>Something sounds wrong</b><span>A drone went quiet, notes jump, a value is gray, no bass</span></a></li>
<li><a href="#backups"><img class="ic" src="../../web/manual/icons/floppy-disk.svg" alt=""><small>Your sounds</small><b>Backups</b><span>Saving and restoring your sounds and scenes</span></a></li>
<li><a href="#going-back-or-to-another-firmware"><img class="ic" src="../../web/manual/icons/arrows-left-right.svg" alt=""><small>Leaving</small><b>Going back</b><span>To M-VAVE's firmware, or to another one</span></a></li>
<li><a href="#send-feedback"><img class="ic" src="../../web/manual/icons/flag.svg" alt=""><small>Ideas, opinions, problems</small><b>Send feedback</b><span>A feature you'd like, what you think, or what went wrong</span></a></li>
</ul>

## Installing and updating

| To | Do |
|---|---|
| **Install** | Open the [web installer](https://evanyerburgh.github.io/niagara-drift/install/) in Chrome or Edge, with a USB data cable straight into the computer |
| **Update** | Run the installer again. Your sounds and scenes are kept, unless that version's notes in the [changelog](../../CHANGELOG.md) say otherwise |

Installing firmware is at your own risk.

## An install doesn't take

Nothing is written until the FM-1 has checked the whole package, so trying again is safe. Work down the steps:

1. Use a USB data cable straight into the computer, close other apps that use MIDI (a DAW, M-UPGRADE), replug, and
   try again.
2. Going back from 0.2 or another firmware, and nothing was written: choose the other M-VAVE file (V16 or V15). Each
   takes back only the version whose bootloader the FM-1 has; from 0.2.1 on, Niagara Drift takes either.
3. Still stuck: hold [[OCT−]] and [[OCT+]] for 5 seconds. The FM-1 goes into its chip's own update mode, which no
   firmware can get in the way of, and [M-Flasher](https://github.com/jvitkauskas/m-flasher) (Windows, Mac, Linux)
   puts M-VAVE's firmware back. If an FM-1 won't start at all, see [below](#an-fm-1-that-won-t-start).

> **Newer FM-1s on 0.2.** Up to 0.2, Niagara Drift didn't recognize the memory chip of some newer FM-1s; there it
> couldn't save and ignored installs. On such an FM-1 running 0.2, use step 3 to leave it. Since 0.2.1 it takes any
> standard chip: the newer UC25HQ80 (ID B36014) and the first Puya P25Q80 (856014).

## An FM-1 that won't start

The FM-1's chip has its own update mode that no firmware can overwrite, so an FM-1 that won't start can still be put
back on M-VAVE's firmware from a computer, with a USB data cable straight in.

1. Switch it off and on. After two failed starts in a row it comes up in safe mode (no sound, USB on): open the
   [install page](https://evanyerburgh.github.io/niagara-drift/install/) and install again, or put M-VAVE's firmware back.
2. Nothing on the screen at all: switch it off and on a few more times. After four failed starts it opens the chip's
   update mode by itself.
3. Download [M-Flasher](https://github.com/jvitkauskas/m-flasher/releases/latest), a free app for Mac, Windows and
   Linux. It finds the FM-1 in its update mode: choose M-VAVE's FM-1 file and press REVIEW & FLASH. It saves the whole
   FM-1 to a file before it writes anything. The install page's "Trouble installing?" card shows each step.
4. Switch the FM-1 off and on: it starts with M-VAVE's firmware, and the install page works again.

> **Good to know.** A terminal on a Mac or Linux can do step 3 with `tools/fm1_rescue.sh`, a script that checks the
> chip and backs it up first ([BUILDING.md](../../BUILDING.md#install)).

## Sound and playing

| What's happening | What to do |
|---|---|
| **You hear nothing** | Press [[PLAY]]: the drones fade in (stopped, PLAY breathes slowly). Then turn up MASTER, and each drone's level on Drones (KNOB 1 and KNOB 3) |
| **Drone II went quiet,** [the header](reference/controls.md#screen-header) says II OUT | [[SEL]] + [[PLAY]] brings it back |
| **You want the sound before a scene** | [[SEL]] + [[SAVE]] goes back; again goes forward |
| **Drone II keeps jumping to other notes** | That's Wander: turn it to 0 on Pitch moves ([[SEQ]] until you're on it, KNOB 3) |
| **A knob's value is gray** | It can't be heard as things are; the screen says why ("needs sqr", "needs [[F#3]]", Movement at 0%) |
| **The low end is missing on the speaker** | It can't play the lowest notes and clicks: listen on headphones |
| **A preset doesn't play at its own pitch** | Pitch lock ([[C#4]]) keeps the pitch you're on: turn it off |

## Backups

| When | What happens |
|---|---|
| **Before every install** over Niagara Drift | Both installers save your sounds, scenes and settings to a file: the web installer into your downloads (and it offers to put them back), the terminal one into `~/niagara-drift-backups/` |
| **Any time** | The web installer's "Your sounds and scenes" card saves or restores one, as `python3 tools/fm1_install.py --backup` and `--restore FILE` do; each reads the other's files |
| **Trying another firmware** | Back up first, restore when you come back |
| **Onto another FM-1** | A backup leaves out the knobs' calibration, which belongs to each FM-1, so it can go onto another |

### Restoring is safe

<img class="shot" src="../media/r-help-0.png" width="240" alt="The FM-1 asking RESTORE FROM USB? in a banner over the Drones page">

1. The whole backup goes onto the FM-1 and is checked first, changing nothing. A damaged file is refused before
   anything is written.
2. The FM-1 asks **RESTORE FROM USB?**: [[SEL]] puts it back; [[HOME]] keeps what it has, as does a minute without
   either.
3. Your sounds and scenes change all together or not at all, even if the power goes off halfway: the next power-on
   finishes it.

## Going back, or to another firmware

| To | Do |
|---|---|
| **M-VAVE's firmware** | The installer's "Back to M-VAVE's firmware" writes V16 or V15 back over the same cable; or use M-VAVE's updater, M-UPGRADE, from [m-vave.com/download](https://www.m-vave.com/download) |
| **Another firmware** | Most expect M-VAVE's: go back first, then install them |

> **Safe mode.** If Niagara Drift fails to start twice in a row, it comes up with no sound and USB on, and the
> installer still works.

## Send feedback

Ideas, requests, what you like or don't, or something that went wrong: tell us here. No account needed. With a GitHub
account you can also [open an issue](https://github.com/evanyerburgh/niagara-drift/issues).

<form class="feedback" action="https://formspree.io/f/xppdwkel" method="POST">
<label>Your message<textarea name="Message" rows="5" required placeholder="An idea, what you think, or what went wrong (for a problem, the version on the startup screen helps)"></textarea></label>
<label>Your email <small>optional, only if you'd like an answer</small><input type="email" name="email" autocomplete="email"></label>
<input type="text" name="_gotcha" class="gotcha" tabindex="-1" autocomplete="off" aria-hidden="true">
<p class="sent-note">Sent with it: this page's address and your browser's name.</p>
<button type="submit">Send</button>
<p class="feedback-status" role="status"></p>
</form>
