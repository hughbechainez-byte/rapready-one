#pragma once

#include "Dsp/RapReadyDSP.h"

#include <functional>
#include <juce_audio_formats/juce_audio_formats.h>

namespace rapready
{
enum class OfflineRenderStatus
{
    succeeded,
    cancelled,
    failed,
};

struct OfflineRenderRequest
{
    juce::File inputFile;
    juce::File outputFile;
    float amount = 62.0f;
    AdvancedSettings advanced;
    bool replaceExisting = false;
};

struct OfflineRenderResult
{
    OfflineRenderStatus status = OfflineRenderStatus::failed;
    juce::File outputFile;
    juce::String message;

    [[nodiscard]] bool wasSuccessful() const noexcept
    {
        return status == OfflineRenderStatus::succeeded;
    }
};

using CancelProbe = std::function<bool()>;
using ProgressSink = std::function<void(float)>;

[[nodiscard]] bool hasSupportedAudioExtension(const juce::File& file);
[[nodiscard]] juce::File makeUniqueOutputFile(const juce::File& inputFile);
[[nodiscard]] OfflineRenderResult renderAudioFile(const OfflineRenderRequest& request,
                                                  CancelProbe shouldCancel = {},
                                                  ProgressSink progress = {});
} // namespace rapready
