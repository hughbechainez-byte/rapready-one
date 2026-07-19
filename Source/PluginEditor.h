#pragma once

#include "AdvancedPanel.h"
#include "OfflineRenderer.h"
#include "PluginProcessor.h"
#include "Theme.h"

#include <JuceHeader.h>
#include <atomic>
#include <thread>

class RapReadyLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    void setPalette(const rapready::ThemePalette& newPalette) { palette = newPalette; }
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosition,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosition,
                          float minSliderPosition, float maxSliderPosition,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted,
                              bool down) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool down, int buttonX, int buttonY,
                      int buttonWidth, int buttonHeight, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

  private:
    rapready::ThemePalette palette = rapready::themePalette(0);
};

class RapReadyOneAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::FileDragAndDropTarget,
                                               private juce::Timer
{
  public:
    explicit RapReadyOneAudioProcessorEditor(RapReadyOneAudioProcessor&);
    ~RapReadyOneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

  private:
    class HistoryKnob final : public juce::Slider
    {
      public:
        std::function<void()> onEditBegin;
        std::function<void()> onEditEnd;

        void mouseWheelMove(const juce::MouseEvent& event,
                            const juce::MouseWheelDetails& wheel) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;
        bool keyPressed(const juce::KeyPress& key) override;
    };

    void timerCallback() override;
    void applyTheme(int themeIndex);
    void toggleAdvanced();
    void browseForRecording();
    void startFileRender(const juce::File& inputFile);
    void cancelFileRender();
    void finishFileRender(const rapready::OfflineRenderResult& result);
    void revealLatestOutput();
    void setRenderStatus(const juce::String& text);
    static bool hasDropExtension(const juce::File& file);
    static float meterProportion(float decibels) noexcept;

    RapReadyOneAudioProcessor& audioProcessor;
    RapReadyLookAndFeel lookAndFeel;
    HistoryKnob amountKnob;
    juce::AudioProcessorValueTreeState::SliderAttachment amountAttachment;
    juce::ComboBox themeSelector;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment themeAttachment;
    juce::TextButton advancedButton{"ADVANCED +"};
    RapReadyAdvancedPanel advancedPanel;
    juce::Viewport advancedViewport;
    juce::TextButton browseButton{"BROWSE FILE"};
    juce::TextButton cancelButton{"CANCEL"};
    juce::TextButton revealButton{"OPEN FOLDER"};
    juce::Label renderStatus;
    double displayedProgress = 0.0;
    juce::ProgressBar progressBar{displayedProgress};
    juce::TooltipWindow tooltipWindow{this, 450};
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::jthread renderThread;
    std::atomic<bool> renderCancelRequested{false};
    std::atomic<bool> rendering{false};
    std::atomic<float> renderProgress{0.0f};
    juce::File latestOutput;
    juce::String selectedInputName;
    rapready::ThemePalette palette = rapready::themePalette(0);
    bool standaloneMode = false;
    bool advancedVisible = false;
    bool dragHighlighted = false;
    int appliedThemeIndex = -1;

    float inputDb = -100.0f;
    float outputDb = -100.0f;
    float reductionDb = 0.0f;
    float noiseFloorDb = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RapReadyOneAudioProcessorEditor)
};
