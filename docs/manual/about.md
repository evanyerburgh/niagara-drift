---
title: About
intro: Two drones that breathe and drift on their own, as open-source firmware for the M-VAVE FM-1
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## What it does

### Sound

<ul class="cards">
<li><b>2 drones</b><span>Virtual-analog: each a wave from sine to square, a partner (a second oscillator) at an interval, a sub and noise, into its own resonant filter and drive</span></li>
<li><b>Tunings</b><span>12-tone to Partch's 43 notes, La Monte Young's, Harrison's, the shrutis; intervals named in each one's terms</span></li>
<li><b>Cross-mod</b><span>FM between the drones: each bending the other's pitch, or the finished sound bending both (Snarl); or drone I moving drone II's filter</span></li>
<li><b>Feedback and Character</b><span>The finished sound fed back into the drones; the oscillators from clean to vintage</span></li>
<li><b>Effects</b><span>Six delays, a drive after them, five reverbs from a room to a freeze</span></li>
</ul>

### Movement

<ul class="cards">
<li><b>Ultra LFO</b><span>Two waves of ten shapes on each drone's pitch, wave, cutoff or resonance, and the delay</span></li>
<li><b>Movement</b><span>One knob for every LFO's depth at once, from still to double</span></li>
<li><b>Swell</b><span>A tremolo on both drones, in time: from a soft swell to a hard gate</span></li>
<li><b>Moves on its own</b><span>Drift (slow random modulation), Wander (a random melody on a scale) and Age (tape wear)</span></li>
</ul>

### Playing

<ul class="cards">
<li><b>16 Presets</b><span>Each a different side of the instrument, each moving over a minute</span></li>
<li><b>8 gestures</b><span>Macros: several settings at once on top of your sound, each in its own order; held, pressed harder, or tapped to stay</span></li>
<li><b>Save 48 sounds</b><span>Six banks of eight scenes, yours to fill; each glides in, or crossfades as you press its key harder</span></li>
<li><b>Motion loop</b><span>Records your knob moves, gestures and scenes, and plays them back</span></li>
<li><b>Never jumps</b><span>Everything glides (morphs): buttons, keys and scenes move to new settings over times you set</span></li>
</ul>

## Videos

<div class="videos">
<figure class="video"><iframe src="https://www.youtube-nocookie.com/embed/6mRatAf7f-k" title="2 x FM-1 AMBIENT TECHNO LIVE PERFORMANCE | Niagara Drift and Groove OS, by Grzegorz Bojanek" loading="lazy" referrerpolicy="strict-origin-when-cross-origin" allow="encrypted-media; picture-in-picture; fullscreen"></iframe>
<figcaption><a href="https://www.youtube.com/watch?v=6mRatAf7f-k">2 x FM-1 AMBIENT TECHNO LIVE PERFORMANCE | Niagara Drift and Groove OS</a> · Grzegorz Bojanek</figcaption></figure>
<figure class="video"><iframe src="https://www.youtube-nocookie.com/embed/bVre9eF2Rm4" title="無数の窓口 - collective unconscious - [soundtrack][5.1ch専用][FM-1 Niagara], by Mirrorofes" loading="lazy" referrerpolicy="strict-origin-when-cross-origin" allow="encrypted-media; picture-in-picture; fullscreen"></iframe>
<figcaption><a href="https://www.youtube.com/watch?v=bVre9eF2Rm4">無数の窓口 - collective unconscious - [soundtrack][5.1ch専用][FM-1 Niagara]</a> · Mirrorofes</figcaption></figure>
<figure class="video"><iframe src="https://www.youtube-nocookie.com/embed/ZzUiysz0RY0" title="M-VAVE FM-1 カスタムファームウェアにびっくり「Niagara Drift」の世界！, by 電気老人" loading="lazy" referrerpolicy="strict-origin-when-cross-origin" allow="encrypted-media; picture-in-picture; fullscreen"></iframe>
<figcaption><a href="https://www.youtube.com/watch?v=ZzUiysz0RY0">M-VAVE FM-1 カスタムファームウェアにびっくり「Niagara Drift」の世界！</a> · 電気老人</figcaption></figure>
</div>

## Technical details

| | |
|---|---|
| **Audio** | 44.1 kHz stereo; 32-bit floating point inside, 24-bit out; a block every 5.8 ms |
| **Oscillators** | virtual analog: saw and pulse band-limited (polyBLEP), sine, triangle, blended continuously; hard sync; cross-FM |
| **Filter** | a state-variable filter (zero-delay feedback, after Andrew Simper): low-, band- or high-pass, resonant to self-oscillation; an antialiased drive |
| **Keys** | the white keys sense pressure (scene crossfades, keys 9–12's gestures) |
| **MIDI** | over USB or TRS: notes on channels 1 and 2 tune the drones; a clock syncs the Swell, LFO, delay, Wander and scenes ([MIDI](reference/midi.md)) |
| **Your sounds** | six banks of eight scenes, plus 16 Presets; backed up before every install, restored safely ([Help](help.md)) |
| **The screen, out loud** | the [FM-1 Screen Reader](reference/screen-reader.md) says what the screen shows |
| **In the browser** | the emulator is the firmware's own code, compiled for the browser, at 44.1 kHz; SAVE keeps your sound in that browser |
| **License** | GPL-3.0, source on [GitHub](https://github.com/evanyerburgh/niagara-drift) |

## Where it came from

Named for *Day of Niagara* (the Theatre of Eternal Music, 1965) and La Monte Young's *Drift Studies*. It began from ideas
in the Stylophone CPM DS-2, the Moog Subharmonicon and the Soma Lyra-8, and has grown into an instrument of its own.

| From | By | What it gave |
|---|---|---|
| [FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x) | Charles Vestal | the code it started from, the host simulator, the browser emulator |
| [Felucca](https://github.com/hugelton/Felucca) | Leo Kuroshita, Hügelton Instruments | the FM-1 platform: hardware layer, USB, update loader, storage, installer |
| [MONK-1](https://github.com/charlesvestal/fm1-monk) | Charles Vestal | reading the keys' pressure |
| [ChoralRoot](https://github.com/Quixotic7/ChoralRootFM1) | Quixotic7 | the delay and reverb's design; the scene crossfade and backup-before-install ideas |
| [fm1-base](https://github.com/charlesvestal/fm1-base) | Charles Vestal | the safe restore's flash core and the screen reader (MPL-2.0) |
| [Groove OS](https://www.groove-os.com/) | | the idea of running FM-1 firmware in a browser |

Made by Evan Yerburgh; not made or endorsed by M-VAVE, Hügelton Instruments or the makers of any instrument it began
from. Licenses in [LICENSING.md](../../LICENSING.md). The site is set in Inter Tight, IBM Plex Mono and Barlow Semi
Condensed (SIL OFL).
