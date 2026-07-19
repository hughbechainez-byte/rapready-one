# RapReady One research basis

## Answer first

There is no safe universal “perfect vocal preset.” Voice register, microphone, distance, room modes, delivery, and noise differ. The defensible product is a **conservative, adaptive, one-knob starting chain** whose fixed tone moves stay broad and small, whose dynamics are bounded, and whose meters tell the artist when the recording itself needs correction.

The 62% default is meant to turn a reasonably recorded dry rap vocal into a controlled, mix-ready vocal. Master loudness targets belong to the finished stereo mix, not this vocal insert.

## Public recommendations translated into the product

| Topic | Public guidance | RapReady One decision |
|---|---|---|
| Input level | Focusrite illustrates setting peaks around −12 dBFS. iZotope recommends vocal recording levels around −12 to −18 dBFS and preserving headroom. [Focusrite Scarlett guide](https://userguides.focusrite.com/hc/en-gb/articles/18676425481362-Scarlett-4i4-Hardware-Features), [iZotope home-vocal mistakes](https://www.izotope.com/community/blog/11-common-vocal-production-mistakes-in-home-studios) | The UI centers its advice on −12 dBFS, warns above −3 dBFS, and does not pretend to repair clipping. |
| Room and mic technique | Shure recommends a pop filter, a controlled room, consistent placement, and generally 6–12 inches (15–30 cm) of distance. [Shure home recording](https://www.shure.com/en-US/docs/education/introduction-to-home-recording) | No algorithmic promise to remove reflections or moving-mic tone; the UI starts with input guidance. |
| High-pass filtering | iZotope identifies rumble below 100 Hz and describes 50–100 Hz as a common vocal HPF range, while warning the engineer to stop before the voice becomes thin. [iZotope vocal EQ](https://www.izotope.com/community/blog/how-to-eq-vocals) | The default remains 45–90 Hz. The Advanced filter macro stays at or below 100 Hz and coordinates the low-pass filter as well. |
| Corrective EQ | iZotope maps body to 100–400 Hz, boxiness to 400–800 Hz, intelligibility to 1.5–5 kHz, sibilance to 5–8 kHz, sparkle to 8–12 kHz, and air above 12 kHz. It recommends small moves and rejects one-size-fits-all EQ. [iZotope vocal EQ](https://www.izotope.com/community/blog/how-to-eq-vocals) | The default broad curve stays small. Advanced adds ten user trims from −6 to +6 dB, labeled by vocal role, with 1–2 dB moves recommended. |
| Gate/expander | FabFilter explains that high ratios increasingly behave like a gate and that Range should cap attenuation. Apple recommends hold/hysteresis to reduce chatter. [FabFilter Pro-G dynamics](https://www.fabfilter.com/help/pro-g/using/dynamiccontrols), [Logic Pro noise gate](https://support.apple.com/en-lamr/guide/logicpro/lgcef1bec259/mac) | A slow-closing downward expander rather than a hard gate, with a learned quiet-floor threshold and at most 12 dB attenuation. |
| Compression | iZotope suggests a fast peak-control stage around 3:1–8:1 and a slower body stage around 1.5:1–4:1, commonly totaling about 3–7 dB of vocal reduction. [Choosing a compressor](https://www.izotope.com/community/blog/choosing-the-right-compressor), [Basic vocal chain](https://www.izotope.com/community/blog/crafting-a-basic-vocal-chain) | Stereo-linked serial peak and body stages, capped at 3 + 4 = 7 dB maximum reduction even at the Advanced maximum. |
| De-essing | Apple places typical male sibilance around 3–6 kHz and female sibilance around 5–8 kHz and warns against excess reduction. [Apple DeEsser 2](https://support.apple.com/en-mide/102006) | The Advanced macro moves the broad linked split from 6.5 to 5.2 kHz while changing sensitivity and slope; reduction never exceeds 6 dB. |
| Saturation/clipping | iZotope describes clipping as destructive distortion and recommends subtle, monitored use; its example is about 1.2 dB. [iZotope clipping in mixing](https://www.izotope.com/community/blog/clipping-in-mixing) | No hard clipper. A low-wet normalized soft curve rounds peaks subtly and stays off at zero. |
| Limiting and mastering | Spotify currently describes −14 LUFS playback normalization and recommends at most −1 dBTP, or −2 dBTP for louder masters. ITU-R BS.1770 defines loudness/true-peak measurement. [Spotify loudness normalization](https://support.spotify.com/eg-en/artists/article/loudness-normalization/), [ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I/en) | This vocal insert does **not** chase −14 LUFS. It uses a low-latency sample-peak safety limiter; final mix loudness and true-peak compliance must be measured on the stereo master. |
| Adaptive assistance | iZotope’s Vocal Assistant analyzes an input passage before selecting level, register, EQ, sibilance, and dynamics behavior, including a rap target. [Nectar Vocal Assistant](https://www.izotope.com/community/blog/nectar-vocal-assistant) | Quiet blocks continuously teach a bounded noise-floor estimate. The first release avoids opaque voice “learning” claims that its deterministic DSP does not implement. |
| Logic compatibility | JUCE produces VST3, AU, and Standalone plug-ins from one project, but AU builds are macOS-only. Apple recommends `auval` and then testing in real hosts such as Logic. [JUCE plug-in tutorial](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/), [Apple Audio Unit validation](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/AudioUnitDevelopmentFundamentals/AudioUnitDevelopmentFundamentals.html) | Windows validates the shared DSP, VST3 wrapper, standalone wrapper, and renderer. macOS CI builds and validates AU. A real Logic Pro scan/session remains required before claiming full commercial compatibility. |

## How the Advanced sliders are bounded

The midpoint of every stage macro exactly preserves the original default chain. Endpoints deliberately expose useful direction without turning technical controls into an unsafe preset editor.

| Macro | LOW | 50 default | HIGH |
|---|---|---|---|
| Filter | retains more extremes; HPF starts around 30–50 Hz | original 45–90 Hz curve | tighter extremes; HPF never above 100 Hz |
| Expansion | 0 dB Range, slower 220 ms close | learned floor +8 dB, up to 12 dB Range, 160 ms | learned floor +11 dB, still capped at 12 dB, 120 ms |
| Compression | effectively off | original serial peak/body preset | stronger thresholds/ratios, still capped at 3 + 4 dB |
| De-ess | 6.5 kHz split, lower sensitivity and 0.5 slope | 5.8 kHz split, original sensitivity and 0.75 slope | 5.2 kHz split, higher sensitivity and 0.9 slope, capped at 6 dB |
| Saturation | clean | original maximum 12% wet at full main amount | up to 18% wet at full main amount |
| Limiter | slower 120 ms release and more headroom | original 85 ms / ceiling | faster 60 ms release and ceiling down to −2 dBFS |

EQ bands are 80, 160, 315, 630 Hz, 1.25, 2.5, 4, 6.3, 10, and 16 kHz. A band at or above 45% of the host sample rate is bypassed instead of being folded into an invalid near-Nyquist filter.

## PREVIOUS/CURRENT behavior

FabFilter's public A/B guidance treats comparison as two complete settings and saves the current setup before switching. [FabFilter undo, redo, and A/B](https://www.fabfilter.com/help/pro-g/using/undoredo) RapReady One captures all 17 audio values immediately before a real user edit, ignores no-change clicks, and auditions PREVIOUS without rewriting saved plug-in state. Theme is intentionally excluded. CURRENT therefore remains recoverable even if the editor closes or the host saves while PREVIOUS is being heard.

## Standalone file handling

JUCE exposes OS file drag targeting, asynchronous file selection, sibling naming, and same-folder temporary files. [JUCE FileDragAndDropTarget](https://docs.juce.com/master/classjuce_1_1FileDragAndDropTarget.html), [FileChooser](https://docs.juce.com/master/classjuce_1_1FileChooser.html), [File](https://docs.juce.com/master/classFile.html), [TemporaryFile](https://docs.juce.com/master/classjuce_1_1TemporaryFile.html) The Windows standalone snapshots current audio settings, renders on a worker thread, verifies exact rate/channels/length/24-bit output, and atomically commits only on success. Source files and existing targets survive errors or cancellation.

## Why the knob is bounded

An aggressive universal preset can remove the fundamental of a deep voice, thin a light voice, make sibilance lisp, chop quiet word endings, or raise room noise. The macro therefore uses `smoothstep(amount / 100)` and moves every stage together inside conservative limits. It is intensity, not a claim that higher is better.

## Acceptance criteria

- Mono-to-mono and stereo-to-stereo at 44.1–192 kHz.
- Finite output for silence, denormals, non-finite input, arbitrary blocks, and 1-sample blocks.
- Knob zero equals a latency-matched dry signal.
- Stereo-linked dynamics preserve channel balance.
- Automation is smoothed and audio processing allocates no memory.
- Advanced stage defaults reproduce the original chain; endpoints remain finite and bounded.
- PREVIOUS/CURRENT compares all audio controls while never changing the saved theme or current-state persistence.
- Offline cleanup preserves source bytes, commits atomically, reports monotonic progress, and writes exact-length 24-bit WAV.
- Safety ceiling holds for sample peaks once the near-zero bypass crossfade is fully engaged; no claim of true-peak conformance without an independent oversampled meter.
- VST3 passes `pluginval` strictness 5 on Windows and macOS.
- AU passes Apple `auval`; actual Logic scan, playback, automation, save/restore, and bounce are separately verified on a Mac.
