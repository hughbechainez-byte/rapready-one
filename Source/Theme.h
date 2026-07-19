#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

namespace rapready
{
struct ThemePalette
{
    juce::Colour base;
    juce::Colour bright;
    juce::Colour text;
    juce::Colour muted;
    juce::Colour dark;
    juce::Colour faint;
};

inline ThemePalette themePalette(int index)
{
    constexpr std::array<std::uint32_t, 6> colours{
        0xff39e1cb, 0xffa970ff, 0xff42e68b, 0xffffc857, 0xff72c7ff, 0xffff6f9f,
    };
    const auto base = juce::Colour(colours[static_cast<std::size_t>(juce::jlimit(0, 5, index))]);
    return {
        base,
        base.brighter(0.38f),
        base.withMultipliedSaturation(0.62f).brighter(0.55f),
        base.withMultipliedSaturation(0.55f).withMultipliedBrightness(0.66f),
        base.darker(0.72f),
        base.darker(0.86f),
    };
}
} // namespace rapready
