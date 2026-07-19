# RapReady One

RapReady One is a single-knob, real-time vocal cleanup processor for bedroom rap recording. One shared C++/JUCE codebase produces:

- a **Windows VST3** plug-in;
- a **Windows standalone app** for live microphone testing;
- a **Windows file renderer** for repeatable before/after WAV tests; and
- a **macOS Audio Unit (AUv2)** for Logic Pro, plus macOS VST3 and standalone builds.

The product promise is deliberately **mix-ready vocal**, not automatic song mastering. A vocal insert cannot balance a beat, choose artistic EQ for every voice, or master a finished stereo mix.

## One knob

`0` is a latency-matched dry path. `62` is the RAP DEFAULT. Turning toward `100` progressively applies the complete linked chain:

`HPF → adaptive soft expander → broad corrective EQ → fast peak compression → body compression → split-band de-essing → presence/air EQ → subtle saturation → LPF → 2 ms safety limiter`

The plug-in exposes only the **Rap Ready** automation parameter. Input, output, learned noise floor, and total gain-reduction meters provide guidance without adding setup controls.

| Internal stage | Knob range |
|---|---|
| HPF | bypass at 0; 45–90 Hz when active |
| Adaptive expander | threshold = learned quiet floor + 8 dB; 0–12 dB maximum attenuation |
| Broad EQ | −2.2 dB at 260 Hz, −1.2 dB at 620 Hz, +1.3 dB at 3.6 kHz, +1 dB air shelf maximum |
| Peak compressor | 1:1–4:1; 0–2.5 dB maximum reduction |
| Body compressor | 1:1–2.3:1; 0–4 dB maximum reduction |
| De-esser | linked high-band reduction above about 5.8 kHz; 0–4.5 dB maximum |
| Saturation | 0–12% wet, normalized soft `tanh` curve |
| LPF | 20 kHz, moving to 18 kHz only near the top of the knob |
| Limiter | stereo-linked, 2 ms lookahead; −0.3 to −1.2 dBFS sample-peak ceiling |

## Recording setup

Use a pop filter, reduce room reflections, and keep microphone distance consistent. Set interface gain so normal rap peaks land near **−12 dBFS**, never clipped. The UI reports READY between −18 and −3 dBFS; the −12 dBFS line is the preferred center, not a hard rule.

RapReady One can reduce steady low-level room noise, rumble, mud, level jumps, and sibilance. It cannot reconstruct clipped audio or fully remove room echo, beat bleed, fan bursts, or a moving microphone.

## Windows use

1. Download `RapReadyOne-Windows-x64.zip` from the latest GitHub Release and extract it.
2. Launch `RapReady One.exe` for the standalone mic test. Select the input/output device from **Options → Audio/MIDI Settings**.
3. For a DAW, run `Install-VST3.ps1`, then rescan plug-ins and insert **Bedroom Labs → RapReady One** on a dry vocal track.
4. For an offline test, run `RapReady File Renderer.exe input.wav output.wav 62`.

The renderer accepts mono or stereo input and produces a latency-compensated 24-bit WAV of the same length.

## Logic Pro use

1. Download and extract `RapReadyOne-macOS-universal.zip` on a Mac.
2. Copy `RapReady One.component` to `~/Library/Audio/Plug-Ins/Components/`.
3. Restart Logic, open **Logic Pro → Settings → Plug-in Manager**, find **Bedroom Labs / RapReady One**, and scan it.
4. Insert it as an Audio FX plug-in on a mono or stereo vocal track.

The CI artifact is an ad-hoc-signed development build, not Apple-notarized commercial distribution. If macOS quarantines a build you created or obtained from this project, follow the troubleshooting note included in the package. Final public commercial distribution needs a Developer ID signature and Apple notarization.

## Build and test

Prerequisites: CMake 3.22+, Git, and either Visual Studio 2022 with **Desktop development with C++** on Windows or current Xcode command-line tools on macOS.

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

