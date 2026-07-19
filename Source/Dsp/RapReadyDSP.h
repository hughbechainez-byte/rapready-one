#pragma once

#include <array>
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace rapready
{
class RapReadyDSP final
{
  public:
    static constexpr int maxChannels = 2;

    void prepare(double newSampleRate, int maximumBlockSize, int channelCount);
    void reset();
    void setAmount(float newAmountPercent) noexcept;
    void process(juce::AudioBuffer<float>& buffer) noexcept;

    [[nodiscard]] int getLatencySamples() const noexcept { return lookaheadSamples; }
    [[nodiscard]] float getInputLevelDb() const noexcept { return inputLevelDb.load(); }
    [[nodiscard]] float getOutputLevelDb() const noexcept { return outputLevelDb.load(); }
    [[nodiscard]] float getGainReductionDb() const noexcept { return gainReductionDb.load(); }
    [[nodiscard]] float getNoiseFloorDb() const noexcept { return learnedNoiseFloorDb.load(); }

  private:
    struct Biquad
    {
        void reset() noexcept;
        float process(float input) noexcept;
        void setHighPass(double sampleRate, float frequency, float q) noexcept;
        void setLowPass(double sampleRate, float frequency, float q) noexcept;
        void setPeak(double sampleRate, float frequency, float q, float gainDb) noexcept;
        void setHighShelf(double sampleRate, float frequency, float gainDb) noexcept;

        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;
    };

    struct PeakEnvelope
    {
        void reset() noexcept { value = 0.0f; }
        void setTimes(double sampleRate, float attackMs, float releaseMs) noexcept;
        float process(float input) noexcept;

        float value = 0.0f;
        float attackCoefficient = 0.0f;
        float releaseCoefficient = 0.0f;
    };

    struct Compressor
    {
        void reset() noexcept { detector.reset(); }
        void configure(double sampleRate, float thresholdDb, float ratio, float kneeDb, float attackMs,
                       float releaseMs, float maximumReductionDb) noexcept;
        float calculateGain(float linkedPeak, float& reductionDb) noexcept;

        PeakEnvelope detector;
        float threshold = 0.0f;
        float ratioValue = 1.0f;
        float knee = 0.0f;
        float maxReduction = 0.0f;
    };

    struct ChannelState
    {
        Biquad highPass;
        Biquad mudCut;
        Biquad boxCut;
        Biquad presence;
        Biquad airShelf;
        Biquad lowPass;
        float deEsserLowState = 0.0f;

        void reset() noexcept;
    };

    void updateStageSettings(float smoothedAmount) noexcept;
    void updateNoiseEstimate(float blockRmsDb, int sampleCount) noexcept;
    float calculateExpanderGain(float linkedPeak) noexcept;
    float calculateDeEsserGain(float linkedFullPeak, float linkedHighPeak) noexcept;
    float calculateLimiterGain(float linkedPeak) noexcept;
    static float dbToGain(float decibels) noexcept;
    static float gainToDb(float gain) noexcept;
    static float smoothStep(float value) noexcept;

    std::array<ChannelState, maxChannels> channels;
    Compressor peakCompressor;
    Compressor bodyCompressor;
    PeakEnvelope expanderDetector;
    PeakEnvelope deEsserDetector;
    PeakEnvelope fullBandDetector;

    double sampleRate = 48000.0;
    int activeChannels = 1;
    int lookaheadSamples = 96;
    int delayWriteIndex = 0;
    int peakWindowWriteIndex = 0;

    std::array<std::vector<float>, maxChannels> delayLines;
    std::vector<float> limiterPeakWindow;

    std::atomic<float> targetAmount{62.0f};
    float currentAmount = 62.0f;
    float amountSmoothingCoefficient = 0.0f;
    float noiseFloorDb = -60.0f;
    float expanderGain = 1.0f;
    float deEsserGain = 1.0f;
    float limiterGain = 1.0f;

    float expanderThresholdDb = -52.0f;
    float expanderMaxAttenuationDb = 8.0f;
    float expanderRatio = 1.5f;
    float expanderOpenCoefficient = 0.0f;
    float expanderCloseCoefficient = 0.0f;
    float deEsserSplitCoefficient = 0.0f;
    float deEsserMaximumReductionDb = 3.0f;
    float deEsserAttackCoefficient = 0.0f;
    float deEsserReleaseCoefficient = 0.0f;
    float saturationDrive = 1.0f;
    float saturationMix = 0.0f;
    float outputGain = 1.0f;
    float limiterCeiling = 1.0f;
    float limiterReleaseCoefficient = 0.0f;
    float strength = 0.0f;

    std::atomic<float> inputLevelDb{-100.0f};
    std::atomic<float> outputLevelDb{-100.0f};
    std::atomic<float> gainReductionDb{0.0f};
    std::atomic<float> learnedNoiseFloorDb{-60.0f};
};
} // namespace rapready
