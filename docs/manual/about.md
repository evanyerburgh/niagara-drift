---
title: About
intro: Two drones that breathe and drift on their own, as open-source firmware for the M-VAVE FM-1
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

## What it does

### Sound

<ul class="cards">
<li><img class="ic" src="../../web/manual/icons/waveform.svg" alt=""><b>2 drones</b><span>Virtual-analog: each a wave from sine to square, a partner (a second oscillator) at an interval, a sub and noise, into its own resonant filter and drive</span></li>
<li><img class="ic" src="../../web/manual/icons/music-notes-simple.svg" alt=""><b>11 Tunings</b><span>12-tone to Partch's 43 notes, La Monte Young's, Harrison's, the shrutis; intervals named in each one's terms</span></li>
<li><img class="ic" src="../../web/manual/icons/intersect.svg" alt=""><b>Cross-mod</b><span>FM between the drones: each bending the other's pitch, or the finished sound bending both (Snarl); or drone I moving drone II's filter</span></li>
<li><img class="ic" src="../../web/manual/icons/arrows-clockwise.svg" alt=""><b>Feedback and Character</b><span>The finished sound fed back into the drones; the oscillators from clean to vintage</span></li>
<li><img class="ic" src="../../web/manual/icons/broadcast.svg" alt=""><b>6 delays, 5 reverbs</b><span>Clean to tape and self-warping echoes, a drive after them, reverbs from a room to a freeze</span></li>
</ul>

### Movement

<ul class="cards">
<li><img class="ic" src="../../web/manual/icons/wave-sine.svg" alt=""><b>Ultra LFO</b><span>2 waves of 10 shapes each, on each drone's pitch, wave, cutoff or resonance, and the delay</span></li>
<li><img class="ic" src="../../web/manual/icons/gauge.svg" alt=""><b>Movement</b><span>One knob for every LFO's depth at once, from still to double</span></li>
<li><img class="ic" src="../../web/manual/icons/pulse.svg" alt=""><b>Swell</b><span>A tremolo on both drones, in time: from a soft swell to a hard gate</span></li>
<li><img class="ic" src="../../web/manual/icons/shuffle.svg" alt=""><b>Moves on its own</b><span>Drift (slow random modulation), Wander (a random melody on a scale) and Age (tape wear)</span></li>
</ul>

### Playing

<ul class="cards">
<li><img class="ic" src="../../web/manual/icons/stack.svg" alt=""><b>16 Presets</b><span>Each a different side of the instrument, each moving over a minute</span></li>
<li><img class="ic" src="../../web/manual/icons/hand-pointing.svg" alt=""><b>8 gestures</b><span>Macros: several settings at once on top of your sound, each in its own order; held, pressed harder, or tapped to stay</span></li>
<li><img class="ic" src="../../web/manual/icons/floppy-disk.svg" alt=""><b>Save 48 sounds</b><span>Six banks of eight scenes, yours to fill; each glides in, or crossfades as you press its key harder</span></li>
<li><img class="ic" src="../../web/manual/icons/repeat.svg" alt=""><b>Motion loop</b><span>Records your knob moves, gestures and scenes, and plays them back</span></li>
<li><img class="ic" src="../../web/manual/icons/path.svg" alt=""><b>Never jumps</b><span>Everything glides (morphs): buttons, keys and scenes move to new settings over times you set</span></li>
</ul>

### Connect

<ul class="cards">
<li><img class="ic" src="../../web/manual/icons/metronome.svg" alt=""><b>MIDI</b><span>A clock over USB or TRS puts the Swell, the LFO, the delay, Wander and the scenes in time; notes tune the drones</span></li>
<li><img class="ic" src="../../web/manual/icons/ear.svg" alt=""><b>Hear the screen</b><span>With the FM-1 Screen Reader, it says what its screen shows, for players who can't see it</span></li>
<li><img class="ic" src="../../web/manual/icons/browser.svg" alt=""><b>Play it in a browser</b><span>The emulator on the front page is the firmware itself, with sound; no FM-1 needed</span></li>
<li><img class="ic" src="../../web/manual/icons/download-simple.svg" alt=""><b>Install from a browser</b><span>Install, update, back up and go back to M-VAVE's firmware from Chrome or Edge</span></li>
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
| **Oscillators** | virtual analog, 4 shapes blended continuously (sine, triangle, saw, square) with pulse width; saw and pulse band-limited (polyBLEP); hard sync; cross-FM |
| **Partner** | a second oscillator on each drone: an octave or a fifth or fourth below, unison, a third, fourth or fifth above, an octave, or an octave and a fifth; each tempered or just (pure, beatless); detune up to 30 cents for slow beating |
| **Sub and noise** | a sub of 4 waves (square, sine, triangle, saw), 1–4 octaves down or the subharmonics ⅓ and ⅕; under a low drone the lowest become ticks and pulses ([recipe](recipes/sounds.md#ticks)); noise from white to pink |
| **Filter** | a state-variable filter (zero-delay feedback, after Andrew Simper): low-, band- or high-pass, resonant to self-oscillation; an antialiased drive |
| **LFO** | 2 waves of 10 shapes each (20 in all: triangles, squares, saws, exponentials, patterns, random, AND and OR steps), one cycle in 100 s to 70 Hz, on pitch, wave, cutoff, resonance and the delay |
| **Tuning** | A from 427 to 453 Hz (440 to start, 432 for those who like it), 1 Hz a click or 0.1 with SEL; 12-tone, quarter tones, 19-, 31- and 53-tone, just, the harmonics, LMY, Harrison, the shrutis, Partch |
| **Detuning** | each drone a cent at a time off its Tuning's notes (SEL + a pitch knob), kept as you move between notes; each partner up to 30 cents, for beating; Drift lets the detune wander |
| **Polyphonic aftertouch** | each white key's own pressure, read as it's held; scenes crossfade and keys 9–12's gestures follow it; the touch is a setting (Setup 2); used inside, not sent out over MIDI |
| **MIDI** | in, over USB and TRS: notes on channels 1 and 2 tune the drones; the clock (24 pulses a beat) and Start, with Sync on or off ([MIDI](reference/midi.md)) |
| **Storage** | 6 banks of 8 scenes and 16 Presets in the FM-1's flash; backups to a file before every install; a restore that a power cut can't break ([Help](help.md)) |
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
| [fm1-base](https://github.com/charlesvestal/fm1-base) | Charles Vestal | the safe restore's flash core (MPL-2.0) |
| [FM-1 Screen Reader](https://github.com/charlesvestal/fm1-reader) | Charles Vestal | the screen's words over USB and the page that speaks them (its firmware side from fm1-base, MPL-2.0); thanks to João Durães (WoodJack) for asking for it |
| [Groove OS](https://www.groove-os.com/) | | the idea of running FM-1 firmware in a browser |

Made by Evan Yerburgh; not made or endorsed by M-VAVE, Hügelton Instruments or the makers of any instrument it began
from. Licenses in [LICENSING.md](../../LICENSING.md). The site is set in Inter Tight, IBM Plex Mono and Barlow Semi
Condensed (SIL OFL).
