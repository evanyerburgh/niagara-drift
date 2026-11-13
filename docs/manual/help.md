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
<li><a href="#report-a-problem"><img class="ic" src="../../web/manual/icons/flag.svg" alt=""><small>Still not right</small><b>Report a problem</b><span>Tell us what happened</span></a></li>
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
   puts M-VAVE's firmware back. If an FM-1 won't start at all, see [BUILDING.md](../../BUILDING.md#install).

> **Newer FM-1s on 0.2.** Up to 0.2, Niagara Drift didn't recognize the memory chip of some newer FM-1s; there it
> couldn't save and ignored installs. On such an FM-1 running 0.2, use step 3 to leave it. Since 0.2.1 it takes any
> standard chip: the newer UC25HQ80 (ID B36014) and the first Puya P25Q80 (856014).

## Sound and playing

| What's happening | What to do |
|---|---|
| **Drone II went quiet,** [the header](reference/controls.md#screen-header) says II OUT | [[SEL]] + [[PLAY]] brings it back |
| **You want the sound before a scene** | [[SEL]] + [[SAVE]] goes back; again goes forward |
| **Drone II keeps jumping to other notes** | That's Wander: turn it to 0 on Pitch moves ([[SEQ]] twice, KNOB 3) |
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

## Report a problem

It's tested on one FM-1 so far. If something goes wrong, sounds wrong or could be better, tell us here. No account
needed. With a GitHub account you can also [open an issue](https://github.com/evanyerburgh/niagara-drift/issues).

<form class="feedback" action="https://formspree.io/f/xppdwkel" method="POST">
<label>What happened<textarea name="What happened" rows="5" required placeholder="What you did, what you heard or saw, what you expected"></textarea></label>
<fieldset><legend>Which FM-1</legend><label><input type="radio" name="FM-1" value="older"> Older</label><label><input type="radio" name="FM-1" value="newer (M-VAVE under the screen)"> Newer, with M-VAVE under the screen</label><label><input type="radio" name="FM-1" value="not sure" checked> Not sure</label></fieldset>
<label>Version <small>on the startup screen, if you know it</small><input name="Version" placeholder="e.g. 0.3"></label>
<label>Your email <small>optional, only if you'd like an answer</small><input type="email" name="email" autocomplete="email"></label>
<input type="text" name="_gotcha" class="gotcha" tabindex="-1" autocomplete="off" aria-hidden="true">
<p class="sent-note">Sent with it: this page's address and your browser's name, to help find the problem.</p>
<button type="submit">Send</button>
<p class="feedback-status" role="status"></p>
</form>
