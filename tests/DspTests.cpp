#include "Dsp/RapReadyDSP.h"
#include "PluginProcessor.h"

#include <algorithm>
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

float maximumDifference(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float result = 0.0f;
    for (int channel = 0; channel < a.getNumChannels(); ++channel)
        for (int sample = 0; sample < a.getNumSamples(); ++sample)
            result = std::max(result, std::abs(a.getSample(channel, sample) - b.getSample(channel, sample)));
    return result;
}

juce::AudioBuffer<float> makeFixture(int sampleCount)
{
    juce::AudioBuffer<float> result(1, sampleCount);
    juce::Random random(0x52505231);
    for (int i = 0; i < sampleCount; ++i)
    {
        const auto inPhrase = (i % 6000) < 4300;
        const auto voice = 0.17f * std::sin(juce::MathConstants<double>::twoPi * 155.0 * i / sampleRate) +
                           0.08f * std::sin(juce::MathConstants<double>::twoPi * 930.0 * i / sampleRate) +
                           0.035f * std::sin(juce::MathConstants<double>::twoPi * 7100.0 * i / sampleRate);
        const auto noise = (random.nextFloat() * 2.0f - 1.0f) * 0.002f;
        result.setSample(0, i, (inPhrase ? voice : voice * 0.008f) + noise);
    }
    return result;
}

void processInPartitions(rapready::RapReadyDSP& dsp, juce::AudioBuffer<float>& buffer, int partitionSize)
{
    for (int position = 0; position < buffer.getNumSamples(); position += partitionSize)
    {
        const auto count = std::min(partitionSize, buffer.getNumSamples() - position);
        juce::AudioBuffer<float> part(buffer.getNumChannels(), count);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            part.copyFrom(channel, 0, buffer, channel, position, count);
        dsp.process(part);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.copyFrom(channel, position, part, channel, 0, count);
    }
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
        juce::AudioBuffer<float> buffer(1, 8);
        buffer.clear();
        buffer.setSample(0, 2, 0.25f);
        juce::AudioBuffer<float> original;
        original.makeCopyOf(buffer);
        dsp.process(buffer);
        check(maximumDifference(buffer, original) == 0.0f, "process before prepare is a safe no-op");
    }

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
        auto whole = makeFixture(24000);
        juce::AudioBuffer<float> partitioned;
        partitioned.makeCopyOf(whole);

        rapready::RapReadyDSP wholeDsp;
        wholeDsp.setAmount(62.0f);
        wholeDsp.prepare(sampleRate, whole.getNumSamples(), 1);
        wholeDsp.process(whole);

        rapready::RapReadyDSP partitionedDsp;
        partitionedDsp.setAmount(62.0f);
        partitionedDsp.prepare(sampleRate, 257, 1);
        processInPartitions(partitionedDsp, partitioned, 257);
        check(maximumDifference(whole, partitioned) < 2.0e-5f,
              "audio is invariant to host block partitioning");
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
              "lookahead limiter respects the internal sample-peak ceiling");
        check(dsp.getGainReductionDb() > 0.1f, "gain-reduction meter responds");
    }

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(std::numeric_limits<float>::quiet_NaN());
        dsp.prepare(44100.0, 1, 2);
        juce::AudioBuffer<float> buffer(2, 1);
        for (int i = 0; i < 2000; ++i)
        {
            const auto invalid = i == 10 ? std::numeric_limits<float>::infinity()
                                         : (i == 11 ? std::numeric_limits<float>::max() : 0.1f);
            buffer.setSample(0, 0, invalid);
            buffer.setSample(1, 0, -invalid);
            dsp.process(buffer);
        }
        check(allFinite(buffer), "non-finite parameter and input recover to finite output");
    }

    {
        const auto lowRms = renderSineRms(40.0f, 75.0f);
        const auto voiceRms = renderSineRms(1000.0f, 75.0f);
        check(voiceRms > lowRms * 2.0f, "rumble is attenuated relative to vocal-band energy");
    }

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(100.0f);
        dsp.prepare(sampleRate, 512, 1);
        juce::AudioBuffer<float> warmup(1, 4096);
        warmup.clear();
        for (int i = 0; i < warmup.getNumSamples(); ++i)
            warmup.setSample(0, i,
                             0.8f * std::sin(juce::MathConstants<double>::twoPi * 220.0 * i / sampleRate));
        processInPartitions(dsp, warmup, 512);

        dsp.setAmount(0.0f);
        juce::AudioBuffer<float> settle(1, 30000);
        settle.clear();
        processInPartitions(dsp, settle, 127);

        juce::AudioBuffer<float> dry(1, 4096);
        juce::AudioBuffer<float> reference(1, 4096);
        for (int i = 0; i < dry.getNumSamples(); ++i)
        {
            const auto value = 0.2f * std::sin(juce::MathConstants<double>::twoPi * 440.0 * i / sampleRate);
            dry.setSample(0, i, value);
            reference.setSample(0, i, value);
        }
        processInPartitions(dsp, dry, 127);
        float maxError = 0.0f;
        for (int i = dsp.getLatencySamples(); i < dry.getNumSamples(); ++i)
            maxError = std::max(maxError, std::abs(dry.getSample(0, i) -
                                                   reference.getSample(0, i - dsp.getLatencySamples())));
        check(maxError < 2.0e-5f, "automation to zero settles to the exact dry path");
    }

    {
        rapready::RapReadyDSP dsp;
        dsp.setAmount(62.0f);
        dsp.prepare(192000.0, 257, 2);
        juce::AudioBuffer<float> buffer(2, 4096);
        juce::Random random(12345);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample(channel, i, random.nextFloat() * 0.4f - 0.2f);
        processInPartitions(dsp, buffer, 257);
        check(allFinite(buffer), "192 kHz stereo processing remains finite");
        check(buffer.getMagnitude(0, dsp.getLatencySamples(),
                                  buffer.getNumSamples() - dsp.getLatencySamples()) > 1.0e-5f,
              "192 kHz test exercises post-latency audio");

        dsp.reset();
        juce::AudioBuffer<float> silence(2, dsp.getLatencySamples() + 512);
        silence.clear();
        processInPartitions(dsp, silence, 113);
        check(silence.getMagnitude(0, 0, silence.getNumSamples()) == 0.0f,
              "reset clears filter, detector, queue, and delay state");
    }

    {
        RapReadyOneAudioProcessor processor;
        processor.setPlayConfigDetails(2, 2, sampleRate, 512);
        processor.prepareToPlay(sampleRate, 512);
        check(processor.getLatencySamples() == static_cast<int>(std::round(sampleRate * 0.002)),
              "plugin wrapper reports the prepared 2 ms latency to the host");

        juce::AudioBuffer<float> buffer(2, 2048);
        juce::MidiBuffer midi;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample(channel, i,
                                 0.15f *
                                     std::sin(juce::MathConstants<double>::twoPi * 330.0 * i / sampleRate));
        processor.processBlock(buffer, midi);
        check(allFinite(buffer), "plugin wrapper processes a stereo host buffer");

        if (auto* amount = processor.parameters.getParameter("amount"))
            amount->setValueNotifyingHost(0.83f);
        juce::MemoryBlock savedState;
        processor.getStateInformation(savedState);
        RapReadyOneAudioProcessor restored;
        restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));
        const auto* restoredAmount = restored.parameters.getRawParameterValue("amount");
        check(restoredAmount != nullptr && std::abs(restoredAmount->load() - 83.0f) < 0.2f,
              "plugin automation state survives save and restore");
    }

    std::cout << (failures == 0 ? "ALL DSP TESTS PASSED\n" : "DSP TESTS FAILED\n");
    return failures == 0 ? 0 : 1;
}
