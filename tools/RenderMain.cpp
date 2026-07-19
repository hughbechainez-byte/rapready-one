#include "OfflineRenderer.h"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Usage: \"RapReady File Renderer\" <input-audio> <output.wav> [amount 0-100]\n";
        return 2;
    }

    rapready::OfflineRenderRequest request;
    request.inputFile = juce::File(juce::String::fromUTF8(argv[1]));
    request.outputFile = juce::File(juce::String::fromUTF8(argv[2]));
    request.amount = argc == 4
                         ? juce::jlimit(0.0f, 100.0f, juce::String::fromUTF8(argv[3]).getFloatValue())
                         : 62.0f;
    request.replaceExisting = true;

    const auto result = rapready::renderAudioFile(request);
    if (!result.wasSuccessful())
    {
        std::cerr << result.message << '\n';
        return result.status == rapready::OfflineRenderStatus::cancelled ? 3 : 2;
    }

    std::cout << "Rendered " << request.inputFile.getFileName() << " at " << request.amount << "% to "
              << result.outputFile.getFullPathName() << '\n';
    return 0;
}
