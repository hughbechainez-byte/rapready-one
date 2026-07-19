#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>

namespace
{
rapready::AdvancedSettings advancedFromSnapshot(const rapready::parameters::AudioSnapshot& snapshot)
{
    rapready::AdvancedSettings settings;
    settings.filter = snapshot[1];
    settings.expansion = snapshot[2];
    settings.compression = snapshot[3];
    settings.deEss = snapshot[4];
    settings.saturation = snapshot[5];
    settings.limiter = snapshot[6];
    for (std::size_t i = 0; i < settings.eqGainDb.size(); ++i)
        settings.eqGainDb[i] = snapshot[7 + i];
    return settings;
}
} // namespace

RapReadyOneAudioProcessor::RapReadyOneAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "RapReadyState", createParameterLayout())
{
    amountParameter = parameters.getRawParameterValue(rapready::parameters::amount);
    for (std::size_t i = 0; i < stageParameters.size(); ++i)
        stageParameters[i] = parameters.getRawParameterValue(rapready::parameters::stageIds[i]);
    for (std::size_t i = 0; i < eqParameters.size(); ++i)
        eqParameters[i] = parameters.getRawParameterValue(rapready::parameters::eqIds[i]);

    for (std::size_t i = 0; i < auditionValues.size(); ++i)
        if (const auto* value = parameters.getRawParameterValue(rapready::parameters::audioStateIds[i]))
            auditionValues[i].store(value->load(), std::memory_order_relaxed);
}

const juce::String RapReadyOneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

juce::AudioProcessorValueTreeState::ParameterLayout RapReadyOneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{rapready::parameters::amount, 1}, "Rap Ready",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f},
        62.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float value, int) { return juce::String(value, 0) + "%"; })
            .withValueFromStringFunction([](const juce::String& text) { return text.getFloatValue(); })));

    const std::array<juce::String, 6> stageNames{
        "Filter", "Expansion", "Compression", "De-ess", "Saturation", "Limiter",
    };
    for (std::size_t i = 0; i < rapready::parameters::stageIds.size(); ++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{rapready::parameters::stageIds[i], 1}, stageNames[i],
            juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 50.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel("%")
                .withStringFromValueFunction(
                    [](float value, int) { return juce::String(value, 0) + "%"; })
                .withValueFromStringFunction(
                    [](const juce::String& text) { return text.getFloatValue(); })));

    const std::array<juce::String, 10> eqNames{
        "EQ 80 Hz",   "EQ 160 Hz",  "EQ 315 Hz", "EQ 630 Hz", "EQ 1.25 kHz",
        "EQ 2.5 kHz", "EQ 4 kHz",   "EQ 6.3 kHz", "EQ 10 kHz", "EQ 16 kHz",
    };
    for (std::size_t i = 0; i < rapready::parameters::eqIds.size(); ++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{rapready::parameters::eqIds[i], 1}, eqNames[i],
            juce::NormalisableRange<float>{-6.0f, 6.0f, 0.1f}, 0.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel("dB")
                .withStringFromValueFunction(
                    [](float value, int) { return juce::String(value, 1) + " dB"; })
                .withValueFromStringFunction(
                    [](const juce::String& text) { return text.getFloatValue(); })));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{rapready::parameters::theme, 1}, "Interface Theme",
        juce::StringArray{"Cyan", "Violet", "Emerald", "Amber", "Ice", "Rose"}, 0));
    return layout;
}

rapready::AdvancedSettings RapReadyOneAudioProcessor::getAdvancedSettings() const noexcept
{
    rapready::AdvancedSettings settings;
    const auto load = [](const std::atomic<float>* value, float fallback)
    {
        return value != nullptr ? value->load() : fallback;
    };
    settings.filter = load(stageParameters[0], 50.0f);
    settings.expansion = load(stageParameters[1], 50.0f);
    settings.compression = load(stageParameters[2], 50.0f);
    settings.deEss = load(stageParameters[3], 50.0f);
    settings.saturation = load(stageParameters[4], 50.0f);
    settings.limiter = load(stageParameters[5], 50.0f);
    for (std::size_t i = 0; i < settings.eqGainDb.size(); ++i)
        settings.eqGainDb[i] = load(eqParameters[i], 0.0f);
    return settings;
}

void RapReadyOneAudioProcessor::setAuditionSnapshot(
    const rapready::parameters::AudioSnapshot& snapshot) noexcept
{
    for (std::size_t i = 0; i < snapshot.size(); ++i)
        auditionValues[i].store(snapshot[i], std::memory_order_relaxed);
    auditionActive.store(true, std::memory_order_release);
}

void RapReadyOneAudioProcessor::clearAuditionSnapshot() noexcept
{
    auditionActive.store(false, std::memory_order_release);
}

void RapReadyOneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    processor.setAmount(amountParameter != nullptr ? amountParameter->load() : 62.0f);
    processor.setAdvancedSettings(getAdvancedSettings());
    processor.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples(processor.getLatencySamples());
}

void RapReadyOneAudioProcessor::releaseResources()
{
    processor.reset();
}

bool RapReadyOneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == output;
}

void RapReadyOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    if (auditionActive.load(std::memory_order_acquire))
    {
        rapready::parameters::AudioSnapshot snapshot{};
        for (std::size_t i = 0; i < snapshot.size(); ++i)
            snapshot[i] = auditionValues[i].load(std::memory_order_relaxed);
        processor.setAmount(snapshot[0]);
        processor.setAdvancedSettings(advancedFromSnapshot(snapshot));
    }
    else
    {
        processor.setAmount(amountParameter != nullptr ? amountParameter->load() : 62.0f);
        processor.setAdvancedSettings(getAdvancedSettings());
    }
    processor.process(buffer);
}

juce::AudioProcessorEditor* RapReadyOneAudioProcessor::createEditor()
{
    return new RapReadyOneAudioProcessorEditor(*this);
}

void RapReadyOneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void RapReadyOneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary(data, sizeInBytes))
        if (state->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*state));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RapReadyOneAudioProcessor();
}
