#include "AdvancedPanel.h"

#include <cmath>

namespace
{
const std::array<juce::String, 6> stageNames{
    "FILTER", "EXPAND", "COMP", "DE-ESS", "SAT", "LIMIT",
};

const std::array<juce::String, 6> stageLowHigh{
    "LOW fuller / HIGH tighter",    "LOW natural / HIGH quieter",
    "LOW dynamic / HIGH dense",     "LOW crisp / HIGH softer",
    "LOW clean / HIGH warmer",      "LOW headroom / HIGH firmer",
};

const std::array<juce::String, 6> stageDescriptions{
    "Filter — Low keeps more lows and air. High raises the rumble cut and lowers the top filter for tighter cleanup.",
    "Expansion — Low leaves breaths and room tone. High suppresses more sound between lines and can trim quiet endings.",
    "Compression — Low preserves level movement. High uses stronger serial peak/body control for a denser vocal.",
    "De-ess — Low leaves consonants bright. High applies more reduction to harsh S, SH, CH and Z sounds.",
    "Saturation — Low stays clean. High adds more soft harmonic color and rounds peaks without hard clipping.",
    "Limiter — Low uses an open safety ceiling. High lowers the ceiling for firmer peak containment.",
};

const std::array<juce::String, 10> eqNames{
    "80", "160", "315", "630", "1.25K", "2.5K", "4K", "6.3K", "10K", "16K",
};

const std::array<juce::String, 10> eqRoles{
    "RUMBLE", "WARMTH", "MUD", "BOX", "FORWARD", "CLARITY", "PRESENCE", "SIBILANCE", "AIR", "SHEEN",
};

const std::array<juce::String, 10> eqDescriptions{
    "80 Hz — Negative cuts rumble and deep weight; positive adds it. Too much boost can shake the room.",
    "160 Hz — Negative cuts chest and warmth; positive adds it. Too much can crowd the beat.",
    "315 Hz — Negative clears mud; positive adds body. Bedroom rooms often build up here.",
    "630 Hz — Negative cuts boxiness; positive adds thickness. Too much can sound closed-in.",
    "1.25 kHz — Negative softens nasal tone; positive pushes tone forward and can honk.",
    "2.5 kHz — Negative relaxes articulation; positive clarifies words and can become biting.",
    "4 kHz — Negative softens presence; positive improves intelligibility. Prefer 1–2 dB moves.",
    "6.3 kHz — Negative tames consonant edge; positive adds crispness and can increase sibilance.",
    "10 kHz — Negative reduces brightness or hiss; positive adds brightness and may reveal noise.",
    "16 kHz — Negative reduces air/noise; positive adds openness when the recording supports it.",
};
} // namespace

RapReadyAdvancedPanel::RapReadyAdvancedPanel(juce::AudioProcessorValueTreeState& state)
    : parameters(state)
{
    for (std::size_t i = 0; i < stageSliders.size(); ++i)
    {
        stageSliders[i] = std::make_unique<DescribedSlider>();
        configureSlider(*stageSliders[i], stageNames[i], stageDescriptions[i], 50.0);
        stageAttachments[i] = std::make_unique<SliderAttachment>(
            parameters, rapready::parameters::stageIds[i], *stageSliders[i]);
        addAndMakeVisible(*stageSliders[i]);
    }

    for (std::size_t i = 0; i < eqSliders.size(); ++i)
    {
        eqSliders[i] = std::make_unique<DescribedSlider>();
        configureSlider(*eqSliders[i], "EQ " + eqNames[i], eqDescriptions[i], 0.0);
        eqAttachments[i] = std::make_unique<SliderAttachment>(
            parameters, rapready::parameters::eqIds[i], *eqSliders[i]);
        eqSliders[i]->textFromValueFunction = [](double value)
        {
            return juce::String(value, 1);
        };
        eqSliders[i]->valueFromTextFunction = [](const juce::String& text)
        {
            return text.getDoubleValue();
        };
        addAndMakeVisible(*eqSliders[i]);
    }

    previousButton.setEnabled(false);
    previousButton.setTooltip("Compare the edited setup with the full setup captured before the last slider move");
    previousButton.onClick = [this] { togglePrevious(); };
    addAndMakeVisible(previousButton);

    resetButton.setTooltip("Return every Advanced stage to 50 and every EQ band to 0 dB");
    resetButton.onClick = [this] { resetAdvanced(); };
    addAndMakeVisible(resetButton);

    infoLabel.setJustificationType(juce::Justification::centredLeft);
    infoLabel.setMinimumHorizontalScale(0.72f);
    infoLabel.setFont(juce::Font(juce::FontOptions(11.5f)));
    infoLabel.setText("Move a control for a plain-language LOW/HIGH explanation.", juce::dontSendNotification);
    addAndMakeVisible(infoLabel);

    currentSnapshot = readSnapshot();
    setTheme(palette);
}

void RapReadyAdvancedPanel::configureSlider(DescribedSlider& slider, const juce::String& title,
                                             const juce::String& description, double defaultValue)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 48, 23);
    slider.setPopupDisplayEnabled(true, false, this);
    slider.setDoubleClickReturnValue(true, defaultValue);
    slider.setTitle(title);
    slider.setDescription(description);
    slider.setTooltip(description);
    slider.onHover = [this, description] { showDescription(description); };
    slider.onEditBegin = [this] { beginParameterEdit(); };
    slider.onEditEnd = [this] { endParameterEdit(); };
    slider.onDragStart = [this]
    {
        beginParameterEdit();
    };
    slider.onDragEnd = [this]
    {
        endParameterEdit();
    };
}

RapReadyAdvancedPanel::Snapshot RapReadyAdvancedPanel::readSnapshot() const
{
    Snapshot snapshot{};
    for (std::size_t i = 0; i < snapshot.size(); ++i)
        if (const auto* value = parameters.getRawParameterValue(rapready::parameters::audioStateIds[i]))
            snapshot[i] = value->load();
    return snapshot;
}

void RapReadyAdvancedPanel::applySnapshot(const Snapshot& snapshot)
{
    applyingSnapshot = true;
    for (std::size_t i = 0; i < snapshot.size(); ++i)
    {
        const auto* current = parameters.getRawParameterValue(
            rapready::parameters::audioStateIds[i]);
        if (current != nullptr && std::abs(current->load() - snapshot[i]) <= 0.0001f)
            continue;
        if (auto* parameter = parameters.getParameter(rapready::parameters::audioStateIds[i]))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(snapshot[i]));
            parameter->endChangeGesture();
        }
    }
    applyingSnapshot = false;
}

RapReadyAdvancedPanel::Snapshot RapReadyAdvancedPanel::getAudibleSnapshot() const
{
    return listeningToPrevious ? previousSnapshot : readSnapshot();
}

void RapReadyAdvancedPanel::beginParameterEdit()
{
    if (applyingSnapshot || listeningToPrevious || parameterEditInProgress)
        return;
    pendingSnapshot = readSnapshot();
    parameterEditInProgress = true;
    updateComparisonUi();
}

void RapReadyAdvancedPanel::endParameterEdit()
{
    if (!parameterEditInProgress)
        return;
    const auto editedSnapshot = readSnapshot();
    const auto changed = [&]
    {
        for (std::size_t i = 0; i < editedSnapshot.size(); ++i)
            if (std::abs(editedSnapshot[i] - pendingSnapshot[i]) > 0.0001f)
                return true;
        return false;
    }();
    if (changed)
    {
        previousSnapshot = pendingSnapshot;
        currentSnapshot = editedSnapshot;
        previousAvailable = true;
    }
    parameterEditInProgress = false;
    updateComparisonUi();
}

void RapReadyAdvancedPanel::beginExternalParameterEdit()
{
    beginParameterEdit();
}

void RapReadyAdvancedPanel::endExternalParameterEdit()
{
    endParameterEdit();
}

void RapReadyAdvancedPanel::togglePrevious()
{
    if (!previousAvailable || parameterEditInProgress)
        return;

    if (!listeningToPrevious)
    {
        currentSnapshot = readSnapshot();
        listeningToPrevious = true;
        if (onAuditionChanged)
            onAuditionChanged(true, previousSnapshot);
        showDescription("Listening to PREVIOUS. Click CURRENT to restore the setup you just edited.");
    }
    else
    {
        listeningToPrevious = false;
        if (onAuditionChanged)
            onAuditionChanged(false, currentSnapshot);
        showDescription("Listening to CURRENT. PREVIOUS returns to the setup captured before the last edit.");
    }
    updateComparisonUi();
}

void RapReadyAdvancedPanel::resetAdvanced()
{
    if (listeningToPrevious)
        return;
    beginParameterEdit();
    Snapshot reset = readSnapshot();
    for (std::size_t i = 1; i <= rapready::parameters::stageIds.size(); ++i)
        reset[i] = 50.0f;
    for (std::size_t i = 1 + rapready::parameters::stageIds.size(); i < reset.size(); ++i)
        reset[i] = 0.0f;
    applySnapshot(reset);
    endParameterEdit();
    showDescription("Advanced reset: every stage is at the research-backed 50 default and every EQ band is flat.");
}

void RapReadyAdvancedPanel::updateComparisonUi()
{
    previousButton.setEnabled(previousAvailable && !parameterEditInProgress);
    previousButton.setButtonText(listeningToPrevious ? "CURRENT" : "PREVIOUS");
    resetButton.setEnabled(!listeningToPrevious);
    for (auto& slider : stageSliders)
        slider->setEnabled(!listeningToPrevious);
    for (auto& slider : eqSliders)
        slider->setEnabled(!listeningToPrevious);
    if (onComparisonModeChanged)
        onComparisonModeChanged(listeningToPrevious);
    repaint();
}

void RapReadyAdvancedPanel::showDescription(const juce::String& description)
{
    infoLabel.setText(description, juce::dontSendNotification);
}

void RapReadyAdvancedPanel::setTheme(const rapready::ThemePalette& newPalette)
{
    palette = newPalette;
    infoLabel.setColour(juce::Label::textColourId, palette.text);
    for (auto* button : {&previousButton, &resetButton})
    {
        button->setColour(juce::TextButton::buttonColourId, juce::Colours::black);
        button->setColour(juce::TextButton::buttonOnColourId, palette.dark);
        button->setColour(juce::TextButton::textColourOffId, palette.text);
        button->setColour(juce::TextButton::textColourOnId, palette.bright);
    }
    const auto colourSlider = [this](juce::Slider& slider)
    {
        slider.setColour(juce::Slider::trackColourId, palette.base);
        slider.setColour(juce::Slider::thumbColourId, palette.bright);
        slider.setColour(juce::Slider::backgroundColourId, palette.faint);
        slider.setColour(juce::Slider::textBoxTextColourId, palette.text);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
        slider.setColour(juce::Slider::textBoxOutlineColourId, palette.dark);
    };
    for (auto& slider : stageSliders)
        colourSlider(*slider);
    for (auto& slider : eqSliders)
        colourSlider(*slider);
    repaint();
}

void RapReadyAdvancedPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(palette.faint);
    for (int x = 0; x < getWidth(); x += 28)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
    for (int y = 0; y < getHeight(); y += 28)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));

    g.setColour(palette.dark);
    g.drawRect(getLocalBounds().reduced(1), 1);
    g.drawVerticalLine(1, 0.0f, static_cast<float>(getHeight()));

    g.setColour(palette.text);
    g.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    g.drawFittedText("ADVANCED // CONTROL", 18, 9, getWidth() - 216, 28,
                     juce::Justification::centredLeft, 1);

    g.setColour(palette.base);
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    g.drawFittedText("STAGE MACROS  //  50 = RESEARCH DEFAULT", 14, 98, getWidth() - 28, 20,
                     juce::Justification::centredLeft, 1);
    g.drawFittedText("10-BAND VOCAL EQ  //  LOW CUTS  •  HIGH BOOSTS  •  DOUBLE-CLICK = FLAT", 14,
                     eqBounds[0].getY() - 29, getWidth() - 28, 20,
                     juce::Justification::centredLeft, 1);

    for (std::size_t i = 0; i < stageBounds.size(); ++i)
    {
        const auto bounds = stageBounds[i];
        g.setColour(palette.text);
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        g.drawText(stageNames[i], bounds.getX() - 8, bounds.getY() - 22, bounds.getWidth() + 16, 18,
                   juce::Justification::centred);
        g.setColour(palette.muted);
        g.setFont(juce::Font(juce::FontOptions(8.5f)));
        g.drawFittedText(stageLowHigh[i], bounds.getX() - 12, bounds.getBottom() + 2,
                         bounds.getWidth() + 24, 24, juce::Justification::centred, 2);
    }

    for (std::size_t i = 0; i < eqBounds.size(); ++i)
    {
        const auto bounds = eqBounds[i];
        g.setColour(palette.text);
        g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        g.drawText(eqNames[i], bounds.getX() - 4, bounds.getY() - 20, bounds.getWidth() + 8, 17,
                   juce::Justification::centred);
        g.setColour(palette.base);
        g.setFont(juce::Font(juce::FontOptions(7.5f, juce::Font::bold)));
        g.drawText("+6 ADD", bounds.getX() - 3, bounds.getY() + 1, bounds.getWidth() + 6, 13,
                   juce::Justification::centred);
        g.drawText("-6 CUT", bounds.getX() - 3, bounds.getBottom() - 46, bounds.getWidth() + 6, 13,
                   juce::Justification::centred);
        g.setColour(palette.muted);
        g.setFont(juce::Font(juce::FontOptions(7.6f, juce::Font::bold)));
        g.drawFittedText(eqRoles[i], bounds.getX() - 5, bounds.getBottom() + 1, bounds.getWidth() + 10,
                         18, juce::Justification::centred, 1);
    }
}

void RapReadyAdvancedPanel::resized()
{
    previousButton.setBounds(getWidth() - 190, 10, 86, 27);
    resetButton.setBounds(getWidth() - 94, 10, 80, 27);
    infoLabel.setBounds(14, 44, getWidth() - 28, 47);

    const auto stageTop = 142;
    const auto stageHeight = juce::jlimit(135, 180, (getHeight() - 210) * 42 / 100);
    const auto stageCell = (getWidth() - 20) / static_cast<int>(stageSliders.size());
    for (std::size_t i = 0; i < stageSliders.size(); ++i)
    {
        const auto centre = 10 + static_cast<int>(i) * stageCell + stageCell / 2;
        stageBounds[i] = {centre - 24, stageTop, 48, stageHeight};
        stageSliders[i]->setBounds(stageBounds[i]);
    }

    const auto eqTop = stageTop + stageHeight + 62;
    const auto eqHeight = juce::jmax(120, getHeight() - eqTop - 42);
    const auto eqCell = (getWidth() - 16) / static_cast<int>(eqSliders.size());
    for (std::size_t i = 0; i < eqSliders.size(); ++i)
    {
        const auto centre = 8 + static_cast<int>(i) * eqCell + eqCell / 2;
        eqBounds[i] = {centre - 17, eqTop, 34, eqHeight};
        eqSliders[i]->setBounds(eqBounds[i]);
    }
}
