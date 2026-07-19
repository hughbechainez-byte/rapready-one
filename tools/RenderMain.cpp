#include "Dsp/RapReadyDSP.h"

#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Usage: RapReadyRender <input-audio> <output.wav> [amount 0-100]\n";
        return 2;
    }

    const juce::File inputFile(juce::String::fromUTF8(argv[1]));
    const juce::File outputFile(juce::String::fromUTF8(argv[2]));
    if (!inputFile.existsAsFile() || inputFile == outputFile)
    {
        std::cerr << "Input must exist and output must be a different file.\n";
        return 2;
    }

    const auto amount = argc == 4 ? juce::jlimit(0.0f, 100.0f, juce::String(argv[3]).getFloatValue()) : 62.0f;
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(inputFile));
    if (reader == nullptr || reader->numChannels < 1 || reader->numChannels > 2)
    {
        std::cerr << "Input must be readable mono or stereo audio.\n";
        return 2;
    }

    outputFile.deleteFile();
    auto outputStream = outputFile.createOutputStream();
    if (outputStream == nullptr)
    {
        std::cerr << "Could not create output file.\n";
        return 2;
    }

    juce::WavAudioFormat wav;
    const auto writerOptions = juce::AudioFormatWriterOptions()
                                   .withSampleRate(reader->sampleRate)
                                   .withNumChannels(static_cast<int>(reader->numChannels))
                                   .withBitsPerSample(24);
    auto writer = wav.createWriterFor(outputStream, writerOptions);
    if (writer == nullptr)
    {
        std::cerr << "Could not create WAV writer.\n";
        return 2;
    }

    constexpr int blockSize = 1024;
    rapready::RapReadyDSP dsp;
    dsp.setAmount(amount);
    dsp.prepare(reader->sampleRate, blockSize, static_cast<int>(reader->numChannels));
    auto latencyRemaining = dsp.getLatencySamples();
    juce::AudioBuffer<float> block(static_cast<int>(reader->numChannels), blockSize);
    juce::int64 inputPosition = 0;
    juce::int64 outputWritten = 0;

    while (inputPosition < reader->lengthInSamples)
    {
        const auto count =
            static_cast<int>(std::min<juce::int64>(blockSize, reader->lengthInSamples - inputPosition));
        block.clear();
        reader->read(&block, 0, count, inputPosition, true, true);
        dsp.process(block);
        auto start = 0;
        if (latencyRemaining > 0)
        {
            const auto skip = std::min(latencyRemaining, count);
            start += skip;
            latencyRemaining -= skip;
        }
        const auto writable = std::min<juce::int64>(count - start, reader->lengthInSamples - outputWritten);
        if (writable > 0)
        {
            writer->writeFromAudioSampleBuffer(block, start, static_cast<int>(writable));
            outputWritten += writable;
        }
        inputPosition += count;
    }

    while (outputWritten < reader->lengthInSamples)
    {
        const auto count =
            static_cast<int>(std::min<juce::int64>(blockSize, reader->lengthInSamples - outputWritten));
        block.clear();
        dsp.process(block);
        auto start = 0;
        if (latencyRemaining > 0)
        {
            const auto skip = std::min(latencyRemaining, count);
            start += skip;
            latencyRemaining -= skip;
        }
        const auto writable = count - start;
        if (writable > 0)
        {
            writer->writeFromAudioSampleBuffer(block, start, writable);
            outputWritten += writable;
        }
    }

    writer.reset();
    std::cout << "Rendered " << inputFile.getFileName() << " at " << amount << "% to "
              << outputFile.getFullPathName() << '\n';
    return 0;
}
