---
title: LFO sendsとMods
intro: LFO：LFOの行き先と、ほかに動かすもの
---

<!-- SPDX-License-Identifier: GPL-3.0-only -->

<!-- github -->
AIで翻訳したページです。英語版：[LFO sends and Mods](../../reference/lfo-pages.md)
<!-- /github -->

## LFO sendsとMods {#lfo-sends-and-mods}

LFOの行き先と、ほかに動かすものです。

## ノブ {#knobs}

[[LFO]]でLFO sendsが開き、もう一度[[LFO]]でModsが開きます。LFOのレートと波形は左のノブにあります。[LFO](lfo.md)を見てください。

<img src="../../../img/knobs-lfo.png" width="520" alt="LFO sends（各ドローンのピッチとカットオフ）とMods（スピード・モジュレーション、ソース、ディレイ・モジュレーション、クロス・モジュレーション）のKNOB 1から4">

## LFO sends {#lfo-sends}

<img class="shot" src="../../../img/screen-lfo.png" width="240" alt="LFO sendsページ：2つのLFOと、キー付きの文で表した各ドローンのセンド">

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|
| I pitch（またはX-Wave） | I cutoff（またはresonance） | II pitch（またはX-Wave） | II cutoff（またはresonance） |

| ノブ | 働き |
|---|---|
| **ピッチ・センド** | LFOがドローンのピッチを動かす幅。代わりにX-Waveを動かすこともできます（LFOページで[[G#3]]、[[G#4]]） |
| **カットオフ・センド** | LFOがドローンのカットオフを動かす幅。代わりにレゾナンスを動かすこともできます（LFOページで[[C#4]]、[[C#5]]） |

### センドのしくみ {#how-a-send-works}

それぞれのセンドは、次の順につながっています：どのLFOを、どれだけ、Movementでスケールして、何を動かすか。

<img class="diagram" src="../../media/lfo-chain-ja.svg" alt="1つのセンド、ドローン Iのピッチ：LFO I、LFO II、または両方をF#3で選び、量をKNOB 1で決め（真ん中はゼロ）、Movement（ALGORITHM）でスケールし、G#3で動かすものを選ぶ：ドローン Iのピッチ（キー・オフ）かX-Wave（キー・オン）。ドローン Iのカットオフも同じで、A#3、KNOB 2、C#4（カットオフかレゾナンス）。ドローン IIの2つは自分のキーとKNOB 3、4を使う。">

ドローン Iのピッチ・センドです。カットオフ・センドも同じで、[[A#3]]、KNOB 2、[[C#4]]（カットオフかレゾナンス）を使います。
ドローン IIの2つは、自分のキーとKNOB 3、4を使います。

### どのLFOで、何を動かすか {#which-lfo-and-what-it-moves}

LFOページでは、各ドローンの4つの黒鍵で、センドごとのLFOと、動かすものを選びます。

<img src="../../../img/black-keys-sends.png" width="560" alt="LFOページの黒鍵で、各ドローンの4つのセンド・キーを示したもの：ドローン IはF#3からC#4、ドローン IIはF#4からC#5">

## Mods {#mods}

<img class="shot" src="../../../img/screen-lfo2.png" width="240" alt="Modsページ：LFO IとIIに送られるLFOのクロック、ディレイの揺れ、クロス・モジュレーションのモード">

| KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|
| Speed mod | Source (LFO) | Delay mod | X-mod |

| ノブ | 働き |
|---|---|
| **Speed mod**（画面ではSpeed） | LFO自身の波の1つで、LFOの速さが上下それぞれ最大3オクターブ変わります |
| **Source** | どのLFOがスピードを動かすか：I、II、または両方 |
| **Delay mod**（画面ではDelay） | LFOがディレイのタイムを揺らす幅、最大±25% |
| **X-mod** | クロス・モジュレーションの深さ。モードは[[F#5]]で選びます。cutoff（ドローン Iがドローン IIのカットオフを動かす）、pitch（ドローンが互いのピッチを曲げる、クロスFM）、snarl |

### Modsのしくみ {#how-the-mods-work}

<img class="diagram" src="../../media/lfo-mods-ja.svg" alt="Modsの3つの部分。スピード・モジュレーション：LFO I、II、または両方がクロック自体を速くしたり遅くしたりする。KNOB 1で幅（最大3オクターブ）、KNOB 2でどのLFOか。ディレイ・モジュレーション：D#5で選んだLFO I、II、または両方がディレイのタイムを揺らす。KNOB 3で幅（最大25%）、Movementでスケールされ、ディレイのミックスがあるときに聞こえる。クロス・モジュレーションはLFOではない：ドローンが互いを動かす。KNOB 4で深さ、F#5でモードを選ぶ：ドローン Iがドローン IIのカットオフを動かす、互いを曲げる（クロスFM）、またはSnarl（音が両方を曲げる）。">

スピード・モジュレーションとディレイ・モジュレーションはLFOと一緒に動きます。クロス・モジュレーションは違います：
ドローンが互いを動かすものです。

## LFOの行き先 {#where-the-lfo-goes}

<img class="diagram" src="../../media/lfo-map-ja.svg" alt="LFOの全体：クロック（SELECTでそのレート）がLFO I（OCT−で波形を選ぶ）とLFO II（OCT+）を動かす。両方とも、Movement（ALGORITHM）でスケールされ、5つの場所に届く。それぞれのキーでLFO I、II、または両方を選ぶ：ドローン Iのピッチ／X-Wave（F#3、G#3、KNOB 1）、ドローン Iのカットオフ／レゾナンス（A#3、C#4、KNOB 2）、ドローン IIのピッチ／X-Wave（F#4、G#4、KNOB 3）、ドローン IIのカットオフ／レゾナンス（A#4、C#5、KNOB 4）、ディレイのタイム（D#5、ModsのKNOB 3）。スピード・モジュレーション（ModsのKNOB 1と2）は、波をクロックに戻して速くしたり遅くしたりする。">

両方のページで、LFOが届くすべての場所です。それぞれのキーで、その場所に送るLFO I、II、または両方を選びます。
スピード・モジュレーションでは、波がクロック自体を速くしたり遅くしたりします。

## 補足 {#good-to-know}

### センド {#sends}

- **LFOページの黒鍵すべて：**[黒鍵](black-keys.md#lfo-keys)を見てください。
- **各センドの値の下に、**実際に聞こえる振れ幅が、Movementを含めて表示されます（「±12c」「±3.6 oct」）。
- **X-Waveとレゾナンスでは、**100で全範囲をスイープし、端で折り返すので動き続けます。
- **Movement**（ALGORITHM）は、センドとディレイ・モジュレーションをスケールします。スピード・モジュレーションと
  クロス・モジュレーションはスケールしません。
- **レートの位置に「mvmt 0%」：**Movementが0なので、何も動きません。

### Mods {#mods}

- **ディレイ・モジュレーション**は、ディレイのミックスがあるときだけ聞こえます。その波はLFOページの[[D#5]]で選びます。
- **クロス・モジュレーションのモード**（画面の名前で）：*cutoff*（ドローン Iの音がドローン IIのフィルターを動かす）、
  *pitch*（それぞれのドローンが互いのピッチを曲げる。荒い音から金属的な音まで）、*snarl*（最終的な音が両方を曲げる：グロウル）。
- **クロス・モジュレーション**は、設定したピッチのまわりで曲げます。ピッチそのものは動きません。

### 画面の絵 {#pictures}

- LFO sendsでは、各ドローンのセンドがキー付きの文で表示され（「[F#3] LFO II → [G#3] x-wave」）、その上に2つのLFO。
  Modsでは、LFOのクロックがLFO IとIIに送られる様子と、クロス・モジュレーションのモードとそのキー。
