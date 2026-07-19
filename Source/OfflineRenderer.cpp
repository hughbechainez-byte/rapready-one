#include "OfflineRenderer.h"

#include <algorithm>
#include <cmath>

namespace rapready
{
namespace
{
OfflineRenderResult failed(const juce::File& output, const juce::String& message)
{
    return {OfflineRenderStatus::failed, output, message};
}

OfflineRenderResult cancelled(const juce::File& output)
{
    return {OfflineRenderStatus::cancelled, output, "Cleaning cancelled"};
}
} // namespace

bool hasSupportedAudioExtension(const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const auto extension = file.getFileExtension();
    for (int i = 0; i < formats.getNumKnownFormats(); ++i)
        if (auto* format = formats.getKnownFormat(i))
            if (format->getFileExtensions().contains(extension, true) ||
                format->getFileExtensions().contains(extension.trimCharactersAtStart("."), true))
                return true;
    return false;
}

juce::File makeUniqueOutputFile(const juce::File& inputFile)
{
    const auto candidate = inputFile.getSiblingFile(inputFile.getFileNameWithoutExtension() + "-RapReady.wav");
    return candidate.getNonexistentSibling(true);
}

OfflineRenderResult renderAudioFile(const OfflineRenderRequest& request, CancelProbe shouldCancel,
                                    ProgressSink progress)
{
    const auto isCancelled = [&] { return shouldCancel && shouldCancel(); };
    const auto reportProgress = [&](float value)
    {
        if (progress)
            progress(juce::jlimit(0.0f, 1.0f, value));
    };

    if (!request.inputFile.existsAsFile())
        return failed(request.outputFile, "The input file does not exist");
    if (request.outputFile == juce::File{} || request.outputFile == request.inputFile)
        return failed(request.outputFile, "Choose a different output file");
    if (!request.outputFile.hasFileExtension(".wav"))
        return failed(request.outputFile, "The output file must use the .wav extension");
    if (request.outputFile.exists() && !request.replaceExisting)
        return failed(request.outputFile, "The output file already exists");
    if (!request.outputFile.getParentDirectory().hasWriteAccess())
        return failed(request.outputFile, "The output folder is not writable");
    if (isCancelled())
        return cancelled(request.outputFile);

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(request.inputFile));
    if (reader == nullptr)
        return failed(request.outputFile, "Unsupported or unreadable audio file");
    if (reader->numChannels < 1 || reader->numChannels > 2)
        return failed(request.outputFile, "Only mono or stereo recordings are supported");
    if (reader->lengthInSamples <= 0 || !std::isfinite(reader->sampleRate) || reader->sampleRate <= 0.0)
        return failed(request.outputFile, "The recording is empty or has an invalid sample rate");

    juce::TemporaryFile temporary(request.outputFile, juce::TemporaryFile::useHiddenFile);
    std::unique_ptr<juce::OutputStream> outputStream = temporary.getFile().createOutputStream();
    if (outputStream == nullptr)
        return failed(request.outputFile, "Could not create a temporary output file");

    juce::WavAudioFormat wav;
    const auto writerOptions = juce::AudioFormatWriterOptions()
                                   .withSampleRate(reader->sampleRate)
                                   .withNumChannels(static_cast<int>(reader->numChannels))
                                   .withBitsPerSample(24);
    auto writer = wav.createWriterFor(outputStream, writerOptions);
    if (writer == nullptr)
        return failed(request.outputFile, "Could not create the 24-bit WAV writer");

    constexpr int blockSize = 1024;
    RapReadyDSP dsp;
    dsp.setAmount(request.amount);
    dsp.setAdvancedSettings(request.advanced);
    dsp.prepare(reader->sampleRate, blockSize, static_cast<int>(reader->numChannels));
    auto latencyRemaining = dsp.getLatencySamples();
    const auto totalProcessSamples = reader->lengthInSamples + latencyRemaining;
    juce::int64 processedSamples = 0;
    juce::AudioBuffer<float> block(static_cast<int>(reader->numChannels), blockSize);
    juce::int64 inputPosition = 0;
    juce::int64 outputWritten = 0;
    reportProgress(0.0f);

    const auto processAndWrite = [&](int count, bool readInput) -> OfflineRenderResult
    {
        block.clear();
        juce::AudioBuffer<float> activeBlock(block.getArrayOfWritePointers(), block.getNumChannels(), count);
        if (readInput && !reader->read(&activeBlock, 0, count, inputPosition, true, true))
            return failed(request.outputFile, "The input file could not be decoded");

        dsp.process(activeBlock);
        auto start = 0;
        if (latencyRemaining > 0)
        {
            const auto skip = std::min(latencyRemaining, count);
            start += skip;
            latencyRemaining -= skip;
        }
        const auto writable = static_cast<int>(std::min<juce::int64>(
            count - start, reader->lengthInSamples - outputWritten));
        if (writable > 0 && !writer->writeFromAudioSampleBuffer(activeBlock, start, writable))
            return failed(request.outputFile, "The cleaned WAV could not be written");
        outputWritten += writable;
        processedSamples += count;
        reportProgress(static_cast<float>(processedSamples) / static_cast<float>(totalProcessSamples));
        return {OfflineRenderStatus::succeeded, request.outputFile, {}};
    };

    while (inputPosition < reader->lengthInSamples)
    {
        if (isCancelled())
        {
            writer.reset();
            return cancelled(request.outputFile);
        }
        const auto count = static_cast<int>(
            std::min<juce::int64>(blockSize, reader->lengthInSamples - inputPosition));
        const auto result = processAndWrite(count, true);
        if (!result.wasSuccessful())
        {
            writer.reset();
            return result;
        }
        inputPosition += count;
    }

    while (outputWritten < reader->lengthInSamples)
    {
        if (isCancelled())
        {
            writer.reset();
            return cancelled(request.outputFile);
        }
        const auto remainingToProcess = totalProcessSamples - processedSamples;
        const auto count = static_cast<int>(std::min<juce::int64>(blockSize, remainingToProcess));
        if (count <= 0)
        {
            writer.reset();
            return failed(request.outputFile, "Internal latency flush ended early");
        }
        const auto result = processAndWrite(count, false);
        if (!result.wasSuccessful())
        {
            writer.reset();
            return result;
        }
    }

    writer.reset();
    if (isCancelled())
        return cancelled(request.outputFile);

    auto verificationReader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(temporary.getFile()));
    if (verificationReader == nullptr || verificationReader->numChannels != reader->numChannels ||
        verificationReader->lengthInSamples != reader->lengthInSamples ||
        std::abs(verificationReader->sampleRate - reader->sampleRate) > 0.5 ||
        verificationReader->bitsPerSample != 24)
        return failed(request.outputFile, "The temporary WAV failed verification");
    verificationReader.reset();

    if (isCancelled())
        return cancelled(request.outputFile);
    if (!request.replaceExisting && request.outputFile.exists())
        return failed(request.outputFile,
                      "Another output appeared while cleaning; the new result was not committed");

    if (!temporary.overwriteTargetFileWithTemporary())
        return failed(request.outputFile, "The cleaned WAV could not replace its target");

    reportProgress(1.0f);
    return {OfflineRenderStatus::succeeded, request.outputFile,
            "Saved " + request.outputFile.getFileName()};
}
} // namespace rapready
