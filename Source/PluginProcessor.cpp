#include "PluginProcessor.h"
#include "PluginEditor.h"

RapReadyOneAudioProcessor::RapReadyOneAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "RapReadyState", createParameterLayout())
{
    amountParameter = parameters.getRawParameterValue("amount");
}

juce::AudioProcessorValueTreeState::ParameterLayout RapReadyOneAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amount", 1}, "Rap Ready", juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f},
        62.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float value, int) { return juce::String(value, 0) + "%"; })
            .withValueFromStringFunction([](const juce::String& text) { return text.getFloatValue(); })));
    return layout;
}

void RapReadyOneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    processor.setAmount(amountParameter != nullptr ? amountParameter->load() : 62.0f);
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
    processor.setAmount(amountParameter != nullptr ? amountParameter->load() : 62.0f);
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
