# RapReady One research basis

## Answer first

There is no safe universal “perfect vocal preset.” Voice register, microphone, distance, room modes, delivery, and noise differ. The defensible product is a **conservative, adaptive, one-knob starting chain** whose fixed tone moves stay broad and small, whose dynamics are bounded, and whose meters tell the artist when the recording itself needs correction.

The 62% default is meant to turn a reasonably recorded dry rap vocal into a controlled, mix-ready vocal. Master loudness targets belong to the finished stereo mix, not this vocal insert.

## Public recommendations translated into the product

| Topic | Public guidance | RapReady One decision |
|---|---|---|
| Input level | Focusrite illustrates setting peaks around −12 dBFS. iZotope recommends vocal recording levels around −12 to −18 dBFS and preserving headroom. [Focusrite Scarlett guide](https://userguides.focusrite.com/hc/en-gb/articles/18676425481362-Scarlett-4i4-Hardware-Features), [iZotope home-vocal mistakes](https://www.izotope.com/community/blog/11-common-vocal-production-mistakes-in-home-studios) | The UI centers its advice on −12 dBFS, warns above −3 dBFS, and does not pretend to repair clipping. |
| Room and mic technique | Shure recommends a pop filter, a controlled room, consistent placement, and generally 6–12 inches (15–30 cm) of distance. [Shure home recording](https://www.shure.com/en-US/docs/education/introduction-to-home-recording) | No algorithmic promise to remove reflections or moving-mic tone; the UI starts with input guidance. |
| High-pass filtering | iZotope identifies rumble below 100 Hz and describes 50–100 Hz as a common vocal HPF range, while warning the engineer to stop before the voice becomes thin. [iZotope vocal EQ](https://www.izotope.com/community/blog/how-to-eq-vocals) | A conservative 45–90 Hz range with a 12 dB/octave biquad, bypassed at knob zero. |
| Corrective EQ | iZotope maps body to 100–400 Hz, boxiness to 400–800 Hz, intelligibility to 1.5–5 kHz, sibilance to 5–8 kHz, and air above that. It recommends small 1–2 dB moves in sensitive presence bands and rejects one-size-fits-all EQ. [iZotope vocal EQ](https://www.izotope.com/community/blog/how-to-eq-vocals) | Broad maximum moves of −2.2 dB mud, −1.2 dB box, +1.3 dB presence, and +1 dB air. No narrow fixed notches. |
| Gate/expander | FabFilter explains that high ratios increasingly behave like a gate and that Range should cap attenuation. Apple recommends hold/hysteresis to reduce chatter. [FabFilter Pro-G dynamics](https://www.fabfilter.com/help/pro-g/using/dynamiccontrols), [Logic Pro noise gate](https://support.apple.com/en-lamr/guide/logicpro/lgcef1bec259/mac) | A slow-closing downward expander rather than a hard gate, with a learned quiet-floor threshold and at most 12 dB attenuation. |
| Compression | iZotope suggests a fast peak-control stage around 3:1–8:1 and a slower body stage around 1.5:1–4:1, commonly totaling about 3–7 dB of vocal reduction. [Choosing a compressor](https://www.izotope.com/community/blog/choosing-the-right-compressor), [Basic vocal chain](https://www.izotope.com/en/learn/crafting-a-basic-vocal-chain) | Stereo-linked serial peak and body stages, limited to 2.5 + 4 dB maximum reduction. |
| De-essing | Apple places typical male sibilance around 3–6 kHz and female sibilance around 5–8 kHz and warns against excess reduction. [Apple DeEsser 2](https://support.apple.com/en-mide/102006) | A broad linked high-band detector/split near 5.8 kHz with no more than 4.5 dB reduction. It avoids pretending one center frequency fits every voice. |
| Saturation/clipping | iZotope describes clipping as destructive distortion and recommends subtle, monitored use; its example is about 1.2 dB. [iZotope clipping in mixing](https://www.izotope.com/community/blog/clipping-in-mixing) | No hard clipper. A low-wet normalized soft curve rounds peaks subtly and stays off at zero. |
| Limiting and mastering | Spotify currently describes −14 LUFS playback normalization and recommends at most −1 dBTP, or −2 dBTP for louder masters. ITU-R BS.1770 defines loudness/true-peak measurement. [Spotify loudness normalization](https://support.spotify.com/eg-en/artists/article/loudness-normalization/), [ITU-R BS.1770-5](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I/en) | This vocal insert does **not** chase −14 LUFS. It uses a low-latency sample-peak safety limiter; final mix loudness and true-peak compliance must be measured on the stereo master. |
| Adaptive assistance | iZotope’s Vocal Assistant analyzes an input passage before selecting level, register, EQ, sibilance, and dynamics behavior, including a rap target. [Nectar Vocal Assistant](https://www.izotope.com/community/blog/nectar-vocal-assistant) | Quiet blocks continuously teach a bounded noise-floor estimate. The first release avoids opaque voice “learning” claims that its deterministic DSP does not implement. |
| Logic compatibility | JUCE produces VST3, AU, and Standalone plug-ins from one project, but AU builds are macOS-only. Apple recommends `auval` and then testing in real hosts such as Logic. [JUCE plug-in tutorial](https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/), [Apple Audio Unit validation](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/AudioUnitDevelopmentFundamentals/AudioUnitDevelopmentFundamentals.html) | Windows validates the shared DSP, VST3 wrapper, standalone wrapper, and renderer. macOS CI builds and validates AU. A real Logic Pro scan/session remains required before claiming full commercial compatibility. |

## Why the knob is bounded

An aggressive universal preset can remove the fundamental of a deep voice, thin a light voice, make sibilance lisp, chop quiet word endings, or raise room noise. The macro therefore uses `smoothstep(amount / 100)` and moves every stage together inside conservative limits. It is intensity, not a claim that higher is better.

## Acceptance criteria

- Mono-to-mono and stereo-to-stereo at 44.1–192 kHz.
- Finite output for silence, denormals, non-finite input, arbitrary blocks, and 1-sample blocks.
- Knob zero equals a latency-matched dry signal.
- Stereo-linked dynamics preserve channel balance.
- Automation is smoothed and audio processing allocates no memory.
- Safety ceiling holds for sample peaks once the near-zero bypass crossfade is fully engaged; no claim of true-peak conformance without an independent oversampled meter.
- VST3 passes `pluginval` strictness 5 on Windows and macOS.
- AU passes Apple `auval`; actual Logic scan, playback, automation, save/restore, and bounce are separately verified on a Mac.
