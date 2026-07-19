# RapReady One

RapReady One is a single-knob, real-time vocal cleanup processor for bedroom rap recording. One shared C++/JUCE codebase produces:

- a **Windows VST3** plug-in;
- a **Windows standalone app** for live microphone testing and drag-and-drop file cleanup;
- a **Windows file renderer** for repeatable automated tests; and
- a **macOS Audio Unit (AUv2)** for Logic Pro, plus macOS VST3 and standalone builds.

The product promise is deliberately **mix-ready vocal**, not automatic song mastering. A vocal insert cannot balance a beat, choose artistic EQ for every voice, or master a finished stereo mix.

## One knob

`0` is a latency-matched dry path. `62` is the RAP DEFAULT. Turning toward `100` progressively applies the complete linked chain:

`HPF → adaptive soft expander → broad corrective EQ → fast peak compression → body compression → split-band de-essing → presence/air EQ → subtle saturation → LPF → 2 ms safety limiter`

The main view stays one-knob simple. Open **ADVANCED** only when a recording needs more help: each cleanup stage gets one guarded LOW-to-HIGH slider, plus a 10-band vocal EQ. Input, output, learned noise floor, and total gain-reduction meters remain visible in the compact view.

| Internal stage | Knob range |
|---|---|
| HPF | main default 45–90 Hz; Advanced range stays at or below 100 Hz |
| Adaptive expander | threshold = learned quiet floor + 8 dB; 0–12 dB maximum attenuation |
| Broad EQ | −2.2 dB at 260 Hz, −1.2 dB at 620 Hz, +1.3 dB at 3.6 kHz, +1 dB air shelf maximum |
| Peak compressor | guarded ratios/timing; no more than 3 dB reduction |
| Body compressor | guarded ratios/timing; no more than 4 dB reduction |
| De-esser | linked high-band reduction around 5.2–6.5 kHz; no more than 6 dB |
| Saturation | up to 18% wet at the Advanced maximum, normalized soft `tanh` curve |
| LPF | 20 kHz, moving to 18 kHz only near the top of the knob |
| Limiter | stereo-linked, 2 ms lookahead; down to −2 dBFS at the Advanced maximum |

At 0%, the signal is an exact latency-aligned dry path. A short crossfade immediately above 0% keeps bypass automation click-free; the limiter ceiling is guaranteed once that crossfade is fully engaged, not while automating through the near-zero transition.

## Advanced controls and comparison

Every stage slider uses `50` as the researched rap default. Moving lower relaxes only that stage; moving higher strengthens it inside fixed safety caps. The panel always shows a short explanation of the LOW and HIGH ends.

- **Filter, Expansion, Compression, De-ess, Saturation, Limiter:** one vertical macro each. These coordinate the technical threshold, ratio, timing, frequency, or ceiling values behind the scenes.
- **10-band EQ:** 80, 160, 315, 630 Hz, 1.25, 2.5, 4, 6.3, 10, and 16 kHz; each band runs from −6 to +6 dB. Start with 1–2 dB moves, especially from 4–16 kHz.
- **PREVIOUS / CURRENT:** after a real user edit, PREVIOUS auditions all 17 audio settings exactly as they were immediately before that edit. CURRENT returns to the new setup. It compares processing settings, not rewound audio; the theme never changes.
- **Theme:** Cyan, Violet, Emerald, Amber, Ice, or Rose. Every interface uses a true black background with text and accents derived from the selected hue. The choice is saved with plug-in state and never affects audio.

## Recording setup

Use a pop filter, reduce room reflections, and keep microphone distance consistent. Set interface gain so normal rap peaks land near **−12 dBFS**, never clipped. The UI reports READY between −18 and −3 dBFS; the −12 dBFS line is the preferred center, not a hard rule.

RapReady One can reduce steady low-level room noise, rumble, mud, level jumps, and sibilance. It cannot reconstruct clipped audio or fully remove room echo, beat bleed, fan bursts, or a moving microphone.

## Windows use

1. Download `RapReadyOne-Windows-x64.zip` from the latest GitHub Release and extract it.
2. Launch `RapReady One.exe`. For live testing, select the input/output device from **Options → Audio/MIDI Settings**.
3. For an existing recording, drag one WAV, AIFF, FLAC, OGG, or MP3 anywhere onto the app, or click **BROWSE FILE**. The app snapshots the current main and Advanced settings and writes a unique 24-bit `<name>-RapReady.wav` beside the source. The source is never overwritten, and cancellation leaves no partial result.
4. For a DAW, run `Install-VST3.ps1`, then rescan plug-ins and insert **Bedroom Labs → RapReady One** on a dry vocal track.
5. For a scripted offline test, run `RapReady File Renderer.exe input.wav output.wav 62`.

File cleanup accepts mono or stereo input and produces a latency-compensated 24-bit WAV at the same sample rate, channel count, and exact length. AAC/M4A and video containers are not accepted in this release.

## Logic Pro use

1. Download and extract `RapReadyOne-macOS-universal.zip` on a Mac.
2. Copy `RapReady One.component` to `~/Library/Audio/Plug-Ins/Components/`.
3. Restart Logic, open **Logic Pro → Settings → Plug-in Manager**, find **Bedroom Labs / RapReady One**, and scan it.
4. Insert it as an Audio FX plug-in on a mono or stereo vocal track.

The AU uses the same compact futuristic interface, six saved themes, Advanced controls, and PREVIOUS/CURRENT audition as the Windows build. File drag-and-drop is intentionally a standalone-app feature; in Logic, place the AU on an audio track.

The CI artifact is an ad-hoc-signed development build, not Apple-notarized commercial distribution. If macOS quarantines a build you created or obtained from this project, follow the troubleshooting note included in the package. Final public commercial distribution needs a Developer ID signature and Apple notarization.

## Build and test

Prerequisites: CMake 3.24+, Git, Python 3 for the renderer regression, and either Visual Studio 2022 with **Desktop development with C++** on Windows or current Xcode command-line tools on macOS.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Automation runs deterministic DSP tests at multiple sample rates, `pluginval` strictness 5 on VST3, and Apple `auval` on the AU. Apple explicitly notes that `auval` does not judge DSP quality or replace host testing, so a real Logic session remains the last compatibility gate.

## License

This repository is released under **GNU AGPLv3** because JUCE 8 is used under its AGPLv3 option. Closed-source or commercial distribution may require a JUCE commercial license; review the current JUCE terms before changing the distribution model.

The evidence and design limits behind the default are recorded in [RESEARCH.md](RESEARCH.md).
