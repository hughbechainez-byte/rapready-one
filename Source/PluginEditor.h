#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class RapReadyLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosition,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};

class RapReadyOneAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit RapReadyOneAudioProcessorEditor(RapReadyOneAudioProcessor&);
    ~RapReadyOneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

  private:
    void timerCallback() override;
    static float meterProportion(float decibels) noexcept;

    RapReadyOneAudioProcessor& audioProcessor;
    RapReadyLookAndFeel lookAndFeel;
    juce::Slider amountKnob;
    juce::AudioProcessorValueTreeState::SliderAttachment amountAttachment;

    float inputDb = -100.0f;
    float outputDb = -100.0f;
    float reductionDb = 0.0f;
    float noiseFloorDb = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RapReadyOneAudioProcessorEditor)
};
