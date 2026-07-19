#include "Dsp/RapReadyDSP.h"
#include "OfflineRenderer.h"
#include "Parameters.h"
#include "PluginProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>
#include <limits>
#include <string_view>
#include <vector>

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

struct RenderMetrics
{
    float rms = 0.0f;
    float peak = 0.0f;
    float gainReductionDb = 0.0f;
    bool finite = false;
};

RenderMetrics renderAdvancedSine(float frequency, float amplitude,
                                 const rapready::AdvancedSettings& settings,
                                 float amount = 100.0f)
{
    constexpr int totalSamples = 96000;
    juce::AudioBuffer<float> buffer(1, totalSamples);
    for (int i = 0; i < totalSamples; ++i)
        buffer.setSample(0, i,
                         amplitude * std::sin(juce::MathConstants<double>::twoPi * frequency * i /
                                              sampleRate));

    rapready::RapReadyDSP dsp;
    dsp.setAmount(amount);
    dsp.setAdvancedSettings(settings);
    dsp.prepare(sampleRate, 257, 1);
    processInPartitions(dsp, buffer, 257);

    const auto measurementStart = totalSamples / 2;
    const auto measurementLength = totalSamples - measurementStart;
    return {buffer.getRMSLevel(0, measurementStart, measurementLength),
            buffer.getMagnitude(0, measurementStart, measurementLength),
            dsp.getGainReductionDb(), allFinite(buffer)};
}

rapready::AdvancedSettings makeNeutralAdvancedSettings()
{
    rapready::AdvancedSettings settings;
    settings.expansion = 0.0f;
    settings.compression = 0.0f;
    settings.deEss = 0.0f;
    settings.saturation = 0.0f;
    settings.limiter = 0.0f;
    return settings;
}

bool setParameterValue(RapReadyOneAudioProcessor& processor, const char* id, float plainValue)
{
    if (auto* parameter = processor.parameters.getParameter(id))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
        return true;
    }
    return false;
}

float getParameterValue(const RapReadyOneAudioProcessor& processor, const char* id)
{
    if (const auto* value = processor.parameters.getRawParameterValue(id))
        return value->load();
    return std::numeric_limits<float>::quiet_NaN();
}

class ScopedTestDirectory final
{
  public:
    ScopedTestDirectory()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile(juce::String{"RapReadyOneTests-"} +
                                                     juce::Uuid().toString(),
                                                 juce::String{}, false))
    {
        created = directory.createDirectory().wasOk();
    }

    ~ScopedTestDirectory()
    {
        if (created)
            directory.deleteRecursively(false);
    }

    [[nodiscard]] const juce::File& get() const noexcept { return directory; }
    [[nodiscard]] bool wasCreated() const noexcept { return created; }

  private:
    juce::File directory;
    bool created = false;
};

bool writeMonoWavFixture(const juce::File& file, int sampleCount)
{
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions()
                             .withSampleRate(sampleRate)
                             .withNumChannels(1)
                             .withBitsPerSample(24);
    auto writer = wav.createWriterFor(stream, options);
    if (writer == nullptr)
        return false;

    juce::AudioBuffer<float> audio(1, sampleCount);
    for (int i = 0; i < sampleCount; ++i)
    {
        auto value = 0.16f * std::sin(juce::MathConstants<double>::twoPi * 180.0 * i / sampleRate) +
                     0.035f * std::sin(juce::MathConstants<double>::twoPi * 4200.0 * i / sampleRate);
        if (i == sampleCount - 1)
            value += 0.2f;
        audio.setSample(0, i, value);
    }
    return writer->writeFromAudioSampleBuffer(audio, 0, sampleCount);
}

bool loadFileBytes(const juce::File& file, juce::MemoryBlock& bytes)
{
    bytes.reset();
    return file.loadFileAsData(bytes);
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
        dsp.setAmount(0.0f);
        dsp.prepare(sampleRate, 128, 1);
        const auto latency = dsp.getLatencySamples();

        juce::AudioBuffer<float> oldDrySample(1, 1);
        oldDrySample.setSample(0, 0, 1.5f);
        dsp.process(oldDrySample);

        dsp.setAmount(100.0f);
        juce::AudioBuffer<float> transition(1, latency);
        transition.clear();
        dsp.process(transition);
        check(std::abs(transition.getSample(0, latency - 1) - 1.5f) < 1.0e-5f,
              "lookahead controls stay aligned with audio during knob automation");
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

    {
        RapReadyOneAudioProcessor processor;
        const auto& ids = rapready::parameters::audioStateIds;
        const auto allAudioParametersExist =
            std::all_of(ids.begin(), ids.end(), [&](const char* id)
                        { return processor.parameters.getParameter(id) != nullptr; });
        const auto themeIsSeparate =
            std::none_of(ids.begin(), ids.end(), [](const char* id)
                         { return std::string_view{id} == rapready::parameters::theme; });
        check(ids.size() == 17 && processor.getParameters().size() == 18 && allAudioParametersExist &&
                  processor.parameters.getParameter(rapready::parameters::theme) != nullptr && themeIsSeparate,
              "parameter layout contains 17 audio controls plus a separate theme choice");

        const auto defaults = processor.getAdvancedSettings();
        const auto near = [](float value, float expected)
        {
            return std::abs(value - expected) < 0.011f;
        };
        const auto stagesDefault = near(defaults.filter, 50.0f) &&
                                   near(defaults.expansion, 50.0f) &&
                                   near(defaults.compression, 50.0f) && near(defaults.deEss, 50.0f) &&
                                   near(defaults.saturation, 50.0f) && near(defaults.limiter, 50.0f);
        const auto eqDefaults = std::all_of(defaults.eqGainDb.begin(), defaults.eqGainDb.end(),
                                            [&](float value) { return near(value, 0.0f); });
        check(std::abs(getParameterValue(processor, rapready::parameters::amount) - 62.0f) < 0.01f &&
                  stagesDefault && eqDefaults &&
                  getParameterValue(processor, rapready::parameters::theme) == 0.0f,
              "advanced controls, EQ, amount, and theme have stable documented defaults");
    }

    {
        RapReadyOneAudioProcessor processor;
        const std::array<float, 17> wantedAudioValues{
            73.4f, 0.0f, 17.5f, 38.0f, 61.0f, 82.5f, 100.0f,
            -6.0f, -4.5f, -3.0f, -1.5f, -0.5f, 0.5f, 1.5f, 3.0f, 4.5f, 6.0f,
        };
        bool allParametersSet = true;
        for (std::size_t i = 0; i < rapready::parameters::audioStateIds.size(); ++i)
            allParametersSet &= setParameterValue(processor, rapready::parameters::audioStateIds[i],
                                                   wantedAudioValues[i]);
        allParametersSet &= setParameterValue(processor, rapready::parameters::theme, 4.0f);

        juce::MemoryBlock savedState;
        processor.getStateInformation(savedState);
        RapReadyOneAudioProcessor restored;
        restored.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));

        bool audioRestored = allParametersSet;
        for (std::size_t i = 0; i < rapready::parameters::audioStateIds.size(); ++i)
            audioRestored &=
                std::abs(getParameterValue(restored, rapready::parameters::audioStateIds[i]) -
                         wantedAudioValues[i]) < 0.11f;
        check(audioRestored &&
                  std::abs(getParameterValue(restored, rapready::parameters::theme) - 4.0f) < 0.01f,
              "all audio controls and the selected interface theme survive state round-trip");

        std::array<float, rapready::parameters::audioStateIds.size()> previousAudio{};
        for (std::size_t i = 0; i < previousAudio.size(); ++i)
            previousAudio[i] = getParameterValue(restored, rapready::parameters::audioStateIds[i]);
        for (const auto* id : rapready::parameters::audioStateIds)
            setParameterValue(restored, id, 0.0f);
        for (std::size_t i = 0; i < previousAudio.size(); ++i)
            setParameterValue(restored, rapready::parameters::audioStateIds[i], previousAudio[i]);
        bool previousAudioRestored = true;
        for (std::size_t i = 0; i < previousAudio.size(); ++i)
            previousAudioRestored &=
                std::abs(getParameterValue(restored, rapready::parameters::audioStateIds[i]) -
                         previousAudio[i]) < 0.11f;
        check(previousAudioRestored &&
                  std::abs(getParameterValue(restored, rapready::parameters::theme) - 4.0f) < 0.01f,
              "audio-only previous-state restore does not change the interface theme");
    }

    {
        auto lowEndpoints = makeNeutralAdvancedSettings();
        lowEndpoints.filter = 0.0f;
        lowEndpoints.eqGainDb.fill(-6.0f);
        auto highEndpoints = lowEndpoints;
        highEndpoints.filter = 100.0f;
        highEndpoints.expansion = 100.0f;
        highEndpoints.compression = 100.0f;
        highEndpoints.deEss = 100.0f;
        highEndpoints.saturation = 100.0f;
        highEndpoints.limiter = 100.0f;
        highEndpoints.eqGainDb.fill(6.0f);
        const auto low = renderAdvancedSine(997.0f, 0.2f, lowEndpoints);
        const auto high = renderAdvancedSine(997.0f, 0.2f, highEndpoints);

        rapready::AdvancedSettings invalid;
        invalid.filter = std::numeric_limits<float>::quiet_NaN();
        invalid.expansion = -std::numeric_limits<float>::infinity();
        invalid.compression = std::numeric_limits<float>::infinity();
        invalid.deEss = -1000.0f;
        invalid.saturation = 1000.0f;
        invalid.limiter = std::numeric_limits<float>::quiet_NaN();
        for (std::size_t i = 0; i < invalid.eqGainDb.size(); ++i)
            invalid.eqGainDb[i] = i % 2 == 0 ? std::numeric_limits<float>::infinity()
                                             : std::numeric_limits<float>::quiet_NaN();
        const auto sanitized = renderAdvancedSine(997.0f, 0.2f, invalid);
        check(low.finite && high.finite && sanitized.finite && std::isfinite(low.rms) &&
                  std::isfinite(high.rms) && std::isfinite(sanitized.rms),
              "advanced low, high, and invalid endpoints always produce finite audio");
    }

    {
        auto cut = makeNeutralAdvancedSettings();
        cut.eqGainDb[6] = -6.0f;
        auto boost = cut;
        boost.eqGainDb[6] = 6.0f;
        const auto cutMetrics = renderAdvancedSine(4000.0f, 0.02f, cut);
        const auto boostMetrics = renderAdvancedSine(4000.0f, 0.02f, boost);
        check(cutMetrics.finite && boostMetrics.finite && boostMetrics.rms > cutMetrics.rms * 3.0f,
              "4 kHz EQ boost is directionally and materially louder than the matching cut");
    }

    {
        auto relaxed = makeNeutralAdvancedSettings();
        relaxed.filter = 0.0f;
        auto strict = relaxed;
        strict.filter = 100.0f;
        const auto relaxedMetrics = renderAdvancedSine(60.0f, 0.08f, relaxed);
        const auto strictMetrics = renderAdvancedSine(60.0f, 0.08f, strict);
        check(relaxedMetrics.finite && strictMetrics.finite &&
                  strictMetrics.rms < relaxedMetrics.rms * 0.65f,
              "higher filter macro removes materially more 60 Hz rumble");
    }

    {
        auto gentle = makeNeutralAdvancedSettings();
        gentle.compression = 0.0f;
        auto firm = gentle;
        firm.compression = 100.0f;
        const auto gentleMetrics = renderAdvancedSine(220.0f, 0.35f, gentle);
        const auto firmMetrics = renderAdvancedSine(220.0f, 0.35f, firm);
        check(gentleMetrics.finite && firmMetrics.finite &&
                  firmMetrics.gainReductionDb > gentleMetrics.gainReductionDb + 3.0f,
              "higher compression macro creates materially more gain reduction");
    }

    {
        auto relaxed = makeNeutralAdvancedSettings();
        relaxed.limiter = 0.0f;
        auto strict = relaxed;
        strict.limiter = 100.0f;
        const auto relaxedMetrics = renderAdvancedSine(997.0f, 1.4f, relaxed);
        const auto strictMetrics = renderAdvancedSine(997.0f, 1.4f, strict);
        check(relaxedMetrics.finite && strictMetrics.finite &&
                  strictMetrics.peak < relaxedMetrics.peak - 0.05f && strictMetrics.peak < 0.81f,
              "higher limiter macro lowers the maximum output ceiling");
    }

    {
        ScopedTestDirectory files;
        const auto input = files.get().getChildFile("Bedroom Take.WAV");
        const auto occupied = files.get().getChildFile("Bedroom Take-RapReady.wav");
        const std::string sentinel = "ORIGINAL TARGET MUST SURVIVE";
        const auto fixtureWritten = files.wasCreated() && writeMonoWavFixture(input, 4097);
        const auto sentinelWritten = occupied.replaceWithData(sentinel.data(), sentinel.size());
        juce::MemoryBlock sourceBefore;
        juce::MemoryBlock targetBefore;
        const auto snapshotsLoaded = loadFileBytes(input, sourceBefore) && loadFileBytes(occupied, targetBefore);

        const auto unique = rapready::makeUniqueOutputFile(input);
        check(fixtureWritten && sentinelWritten && snapshotsLoaded &&
                  rapready::hasSupportedAudioExtension(input) && unique != occupied &&
                  unique.getParentDirectory() == input.getParentDirectory() && !unique.exists() &&
                  unique.hasFileExtension(".wav"),
              "supported uppercase input produces a collision-safe sibling WAV name");

        rapready::OfflineRenderRequest protectedRequest;
        protectedRequest.inputFile = input;
        protectedRequest.outputFile = occupied;
        const auto protectedResult = rapready::renderAudioFile(protectedRequest);
        juce::MemoryBlock targetAfterProtection;
        loadFileBytes(occupied, targetAfterProtection);
        check(protectedResult.status == rapready::OfflineRenderStatus::failed &&
                  targetAfterProtection == targetBefore,
              "offline render refuses to overwrite an existing target without explicit permission");

        rapready::OfflineRenderRequest cancelledRequest;
        cancelledRequest.inputFile = input;
        cancelledRequest.outputFile = occupied;
        cancelledRequest.replaceExisting = true;
        int cancelChecks = 0;
        std::vector<float> cancelProgress;
        const auto cancelled = rapready::renderAudioFile(
            cancelledRequest, [&] { return ++cancelChecks >= 3; },
            [&](float value) { cancelProgress.push_back(value); });
        juce::MemoryBlock sourceAfterCancel;
        juce::MemoryBlock targetAfterCancel;
        loadFileBytes(input, sourceAfterCancel);
        loadFileBytes(occupied, targetAfterCancel);
        const auto filesAfterCancel =
            files.get().findChildFiles(juce::File::findFilesAndDirectories, false).size();
        check(cancelled.status == rapready::OfflineRenderStatus::cancelled && cancelChecks >= 3 &&
                  cancelProgress.size() >= 2 && cancelProgress.front() == 0.0f &&
                  cancelProgress.back() > 0.0f && sourceAfterCancel == sourceBefore &&
                  targetAfterCancel == targetBefore && filesAfterCancel == 2,
              "mid-render cancellation preserves both source and pre-existing target atomically");

        const auto output = unique;
        rapready::OfflineRenderRequest successfulRequest;
        successfulRequest.inputFile = input;
        successfulRequest.outputFile = output;
        successfulRequest.amount = 62.0f;
        std::vector<float> progress;
        const auto rendered = rapready::renderAudioFile(
            successfulRequest, {}, [&](float value) { progress.push_back(value); });
        const auto progressIsMonotonic =
            !progress.empty() && progress.front() == 0.0f && progress.back() == 1.0f &&
            std::is_sorted(progress.begin(), progress.end()) &&
            std::all_of(progress.begin(), progress.end(),
                        [](float value) { return value >= 0.0f && value <= 1.0f; });
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        auto outputReader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(output));
        juce::MemoryBlock sourceAfterSuccess;
        juce::MemoryBlock targetAfterSuccess;
        loadFileBytes(input, sourceAfterSuccess);
        loadFileBytes(occupied, targetAfterSuccess);
        check(rendered.wasSuccessful() && rendered.outputFile == output && progressIsMonotonic &&
                  outputReader != nullptr && outputReader->numChannels == 1 &&
                  outputReader->lengthInSamples == 4097 && outputReader->bitsPerSample == 24 &&
                  std::abs(outputReader->sampleRate - sampleRate) < 0.5 &&
                  sourceAfterSuccess == sourceBefore && targetAfterSuccess == targetBefore,
              "offline render reports monotonic progress and commits an exact-length 24-bit WAV");
    }

    std::cout << (failures == 0 ? "ALL DSP TESTS PASSED\n" : "DSP TESTS FAILED\n");
    return failures == 0 ? 0 : 1;
}
