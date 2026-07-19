#pragma once

#include "Parameters.h"
#include "Theme.h"

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class RapReadyAdvancedPanel final : public juce::Component
{
  public:
    using Snapshot = rapready::parameters::AudioSnapshot;

    explicit RapReadyAdvancedPanel(juce::AudioProcessorValueTreeState& state);
    ~RapReadyAdvancedPanel() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void setTheme(const rapready::ThemePalette& newPalette);

    void beginExternalParameterEdit();
    void endExternalParameterEdit();
    [[nodiscard]] bool isListeningToPrevious() const noexcept { return listeningToPrevious; }
    [[nodiscard]] Snapshot getAudibleSnapshot() const;

    std::function<void(bool, const Snapshot&)> onAuditionChanged;
    std::function<void(bool)> onComparisonModeChanged;

  private:
    class DescribedSlider final : public juce::Slider
    {
      public:
        std::function<void()> onHover;
        std::function<void()> onEditBegin;
        std::function<void()> onEditEnd;
        void mouseEnter(const juce::MouseEvent& event) override
        {
            juce::Slider::mouseEnter(event);
            if (onHover)
                onHover();
        }
        void mouseWheelMove(const juce::MouseEvent& event,
                            const juce::MouseWheelDetails& wheel) override
        {
            if (onEditBegin)
                onEditBegin();
            juce::Slider::mouseWheelMove(event, wheel);
            if (onEditEnd)
                onEditEnd();
        }
        void mouseDoubleClick(const juce::MouseEvent& event) override
        {
            if (onEditBegin)
                onEditBegin();
            juce::Slider::mouseDoubleClick(event);
            if (onEditEnd)
                onEditEnd();
        }
        bool keyPressed(const juce::KeyPress& key) override
        {
            if (onEditBegin)
                onEditBegin();
            const auto handled = juce::Slider::keyPressed(key);
            if (onEditEnd)
                onEditEnd();
            return handled;
        }
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    void configureSlider(DescribedSlider& slider, const juce::String& title,
                         const juce::String& description, double defaultValue);
    [[nodiscard]] Snapshot readSnapshot() const;
    void applySnapshot(const Snapshot& snapshot);
    void beginParameterEdit();
    void endParameterEdit();
    void togglePrevious();
    void resetAdvanced();
    void updateComparisonUi();
    void showDescription(const juce::String& description);

    juce::AudioProcessorValueTreeState& parameters;
    std::array<std::unique_ptr<DescribedSlider>, rapready::parameters::stageIds.size()> stageSliders;
    std::array<std::unique_ptr<DescribedSlider>, rapready::parameters::eqIds.size()> eqSliders;
    std::array<std::unique_ptr<SliderAttachment>, rapready::parameters::stageIds.size()> stageAttachments;
    std::array<std::unique_ptr<SliderAttachment>, rapready::parameters::eqIds.size()> eqAttachments;
    std::array<juce::Rectangle<int>, rapready::parameters::stageIds.size()> stageBounds;
    std::array<juce::Rectangle<int>, rapready::parameters::eqIds.size()> eqBounds;

    juce::TextButton previousButton{"PREVIOUS"};
    juce::TextButton resetButton{"RESET"};
    juce::Label infoLabel;
    juce::TooltipWindow tooltipWindow{this, 450};

    rapready::ThemePalette palette = rapready::themePalette(0);
    Snapshot previousSnapshot{};
    Snapshot currentSnapshot{};
    Snapshot pendingSnapshot{};
    bool previousAvailable = false;
    bool listeningToPrevious = false;
    bool parameterEditInProgress = false;
    bool applyingSnapshot = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RapReadyAdvancedPanel)
};
