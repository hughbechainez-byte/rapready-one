#pragma once

#include "Dsp/RapReadyDSP.h"
#include "Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>

class RapReadyOneAudioProcessor final : public juce::AudioProcessor
{
  public:
    RapReadyOneAudioProcessor();
    ~RapReadyOneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Rap Clean"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] float getInputLevelDb() const noexcept { return processor.getInputLevelDb(); }
    [[nodiscard]] float getOutputLevelDb() const noexcept { return processor.getOutputLevelDb(); }
    [[nodiscard]] float getGainReductionDb() const noexcept { return processor.getGainReductionDb(); }
    [[nodiscard]] float getNoiseFloorDb() const noexcept { return processor.getNoiseFloorDb(); }
    [[nodiscard]] rapready::AdvancedSettings getAdvancedSettings() const noexcept;
    void setAuditionSnapshot(const rapready::parameters::AudioSnapshot& snapshot) noexcept;
    void clearAuditionSnapshot() noexcept;

    juce::AudioProcessorValueTreeState parameters;

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    rapready::RapReadyDSP processor;
    std::atomic<float>* amountParameter = nullptr;
    std::array<std::atomic<float>*, rapready::parameters::stageIds.size()> stageParameters{};
    std::array<std::atomic<float>*, rapready::parameters::eqIds.size()> eqParameters{};
    std::array<std::atomic<float>, rapready::parameters::audioStateIds.size()> auditionValues{};
    std::atomic<bool> auditionActive{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RapReadyOneAudioProcessor)
};
