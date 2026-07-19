#include "Dsp/RapReadyDSP.h"

#include <cmath>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>
#include <limits>

namespace
{
constexpr double sampleRate = 48000.0;

bool allFinite(const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (!std::isfinite(buffer.getSample(channel, sample)))
                return false;
    return true;
}

float renderSineRms(float frequency, float amount)
{
    constexpr int totalSamples = 48000;
    juce::AudioBuffer<float> buffer(1, totalSamples);
    for (int i = 0; i < totalSamples; ++i)
        buffer.setSample(0, i,
                         0.2f * std::sin(juce::MathConstants<double>::twoPi * frequency * i / sampleRate));

    rapready::RapReadyDSP dsp;
    dsp.setAmount(amount);
    dsp.prepare(sampleRate, totalSamples, 1);
    dsp.process(buffer);
    return buffer.getRMSLevel(0, totalSamples / 2, totalSamples / 2);
}
} // namespace

int main()
{
    int failures = 0;
    const auto check = [&](bool condition, const char* description)
    {
        std::cout << (condition ? "PASS  " : "FAIL  ") << description << '\n';
        if (!condition)
            ++failures;
    };

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(0.0f);
        dsp.prepare(sampleRate, 4096, 1);
        const auto latency = dsp.getLatencySamples();
        juce::AudioBuffer<float> buffer(1, 2048);
        juce::AudioBuffer<float> original(1, 2048);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto value = 0.35f * std::sin(juce::MathConstants<double>::twoPi * 997.0 * i / sampleRate);
            buffer.setSample(0, i, value);
            original.setSample(0, i, value);
        }
        dsp.process(buffer);
        float maxError = 0.0f;
        for (int i = latency; i < buffer.getNumSamples(); ++i)
            maxError =
                std::max(maxError, std::abs(buffer.getSample(0, i) - original.getSample(0, i - latency)));
        check(maxError < 1.0e-6f, "zero amount is a latency-matched dry path");
    }

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(100.0f);
        dsp.prepare(sampleRate, 8192, 2);
        juce::AudioBuffer<float> buffer(2, 8192);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample(
                    channel, i, 1.6f * std::sin(juce::MathConstants<double>::twoPi * 997.0 * i / sampleRate));
        dsp.process(buffer);
        check(allFinite(buffer), "extreme input produces only finite samples");
        check(buffer.getMagnitude(0, dsp.getLatencySamples(),
                                  buffer.getNumSamples() - dsp.getLatencySamples()) <= 0.88f,
              "lookahead limiter respects the internal ceiling");
        check(dsp.getGainReductionDb() > 0.1f, "gain-reduction meter responds");
    }

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(62.0f);
        dsp.prepare(44100.0, 1, 2);
        juce::AudioBuffer<float> buffer(2, 1);
        for (int i = 0; i < 2000; ++i)
        {
            buffer.setSample(0, 0, i == 10 ? std::numeric_limits<float>::quiet_NaN() : 0.0f);
            buffer.setSample(1, 0, 0.0f);
            dsp.process(buffer);
        }
        check(allFinite(buffer), "one-sample blocks and reset silence remain finite");
    }

    {
        const auto lowRms = renderSineRms(40.0f, 75.0f);
        const auto voiceRms = renderSineRms(1000.0f, 75.0f);
        check(voiceRms > lowRms * 2.0f, "rumble is attenuated relative to vocal-band energy");
    }

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(62.0f);
        dsp.prepare(192000.0, 257, 2);
        juce::AudioBuffer<float> buffer(2, 257);
        juce::Random random(12345);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample(channel, i, random.nextFloat() * 0.4f - 0.2f);
        dsp.process(buffer);
        check(allFinite(buffer), "192 kHz stereo processing remains finite");
    }

    std::cout << (failures == 0 ? "ALL DSP TESTS PASSED\n" : "DSP TESTS FAILED\n");
    return failures == 0 ? 0 : 1;
}
