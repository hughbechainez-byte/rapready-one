#include "PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr int mainPanelWidth = 520;
constexpr int pluginCollapsedHeight = 640;
constexpr int standaloneCollapsedHeight = 790;
constexpr int advancedWidth = 600;

rapready::AdvancedSettings advancedFromSnapshot(
    const rapready::parameters::AudioSnapshot& snapshot)
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

void drawTechCorner(juce::Graphics& g, juce::Rectangle<float> bounds, float length)
{
    juce::Path path;
    path.startNewSubPath(bounds.getX(), bounds.getY() + length);
    path.lineTo(bounds.getX(), bounds.getY());
    path.lineTo(bounds.getX() + length, bounds.getY());
    path.startNewSubPath(bounds.getRight() - length, bounds.getY());
    path.lineTo(bounds.getRight(), bounds.getY());
    path.lineTo(bounds.getRight(), bounds.getY() + length);
    path.startNewSubPath(bounds.getX(), bounds.getBottom() - length);
    path.lineTo(bounds.getX(), bounds.getBottom());
    path.lineTo(bounds.getX() + length, bounds.getBottom());
    path.startNewSubPath(bounds.getRight() - length, bounds.getBottom());
    path.lineTo(bounds.getRight(), bounds.getBottom());
    path.lineTo(bounds.getRight(), bounds.getBottom() - length);
    g.strokePath(path, juce::PathStrokeType(1.5f));
}
} // namespace

void RapReadyLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosition, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height))
                      .reduced(16.0f);
    const auto radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPosition * (rotaryEndAngle - rotaryStartAngle);

    for (int ring = 0; ring < 3; ++ring)
    {
        juce::Path glow;
        glow.addCentredArc(centre.x, centre.y, radius + static_cast<float>(ring * 4),
                           radius + static_cast<float>(ring * 4), 0.0f, rotaryStartAngle, angle, true);
        g.setColour(palette.base.withAlpha(0.18f - static_cast<float>(ring) * 0.045f));
        g.strokePath(glow, juce::PathStrokeType(17.0f + static_cast<float>(ring * 5),
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(palette.faint);
    g.strokePath(track, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    juce::Path active;
    active.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    juce::ColourGradient gradient(palette.dark, centre.x - radius, centre.y, palette.bright,
                                  centre.x + radius, centre.y, false);
    g.setGradientFill(gradient);
    g.strokePath(active, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    const auto face = bounds.reduced(23.0f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(face);
    g.setColour(palette.dark);
    g.drawEllipse(face, 2.0f);
    g.setColour(palette.faint);
    g.drawEllipse(face.reduced(8.0f), 1.0f);

    juce::Path pointer;
    const auto pointerLength = radius * 0.46f;
    pointer.addRoundedRectangle(-2.5f, -radius * 0.59f, 5.0f, pointerLength, 2.5f);
    g.setColour(palette.bright);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

void RapReadyLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosition, float minSliderPosition,
                                           float maxSliderPosition, juce::Slider::SliderStyle style,
                                           juce::Slider& slider)
{
    if (!slider.isVertical())
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPosition,
                                               minSliderPosition, maxSliderPosition, style, slider);
        return;
    }

    const auto centreX = static_cast<float>(x + width / 2);
    const auto top = std::min(minSliderPosition, maxSliderPosition);
    const auto bottom = std::max(minSliderPosition, maxSliderPosition);
    const auto enabledAlpha = slider.isEnabled() ? 1.0f : 0.34f;
    g.setColour(palette.faint.withMultipliedAlpha(enabledAlpha));
    g.fillRoundedRectangle(centreX - 4.0f, top, 8.0f, bottom - top, 4.0f);
    g.setColour(palette.base.withMultipliedAlpha(enabledAlpha));
    g.fillRoundedRectangle(centreX - 3.0f, sliderPosition, 6.0f,
                           std::max(1.0f, bottom - sliderPosition), 3.0f);
    g.setColour(palette.bright.withMultipliedAlpha(enabledAlpha));
    g.fillRoundedRectangle(centreX - 10.0f, sliderPosition - 4.0f, 20.0f, 8.0f, 3.0f);
    g.setColour(palette.dark.withMultipliedAlpha(enabledAlpha));
    g.drawRoundedRectangle(centreX - 12.0f, sliderPosition - 6.0f, 24.0f, 12.0f, 4.0f, 1.0f);
}

void RapReadyLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                               const juce::Colour&, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(down ? palette.dark : juce::Colours::black);
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour((highlighted ? palette.bright : palette.base)
                    .withAlpha(button.isEnabled() ? 0.92f : 0.28f));
    g.drawRoundedRectangle(bounds, 3.0f, down ? 2.0f : 1.0f);
}

void RapReadyLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool down,
                                       int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                         static_cast<float>(height))
                      .reduced(0.5f);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(down ? palette.bright : palette.base);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    juce::Path arrow;
    arrow.startNewSubPath(static_cast<float>(width - 20), static_cast<float>(height / 2 - 2));
    arrow.lineTo(static_cast<float>(width - 14), static_cast<float>(height / 2 + 4));
    arrow.lineTo(static_cast<float>(width - 8), static_cast<float>(height / 2 - 2));
    g.strokePath(arrow, juce::PathStrokeType(1.7f));
}

void RapReadyLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(9, 1, box.getWidth() - 32, box.getHeight() - 2);
    label.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
}

void RapReadyOneAudioProcessorEditor::HistoryKnob::mouseWheelMove(
    const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (onEditBegin)
        onEditBegin();
    juce::Slider::mouseWheelMove(event, wheel);
    if (onEditEnd)
        onEditEnd();
}

void RapReadyOneAudioProcessorEditor::HistoryKnob::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (onEditBegin)
        onEditBegin();
    juce::Slider::mouseDoubleClick(event);
    if (onEditEnd)
        onEditEnd();
}

bool RapReadyOneAudioProcessorEditor::HistoryKnob::keyPressed(const juce::KeyPress& key)
{
    if (onEditBegin)
        onEditBegin();
    const auto handled = juce::Slider::keyPressed(key);
    if (onEditEnd)
        onEditEnd();
    return handled;
}

RapReadyOneAudioProcessorEditor::RapReadyOneAudioProcessorEditor(
    RapReadyOneAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor),
      amountAttachment(processor.parameters, rapready::parameters::amount, amountKnob),
      themeAttachment(processor.parameters, rapready::parameters::theme, themeSelector),
      advancedPanel(processor.parameters)
{
    standaloneMode = audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
    setLookAndFeel(&lookAndFeel);

    amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 92, 32);
    amountKnob.setDoubleClickReturnValue(true, 62.0);
    amountKnob.setTitle("Rap Ready amount");
    amountKnob.setDescription("Controls the intensity of the complete vocal cleanup chain");
    amountKnob.setTooltip("0 keeps the source dry; 62 is the rap default; 100 applies the full guarded cleanup chain.");
    amountKnob.onDragStart = [this] { advancedPanel.beginExternalParameterEdit(); };
    amountKnob.onDragEnd = [this] { advancedPanel.endExternalParameterEdit(); };
    amountKnob.onEditBegin = [this] { advancedPanel.beginExternalParameterEdit(); };
    amountKnob.onEditEnd = [this] { advancedPanel.endExternalParameterEdit(); };
    addAndMakeVisible(amountKnob);

    themeSelector.addItemList(
        juce::StringArray{"CYAN", "VIOLET", "EMERALD", "AMBER", "ICE", "ROSE"}, 1);
    if (const auto* theme = processor.parameters.getRawParameterValue(rapready::parameters::theme))
        themeSelector.setSelectedItemIndex(juce::roundToInt(theme->load()),
                                           juce::dontSendNotification);
    themeSelector.setTooltip("Changes the interface hue; audio is unchanged and the choice is saved with the plug-in state.");
    themeSelector.onChange = [this] { applyTheme(themeSelector.getSelectedItemIndex()); };
    addAndMakeVisible(themeSelector);

    advancedButton.setTooltip("Show fine controls for every cleanup stage and the 10-band vocal EQ");
    advancedButton.onClick = [this] { toggleAdvanced(); };
    addAndMakeVisible(advancedButton);

    advancedViewport.setViewedComponent(&advancedPanel, false);
    advancedViewport.setScrollBarsShown(true, true);
    advancedViewport.setScrollBarThickness(10);
    advancedViewport.setVisible(false);
    addAndMakeVisible(advancedViewport);
    advancedPanel.onAuditionChanged = [this](bool listening, const RapReadyAdvancedPanel::Snapshot& snapshot)
    {
        if (listening)
            audioProcessor.setAuditionSnapshot(snapshot);
        else
            audioProcessor.clearAuditionSnapshot();
    };
    advancedPanel.onComparisonModeChanged = [this](bool listening)
    {
        amountKnob.setEnabled(!listening);
    };

    browseButton.setTooltip("Choose one recording and save a cleaned 24-bit WAV beside it");
    browseButton.onClick = [this] { browseForRecording(); };
    cancelButton.setTooltip("Stop the current file cleanup without leaving a partial output");
    cancelButton.onClick = [this] { cancelFileRender(); };
    revealButton.setTooltip("Show the most recently cleaned WAV in its folder");
    revealButton.onClick = [this] { revealLatestOutput(); };
    cancelButton.setEnabled(false);
    revealButton.setEnabled(false);
    for (auto* button : {&browseButton, &cancelButton, &revealButton})
    {
        button->setVisible(standaloneMode);
        addAndMakeVisible(*button);
    }

    renderStatus.setJustificationType(juce::Justification::centredLeft);
    renderStatus.setMinimumHorizontalScale(0.7f);
    renderStatus.setText("READY // drop one recording anywhere on this window",
                         juce::dontSendNotification);
    renderStatus.setVisible(standaloneMode);
    addAndMakeVisible(renderStatus);
    progressBar.setPercentageDisplay(false);
    progressBar.setVisible(standaloneMode);
    addAndMakeVisible(progressBar);

    setResizable(true, true);
    setResizeLimits(mainPanelWidth, standaloneMode ? 760 : 600, 1500, 1000);
    setSize(mainPanelWidth, standaloneMode ? standaloneCollapsedHeight : pluginCollapsedHeight);
    applyTheme(themeSelector.getSelectedItemIndex());
    startTimerHz(30);
}

RapReadyOneAudioProcessorEditor::~RapReadyOneAudioProcessorEditor()
{
    fileChooser.reset();
    renderCancelRequested.store(true);
    if (renderThread.joinable())
    {
        renderThread.request_stop();
        renderThread.join();
    }
    audioProcessor.clearAuditionSnapshot();
    advancedPanel.onAuditionChanged = nullptr;
    advancedPanel.onComparisonModeChanged = nullptr;
    setLookAndFeel(nullptr);
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
    displayedProgress = static_cast<double>(renderProgress.load());

    if (const auto* theme = audioProcessor.parameters.getRawParameterValue(rapready::parameters::theme))
    {
        const auto index = juce::roundToInt(theme->load());
        if (index != appliedThemeIndex)
            applyTheme(index);
    }
    repaint();
}

void RapReadyOneAudioProcessorEditor::applyTheme(int themeIndex)
{
    appliedThemeIndex = juce::jlimit(0, 5, themeIndex);
    palette = rapready::themePalette(appliedThemeIndex);
    lookAndFeel.setPalette(palette);
    advancedPanel.setTheme(palette);

    amountKnob.setColour(juce::Slider::textBoxTextColourId, palette.text);
    amountKnob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
    amountKnob.setColour(juce::Slider::textBoxOutlineColourId, palette.dark);
    themeSelector.setColour(juce::ComboBox::textColourId, palette.text);
    themeSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
    themeSelector.setColour(juce::ComboBox::outlineColourId, palette.base);
    themeSelector.setColour(juce::ComboBox::arrowColourId, palette.bright);
    renderStatus.setColour(juce::Label::textColourId, palette.text);
    progressBar.setColour(juce::ProgressBar::foregroundColourId, palette.base);
    progressBar.setColour(juce::ProgressBar::backgroundColourId, palette.faint);
    for (auto* button : {&advancedButton, &browseButton, &cancelButton, &revealButton})
    {
        button->setColour(juce::TextButton::buttonColourId, juce::Colours::black);
        button->setColour(juce::TextButton::buttonOnColourId, palette.dark);
        button->setColour(juce::TextButton::textColourOffId, palette.text);
        button->setColour(juce::TextButton::textColourOnId, palette.bright);
    }
    repaint();
}

void RapReadyOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    const auto mainBounds = juce::Rectangle<int>(0, 0, std::min(mainPanelWidth, getWidth()), getHeight());
    g.saveState();
    g.reduceClipRegion(mainBounds);

    g.setColour(palette.faint.withAlpha(0.55f));
    for (int x = 0; x < mainPanelWidth; x += 26)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(mainBounds.getHeight()));
    for (int y = 0; y < mainBounds.getHeight(); y += 26)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(mainPanelWidth));

    auto frame = juce::Rectangle<float>(10.0f, 10.0f, static_cast<float>(mainPanelWidth - 20),
                                        static_cast<float>(mainBounds.getHeight() - 20));
    g.setColour(palette.dark);
    g.drawRect(frame, 1.0f);
    g.setColour(palette.base);
    drawTechCorner(g, frame, 22.0f);

    g.setColour(palette.text);
    g.setFont(juce::Font(juce::FontOptions(29.0f, juce::Font::bold)));
    g.drawFittedText("RAPREADY ONE", 24, 14, mainPanelWidth - 48, 38,
                     juce::Justification::centred, 1);
    g.setColour(palette.muted);
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    g.drawFittedText("VOCAL CLEANUP CONTROL SYSTEM // V0.2", 24, 48, mainPanelWidth - 48, 16,
                     juce::Justification::centred, 1);
    g.drawText("THEME", 330, 69, 58, 24, juce::Justification::centredRight);

    const std::array<juce::String, 7> stages{"FILTER", "EXPAND", "EQ", "COMP", "DE-ESS", "SAT", "LIMIT"};
    const auto stageWidth = 480 / static_cast<int>(stages.size());
    const auto amount = static_cast<float>(amountKnob.getValue() / 100.0);
    for (std::size_t i = 0; i < stages.size(); ++i)
    {
        const auto x = 20 + static_cast<int>(i) * stageWidth;
        const auto on = amount > static_cast<float>(i) * 0.035f;
        g.setColour(on ? palette.bright.withAlpha(0.85f) : palette.muted.withAlpha(0.38f));
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.drawFittedText(stages[i], x, 102, stageWidth, 18, juce::Justification::centred, 1);
        g.fillEllipse(static_cast<float>(x + stageWidth / 2 - 2), 121.0f, 4.0f, 4.0f);
    }

    const auto meterY = 468;
    const auto meterWidth = 218;
    const auto drawMeter = [&](int left, float db, bool brighter, const juce::String& label)
    {
        auto track = juce::Rectangle<float>(static_cast<float>(left), static_cast<float>(meterY),
                                            static_cast<float>(meterWidth), 8.0f);
        g.setColour(palette.faint);
        g.fillRoundedRectangle(track, 3.0f);
        track.setWidth(track.getWidth() * meterProportion(db));
        g.setColour(brighter ? palette.bright : palette.base);
        g.fillRoundedRectangle(track, 3.0f);
        g.setColour(palette.muted);
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        g.drawText(label + "  " + juce::String(db, 1) + " dB", left, meterY + 12, meterWidth, 17,
                   juce::Justification::centredLeft);
    };
    drawMeter(30, inputDb, false, "INPUT");
    drawMeter(272, outputDb, true, "OUTPUT");

    const auto inputStatus = inputDb > -3.0f    ? "INPUT HOT // TURN DOWN"
                             : inputDb > -18.0f ? "INPUT READY"
                             : inputDb > -45.0f ? "INPUT LOW // ADD GAIN"
                                                : "WAITING FOR VOCAL";
    g.setColour(inputDb > -3.0f ? palette.bright : inputDb > -18.0f ? palette.base : palette.muted);
    g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
    g.drawFittedText(inputStatus, 24, 508, mainPanelWidth - 48, 20,
                     juce::Justification::centred, 1);
    g.setColour(palette.muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawFittedText("PEAK NEAR -12 dBFS  //  GR " + juce::String(reductionDb, 1) +
                         " dB  //  NOISE " + juce::String(noiseFloorDb, 0) + " dBFS",
                     20, 532, mainPanelWidth - 40, 18, juce::Justification::centred, 1);
    g.setColour(palette.text);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawFittedText("35 CLEAN       62 RAP DEFAULT       85 POLISH", 20, 568,
                     mainPanelWidth - 40, 18, juce::Justification::centred, 1);

    if (standaloneMode)
    {
        auto drop = juce::Rectangle<float>(20.0f, 612.0f, 480.0f,
                                           static_cast<float>(std::max(150, getHeight() - 632)));
        g.setColour(dragHighlighted ? palette.dark.brighter(0.2f) : juce::Colours::black);
        g.fillRoundedRectangle(drop, 5.0f);
        g.setColour(dragHighlighted ? palette.bright : palette.base);
        const float dashLengths[] = {7.0f, 5.0f};
        juce::Path dropPath;
        dropPath.addRoundedRectangle(drop, 5.0f);
        juce::Path dashed;
        juce::PathStrokeType(1.3f).createDashedStroke(dashed, dropPath, dashLengths, 2);
        g.strokePath(dashed, juce::PathStrokeType(1.0f));
        g.setColour(palette.text);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        g.drawText(dragHighlighted ? "RELEASE TO CLEAN" : "DROP A VOCAL RECORDING", 30, 620, 460, 24,
                   juce::Justification::centred);
        g.setColour(palette.muted);
        g.setFont(juce::Font(juce::FontOptions(9.7f, juce::Font::bold)));
        g.drawText("WAV  //  AIFF  //  FLAC  //  OGG  //  MP3    ->    24-BIT WAV",
                   30, 642, 460, 18, juce::Justification::centred);
        if (selectedInputName.isNotEmpty())
            g.drawFittedText(selectedInputName, 30, 679, 460, 16,
                             juce::Justification::centred, 1);
    }
    g.restoreState();
}

void RapReadyOneAudioProcessorEditor::resized()
{
    amountKnob.setBounds(98, 126, 324, 324);
    advancedButton.setBounds(28, 68, 132, 26);
    themeSelector.setBounds(393, 68, 98, 26);

    if (standaloneMode)
    {
        browseButton.setBounds(30, 662, 130, 30);
        cancelButton.setBounds(170, 662, 110, 30);
        revealButton.setBounds(290, 662, 200, 30);
        progressBar.setBounds(30, 705, 460, 8);
        renderStatus.setBounds(30, 718, 460, 45);
    }

    if (advancedVisible)
    {
        const auto x = std::min(mainPanelWidth, getWidth());
        advancedViewport.setBounds(x, 0, std::max(0, getWidth() - x), getHeight());
        advancedPanel.setSize(advancedWidth, std::max(760, getHeight()));
    }
}

void RapReadyOneAudioProcessorEditor::toggleAdvanced()
{
    advancedVisible = !advancedVisible;
    advancedButton.setButtonText(advancedVisible ? "ADVANCED -" : "ADVANCED +");
    advancedViewport.setVisible(advancedVisible);
    if (advancedVisible)
    {
        setResizeLimits(900, standaloneMode ? 760 : 640, 1500, 1000);
        setSize(mainPanelWidth + advancedWidth, standaloneMode ? 820 : 760);
    }
    else
    {
        setResizeLimits(mainPanelWidth, standaloneMode ? 760 : 600, 1500, 1000);
        setSize(mainPanelWidth, standaloneMode ? standaloneCollapsedHeight : pluginCollapsedHeight);
    }
    resized();
    repaint();
}

bool RapReadyOneAudioProcessorEditor::hasDropExtension(const juce::File& file)
{
    const auto extension = file.getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".aif" || extension == ".aiff" ||
           extension == ".flac" || extension == ".ogg" || extension == ".mp3";
}

bool RapReadyOneAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    return standaloneMode && !rendering.load() && files.size() == 1 &&
           hasDropExtension(juce::File(files[0]));
}

void RapReadyOneAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int, int)
{
    dragHighlighted = isInterestedInFileDrag(files);
    repaint();
}

void RapReadyOneAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
    dragHighlighted = false;
    repaint();
}

void RapReadyOneAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    dragHighlighted = false;
    repaint();
    if (!standaloneMode || files.size() != 1)
    {
        setRenderStatus("DROP REJECTED // choose exactly one supported recording");
        return;
    }
    startFileRender(juce::File(files[0]));
}

void RapReadyOneAudioProcessorEditor::browseForRecording()
{
    if (!standaloneMode || rendering.load())
        return;
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a vocal recording", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3", true);
    const auto safeThis = juce::Component::SafePointer<RapReadyOneAudioProcessorEditor>(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
                             [safeThis](const juce::FileChooser& chooser)
                             {
                                 if (safeThis == nullptr)
                                     return;
                                 const auto result = chooser.getResult();
                                 if (result.existsAsFile())
                                     safeThis->startFileRender(result);
                             });
}

void RapReadyOneAudioProcessorEditor::startFileRender(const juce::File& inputFile)
{
    if (rendering.load())
    {
        setRenderStatus("BUSY // cancel or wait for the current cleanup");
        return;
    }
    if (!inputFile.existsAsFile() || !hasDropExtension(inputFile) ||
        !rapready::hasSupportedAudioExtension(inputFile))
    {
        setRenderStatus("UNSUPPORTED // use WAV, AIFF, FLAC, OGG, or MP3");
        return;
    }
    if (renderThread.joinable())
        renderThread.join();

    rapready::OfflineRenderRequest request;
    request.inputFile = inputFile;
    request.outputFile = rapready::makeUniqueOutputFile(inputFile);
    const auto audibleSnapshot = advancedPanel.getAudibleSnapshot();
    request.amount = audibleSnapshot[0];
    request.advanced = advancedFromSnapshot(audibleSnapshot);

    selectedInputName = inputFile.getFileName();
    latestOutput = {};
    renderCancelRequested.store(false);
    renderProgress.store(0.0f);
    rendering.store(true);
    browseButton.setEnabled(false);
    cancelButton.setEnabled(true);
    revealButton.setEnabled(false);
    setRenderStatus("CLEANING // " + inputFile.getFileName());
    repaint();

    const auto safeThis = juce::Component::SafePointer<RapReadyOneAudioProcessorEditor>(this);
    renderThread = std::jthread([this, safeThis, request](std::stop_token stopToken)
    {
        const auto result = rapready::renderAudioFile(
            request,
            [this, &stopToken]
            {
                return stopToken.stop_requested() || renderCancelRequested.load();
            },
            [this](float progress) { renderProgress.store(progress); });
        juce::MessageManager::callAsync([safeThis, result]
        {
            if (safeThis != nullptr)
                safeThis->finishFileRender(result);
        });
    });
}

void RapReadyOneAudioProcessorEditor::cancelFileRender()
{
    if (!rendering.load())
        return;
    renderCancelRequested.store(true);
    if (renderThread.joinable())
        renderThread.request_stop();
    cancelButton.setEnabled(false);
    setRenderStatus("CANCELLING // partial output will be discarded");
}

void RapReadyOneAudioProcessorEditor::finishFileRender(const rapready::OfflineRenderResult& result)
{
    rendering.store(false);
    browseButton.setEnabled(true);
    cancelButton.setEnabled(false);
    if (result.wasSuccessful())
    {
        latestOutput = result.outputFile;
        revealButton.setEnabled(true);
        renderProgress.store(1.0f);
        setRenderStatus("SAVED // " + result.outputFile.getFullPathName());
    }
    else
    {
        revealButton.setEnabled(false);
        renderProgress.store(0.0f);
        setRenderStatus(result.status == rapready::OfflineRenderStatus::cancelled
                            ? "CANCELLED // source unchanged; no partial output kept"
                            : "ERROR // " + result.message);
    }
    repaint();
}

void RapReadyOneAudioProcessorEditor::revealLatestOutput()
{
    if (latestOutput.existsAsFile())
        latestOutput.revealToUser();
}

void RapReadyOneAudioProcessorEditor::setRenderStatus(const juce::String& text)
{
    renderStatus.setText(text, juce::dontSendNotification);
}
