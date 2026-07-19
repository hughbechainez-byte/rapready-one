#include "PluginEditor.h"

#include <cmath>

namespace
{
const auto ink = juce::Colour::fromRGB(235, 241, 244);
const auto muted = juce::Colour::fromRGB(132, 151, 160);
const auto cyan = juce::Colour::fromRGB(57, 225, 203);
const auto coral = juce::Colour::fromRGB(255, 111, 87);
} // namespace

void RapReadyLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosition, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height))
                      .reduced(14.0f);
    const auto radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPosition * (rotaryEndAngle - rotaryStartAngle);

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour::fromRGB(33, 50, 57));
    g.strokePath(track,
                 juce::PathStrokeType(11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path active;
    active.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    juce::ColourGradient glow(cyan, centre.x - radius, centre.y, coral, centre.x + radius, centre.y, false);
    g.setGradientFill(glow);
    g.strokePath(active,
                 juce::PathStrokeType(11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour::fromRGB(17, 27, 32));
    g.fillEllipse(bounds.reduced(22.0f));
    g.setColour(juce::Colour::fromRGB(57, 75, 82));
    g.drawEllipse(bounds.reduced(22.0f), 1.5f);

    juce::Path pointer;
    const auto pointerLength = radius * 0.46f;
    const auto pointerThickness = 4.0f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -radius * 0.56f, pointerThickness, pointerLength,
                                2.0f);
    g.setColour(ink);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

RapReadyOneAudioProcessorEditor::RapReadyOneAudioProcessorEditor(RapReadyOneAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor),
      amountAttachment(processor.parameters, "amount", amountKnob)
{
    amountKnob.setLookAndFeel(&lookAndFeel);
    amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 92, 34);
    amountKnob.setColour(juce::Slider::textBoxTextColourId, ink);
    amountKnob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    amountKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    amountKnob.setDoubleClickReturnValue(true, 62.0);
    amountKnob.setTitle("Rap Ready amount");
    amountKnob.setDescription("Controls the intensity of the complete vocal cleanup chain");
    addAndMakeVisible(amountKnob);

    setResizable(true, true);
    setResizeLimits(420, 520, 760, 900);
    setSize(520, 640);
    startTimerHz(30);
}

RapReadyOneAudioProcessorEditor::~RapReadyOneAudioProcessorEditor()
{
    amountKnob.setLookAndFeel(nullptr);
}

float RapReadyOneAudioProcessorEditor::meterProportion(float decibels) noexcept
{
    return juce::jlimit(0.0f, 1.0f, (decibels + 60.0f) / 60.0f);
}

void RapReadyOneAudioProcessorEditor::timerCallback()
{
    inputDb = audioProcessor.getInputLevelDb();
    outputDb = audioProcessor.getOutputLevelDb();
    reductionDb = audioProcessor.getGainReductionDb();
    noiseFloorDb = audioProcessor.getNoiseFloorDb();
    repaint();
}

void RapReadyOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient background(juce::Colour::fromRGB(17, 28, 34), 0.0f, 0.0f,
                                    juce::Colour::fromRGB(7, 12, 16), bounds.getWidth(), bounds.getHeight(),
                                    false);
    g.setGradientFill(background);
    g.fillAll();

    g.setColour(cyan.withAlpha(0.09f));
    g.fillEllipse(bounds.getWidth() * 0.08f, bounds.getHeight() * 0.15f, bounds.getWidth() * 0.84f,
                  bounds.getWidth() * 0.84f);

    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    g.drawFittedText("RAPREADY ONE", 24, 20, getWidth() - 48, 38, juce::Justification::centred, 1);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.5f)));
    g.drawFittedText("ONE KNOB. MIX-READY VOCAL.", 24, 57, getWidth() - 48, 20, juce::Justification::centred,
                     1);

    const auto meterY = getHeight() - 155;
    const auto meterWidth = (getWidth() - 84) / 2;
    const auto drawMeter = [&](int left, float db, juce::Colour colour, const juce::String& label)
    {
        auto track = juce::Rectangle<float>(static_cast<float>(left), static_cast<float>(meterY),
                                            static_cast<float>(meterWidth), 8.0f);
        g.setColour(juce::Colour::fromRGB(30, 44, 50));
        g.fillRoundedRectangle(track, 4.0f);
        track.setWidth(track.getWidth() * meterProportion(db));
        g.setColour(colour);
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
        g.drawText(label + "  " + juce::String(db, 1) + " dB", left, meterY + 13, meterWidth, 18,
                   juce::Justification::centredLeft);
    };
    drawMeter(28, inputDb, cyan, "IN");
    drawMeter(56 + meterWidth, outputDb, coral, "OUT");

    const auto inputStatus = inputDb > -3.0f    ? "INPUT HOT - TURN DOWN"
                             : inputDb > -18.0f ? "INPUT READY"
                             : inputDb > -45.0f ? "INPUT LOW - ADD GAIN"
                                                : "WAITING FOR VOCAL";
    const auto statusColour = inputDb > -3.0f ? coral : inputDb > -18.0f ? cyan : muted;
    g.setColour(statusColour);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawFittedText(inputStatus, 24, meterY + 47, getWidth() - 48, 22, juce::Justification::centred, 1);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawFittedText("Aim for vocal peaks around -12 dBFS  |  GR " + juce::String(reductionDb, 1) +
                         " dB  |  Noise " + juce::String(noiseFloorDb, 0) + " dBFS",
                     20, meterY + 72, getWidth() - 40, 20, juce::Justification::centred, 1);
    g.drawFittedText("35 CLEAN       62 RAP DEFAULT       85 POLISH", 20, getHeight() - 42, getWidth() - 40,
                     18, juce::Justification::centred, 1);

    const std::array<juce::String, 7> stages{"FILTER", "EXPAND", "EQ", "COMP", "DE-ESS", "SAT", "LIMIT"};
    const auto stageWidth = (getWidth() - 40) / static_cast<int>(stages.size());
    const auto amount = static_cast<float>(amountKnob.getValue() / 100.0);
    for (std::size_t i = 0; i < stages.size(); ++i)
    {
        const auto x = 20 + static_cast<int>(i) * stageWidth;
        const auto on = amount > static_cast<float>(i) * 0.035f;
        g.setColour(on ? cyan.withAlpha(0.75f) : muted.withAlpha(0.45f));
        g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        g.drawFittedText(stages[i], x, 90, stageWidth, 18, juce::Justification::centred, 1);
    }
}

void RapReadyOneAudioProcessorEditor::resized()
{
    const auto side = std::min(getWidth() - 90, getHeight() - 300);
    amountKnob.setBounds((getWidth() - side) / 2, 118, side, side);
}
