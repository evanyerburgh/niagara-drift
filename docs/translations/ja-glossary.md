<!-- SPDX-License-Identifier: GPL-3.0-only -->

# Japanese glossary (用語集)

How the manual's words go into Japanese, so every page says a thing the same way. Made 2026-11-17 from Roland's and
Korg's Japanese manuals (SH-4d, SYSTEM-8, S-1; minilogue xd): the words Japanese synth players already read. Not yet
checked by a native speaker: corrections welcome.

## Rules

1. **What the FM-1 shows stays as it shows it.** The screen isn't translated, so page names, knob names, values and
   messages stay in English letters, exactly as on screen: Drones, Pitch moves, Swell, Wander, Scene time, Tuning,
   "II OUT", "RESTORE FROM USB?". Japanese manuals do the same with a panel's printed names.
2. **Buttons and keys stay in their boxes, in English**: [PLAY], [SEL], [F#3], KNOB 1. Roland writes panel names in
   ［ ］ the same way.
3. **Ordinary synth words in katakana, with ・ between the words of one term** (Roland's and Korg's style):
   サブ・オシレーター, ピッチ・ロック, テープ・エコー. A long vowel at the end is written: フィルター, パラメーター,
   オシレーター, ディレイ.
4. **Polite, plain manual style (です・ます)**, short sentences. Steps as numbered lists, as in English.
5. **Punctuation:** 。and 、 in Japanese sentences; numbers, units and ranges half-width (1.45 s → 1.45秒, 0–100,
   KNOB 1–4). No spaces between Japanese words; a space is fine around English words for reading.
6. **Names stay as they are**: Niagara Drift, FM-1, M-VAVE, M-Flasher, M-UPGRADE, Chrome, Edge, GitHub. The Presets'
   names (NIAGARA, SONAR, ...) and the Tunings' (La Monte Young, Partch) stay in English letters.
7. **Drone I / drone II**: ドローン I / ドローン II (Roman numerals, as on screen).
8. **Every heading keeps the English page's link name**: `## ノブ {#knobs}`, `### クリックとチック {#ticks}`, so a link
   to a section (gestures.md#keep) lands on it in both languages. The drawings (docs/manual/media/*-ja.svg,
   `tools/make_diagrams.py ja`) follow rule 1: the FM-1's names stay, the words around them are translated.
9. **A step that presses a button until a page shows** reads 「**Pitch**ページになるまで[[HOME]]を押し」: most buttons
   open the page last used there, so "press once" would land on the wrong page. [[ARP]] opens the Swell page at once.
10. **Style:** no あなた (お使いの, 自分の, or nothing); "：" for labels and lists, not to join two clauses (use 。 or 、);
    counters with 個 or 種類 (11個の黒鍵, 16種類のサウンド); seconds in prose as 秒 (0.1秒), "s" only when quoting
    the screen (GESTURE TIME 33 s); ms stays ms.

## Terms

| English | 日本語 | Note |
|---|---|---|
| drone | ドローン | |
| drone synth | ドローン・シンセ | |
| firmware | ファームウェア | |
| install / installer | インストール / インストーラー | |
| web installer | Webインストーラー | |
| update (n. / v.) | アップデート / アップデートする | |
| backup / back up | バックアップ / バックアップする | |
| restore | 復元 / 復元する | Roland: 復元 for backup restore |
| sounds and scenes (yours) | サウンドとシーン | |
| save (a sound) | 保存する | SAVE the button stays [SAVE] |
| safe mode | セーフ・モード | |
| update mode (the chip's) | アップデート・モード | |
| bootloader | ブートローダー | |
| memory chip | メモリー・チップ | |
| USB data cable | USBデータ・ケーブル | "not a charge-only cable": 充電専用ではない |
| computer | パソコン | Japanese manuals' usual word |
| browser | ブラウザー | |
| download / downloads folder | ダウンロード / ダウンロード・フォルダー | |
| file | ファイル | |
| feedback (to us) | フィードバック、ご意見 | the form: ご意見・不具合の報告 |
| issue (GitHub) | Issue | GitHub's own word |
| changelog | 変更履歴 | |
| version | バージョン | |
| at your own risk | 自己責任で | |
| emulator / play it here | エミュレーター / ここで弾く | |
| computer keys | パソコンのキー | |
| preset | プリセット | |
| scene | シーン | |
| bank | バンク | |
| gesture | ジェスチャー | each gesture's name stays English (Weight, Wash) |
| hold / tap / press harder | 長押し / タップ / 強く押す | |
| knob | ノブ | Korg: ノブ; Roland: つまみ. ノブ matches "KNOB" on the panel |
| button | ボタン | |
| key (white / black) | 白鍵 / 黒鍵 | |
| page (on the screen) | ページ | |
| screen | 画面 | |
| header (the screen's top line) | ヘッダー | |
| message / banner | メッセージ | |
| value | 値 | |
| level | レベル | |
| pitch | ピッチ | |
| note (a pitch) | 音、ノート | 音 in prose; ノート for MIDI |
| melody | メロディー | |
| interval | 音程 | |
| tuning (a scale system) | チューニング | the screen's Tuning stays English as a name |
| just (intonation) | 純正律 | |
| equal temperament | 平均律 | |
| microtonal | マイクロトーナル | as Japanese synth makers write it (微分音 is the theory term) |
| octave | オクターブ | |
| cent | セント | |
| oscillator | オシレーター | |
| waveform / wave | 波形 | |
| sine / triangle / saw / square | サイン波 / 三角波 / ノコギリ波 / 矩形波 | |
| partner (oscillator) | パートナー・オシレーター | screen name Partner stays |
| detune | デチューン | Roland and Korg both write デチューン |
| sub oscillator / subharmonic | サブ・オシレーター / サブハーモニック | |
| noise (white / pink) | ノイズ（ホワイト / ピンク） | |
| filter (low-, band-, high-pass) | フィルター（ローパス / バンドパス / ハイパス） | |
| cutoff | カットオフ | |
| resonance | レゾナンス | |
| self-oscillating | 自己発振 | |
| drive / distortion | ドライブ / 歪み | |
| cross-modulation, FM | クロス・モジュレーション、FM | |
| feedback (in the sound) | フィードバック | |
| LFO | LFO | |
| rate / depth | レート / デプス | |
| modulation | モジュレーション | |
| random | ランダム | |
| tremolo | トレモロ | the Swell is a tremolo |
| envelope | エンベロープ | |
| fade in / fade out | フェード・イン / フェード・アウト | |
| glide (to new settings) | なめらかに移る | prose; "Morph" stays as the screen's name |
| crossfade | クロスフェード | |
| pressure (pressing a key harder) | プレッシャー（押す強さ） | |
| motion loop | モーション・ループ | |
| effect | エフェクト | |
| delay | ディレイ | |
| tape echo | テープ・エコー | |
| ping-pong | ピンポン | |
| reverb | リバーブ | |
| room / hall | ルーム / ホール | |
| freeze | フリーズ | |
| shimmer | シマー | |
| mix (dry / wet) | ミックス | |
| time | タイム | prose: 時間 |
| tempo / clock / sync | テンポ / クロック / 同期 | |
| MIDI in | MIDIイン | |
| headphones / speaker | ヘッドホン / スピーカー | |
| low end (bass) | 低音 | |
| volume (MASTER) | 音量 | MASTER stays as printed |
| calibration (the knobs') | キャリブレーション | |
| setting / settings | 設定 | |
| default (factory) | 初期設定 | factory reset: 初期化 |
| on / off | オン / オフ | |
| lit (a key's light) | 点灯 | blinking: 点滅 |
| gray (a value shown gray) | グレー表示 | |
| recipe (the manual's how-tos) | レシピ | |
| reference | リファレンス | |
| shortcuts | ショートカット | |
| help | ヘルプ | |
| start here | はじめに | |
| good to know | 補足 | |
| listen | 聴いてみる | the recipes' Listen line |
| beating (two tones) | うなり / うなる | |
| latch / latched | ラッチ / ラッチされる | the screen's "latched" stays |
| peek (at a page) | のぞく | |
| glide (a scene, a pitch) | なめらかに移る / すべる | a scene: なめらかに移る; a pitch: すべる |
| crossfade by pressure | 押した分だけクロスフェード | |
| gesture key | ジェスチャー・キー | |
| pressure gesture / held gesture | 押す強さのジェスチャー / 長押しのジェスチャー | |
| tutorial | チュートリアル | |
| speed mod / delay mod | スピード・モジュレーション / ディレイ・モジュレーション | in prose; the screen's Speed mod, Delay mod stay |
| send (LFO) | センド | ピッチ・センド, カットオフ・センド |
| runaway (echoes) | 暴走 | the screen's "runaway" stays |
| tail (reverb) | 残響 | |
| the pictures (a page's drawing) | 画面の絵 | Good to know's "The pictures" |
| stepped figure | ステップするフレーズ | |
| growl, snarl (the sound) | グロウル | うなり is only for beating |
| blend (a scene crossfade kept) | ブレンド | not 配合 |
| a gesture leaves / let go | 抜ける | not 去る |
| unlatch | 解除する | |
| restart (LFO reset) / back to the wave's start (hard sync) | リスタート / 先頭に戻る | not やり直す |
| the starting sound | 起動時のサウンド | |
| the room's settings (Setup) | 演奏環境の設定 | |
| rush / drag (timing) | 走る / もたる | |
| wow and flutter | ワウ・フラッター | |
| breathing (a light) | 明滅する | |
| Pitch glide (in prose) | グライド | beside すべる for a pitch moving |
| reverb types in prose | as on screen: Chorus hallのリバーブ, Cathedralリバーブ | |
| minor chord | マイナー・コード | not 短調 (a key) |
| translated by AI, not yet checked | AIによる翻訳です。ネイティブ・スピーカーによる確認はまだです。 | the note at the top of each page |
