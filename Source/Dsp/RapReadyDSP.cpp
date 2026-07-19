#include "RapReadyDSP.h"

#include <algorithm>
#include <cmath>

namespace rapready
{
namespace
{
constexpr float minimumDb = -100.0f;
constexpr double pi = 3.14159265358979323846;

float timeCoefficient(double sampleRate, float milliseconds) noexcept
{
    return static_cast<float>(std::exp(-1.0 / (0.001 * std::max(0.01f, milliseconds) * sampleRate)));
}
} // namespace

void RapReadyDSP::Biquad::reset() noexcept
{
    z1 = 0.0;
    z2 = 0.0;
}

float RapReadyDSP::Biquad::process(float input) noexcept
{
    const auto output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return static_cast<float>(output);
}

void RapReadyDSP::Biquad::setHighPass(double sr, float frequency, float q) noexcept
{
    const auto f = std::clamp(static_cast<double>(frequency), 10.0, sr * 0.45);
    const auto w0 = 2.0 * pi * f / sr;
    const auto cosine = std::cos(w0);
    const auto alpha = std::sin(w0) / (2.0 * std::max(0.1f, q));
    const auto a0 = 1.0 + alpha;
    b0 = ((1.0 + cosine) * 0.5) / a0;
    b1 = (-(1.0 + cosine)) / a0;
    b2 = b0;
    a1 = (-2.0 * cosine) / a0;
    a2 = (1.0 - alpha) / a0;
}

void RapReadyDSP::Biquad::setLowPass(double sr, float frequency, float q) noexcept
{
    const auto f = std::clamp(static_cast<double>(frequency), 10.0, sr * 0.45);
    const auto w0 = 2.0 * pi * f / sr;
    const auto cosine = std::cos(w0);
    const auto alpha = std::sin(w0) / (2.0 * std::max(0.1f, q));
    const auto a0 = 1.0 + alpha;
    b0 = ((1.0 - cosine) * 0.5) / a0;
    b1 = (1.0 - cosine) / a0;
    b2 = b0;
    a1 = (-2.0 * cosine) / a0;
    a2 = (1.0 - alpha) / a0;
}

void RapReadyDSP::Biquad::setPeak(double sr, float frequency, float q, float gainDb) noexcept
{
    const auto f = std::clamp(static_cast<double>(frequency), 10.0, sr * 0.45);
    const auto w0 = 2.0 * pi * f / sr;
    const auto cosine = std::cos(w0);
    const auto alpha = std::sin(w0) / (2.0 * std::max(0.1f, q));
    const auto a = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
    const auto a0 = 1.0 + alpha / a;
    b0 = (1.0 + alpha * a) / a0;
    b1 = (-2.0 * cosine) / a0;
    b2 = (1.0 - alpha * a) / a0;
    a1 = b1;
    a2 = (1.0 - alpha / a) / a0;
}

void RapReadyDSP::Biquad::setHighShelf(double sr, float frequency, float gainDb) noexcept
{
    const auto f = std::clamp(static_cast<double>(frequency), 10.0, sr * 0.45);
    const auto w0 = 2.0 * pi * f / sr;
    const auto cosine = std::cos(w0);
    const auto sine = std::sin(w0);
    const auto a = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
    const auto beta = 2.0 * std::sqrt(a) * (sine / std::sqrt(2.0));
    const auto a0 = (a + 1.0) - (a - 1.0) * cosine + beta;
    b0 = (a * ((a + 1.0) + (a - 1.0) * cosine + beta)) / a0;
    b1 = (-2.0 * a * ((a - 1.0) + (a + 1.0) * cosine)) / a0;
    b2 = (a * ((a + 1.0) + (a - 1.0) * cosine - beta)) / a0;
    a1 = (2.0 * ((a - 1.0) - (a + 1.0) * cosine)) / a0;
    a2 = ((a + 1.0) - (a - 1.0) * cosine - beta) / a0;
}

void RapReadyDSP::PeakEnvelope::setTimes(double sr, float attackMs, float releaseMs) noexcept
{
    attackCoefficient = timeCoefficient(sr, attackMs);
    releaseCoefficient = timeCoefficient(sr, releaseMs);
}

float RapReadyDSP::PeakEnvelope::process(float input) noexcept
{
    const auto coefficient = input > value ? attackCoefficient : releaseCoefficient;
    value = coefficient * value + (1.0f - coefficient) * input;
    return value;
}

void RapReadyDSP::Compressor::configure(double sr, float thresholdDb, float newRatio, float kneeDb,
                                        float attackMs, float releaseMs, float maximumReductionDb) noexcept
{
    threshold = thresholdDb;
    ratioValue = std::max(1.0f, newRatio);
    knee = std::max(0.0f, kneeDb);
    maxReduction = std::max(0.0f, maximumReductionDb);
    detector.setTimes(sr, attackMs, releaseMs);
}

float RapReadyDSP::Compressor::calculateGain(float linkedPeak, float& reductionDb) noexcept
{
    const auto levelDb = RapReadyDSP::gainToDb(detector.process(linkedPeak));
    float gainDb = 0.0f;
    const auto halfKnee = knee * 0.5f;

    if (levelDb > threshold + halfKnee)
        gainDb = (threshold + (levelDb - threshold) / ratioValue) - levelDb;
    else if (knee > 0.0f && levelDb > threshold - halfKnee)
    {
        const auto distance = levelDb - threshold + halfKnee;
        gainDb = (1.0f / ratioValue - 1.0f) * distance * distance / (2.0f * knee);
    }

    reductionDb = std::clamp(gainDb, -maxReduction, 0.0f);
    return RapReadyDSP::dbToGain(reductionDb);
}

void RapReadyDSP::ChannelState::reset() noexcept
{
    highPass.reset();
    mudCut.reset();
    boxCut.reset();
    presence.reset();
    airShelf.reset();
    lowPass.reset();
    deEsserLowState1 = 0.0f;
    deEsserLowState2 = 0.0f;
}

void RapReadyDSP::prepare(double newSampleRate, int, int channelCount)
{
    sampleRate = std::max(8000.0, newSampleRate);
    activeChannels = std::clamp(channelCount, 1, maxChannels);
    lookaheadSamples = std::max(1, static_cast<int>(std::round(sampleRate * 0.002)));
    noiseFrameSamples = std::max(1, static_cast<int>(std::round(sampleRate * 0.020)));

    for (auto& line : delayLines)
        line.assign(static_cast<std::size_t>(lookaheadSamples + 1), 0.0f);
    limiterQueueValues.assign(static_cast<std::size_t>(lookaheadSamples + 1), 0.0f);
    limiterQueueIndices.assign(static_cast<std::size_t>(lookaheadSamples + 1), 0);

    amountSmoothingCoefficient = timeCoefficient(sampleRate, 45.0f);
    reset();
    updateStageSettings(currentAmount);
}

void RapReadyDSP::reset()
{
    for (auto& channel : channels)
        channel.reset();
    for (auto& line : delayLines)
        std::fill(line.begin(), line.end(), 0.0f);
    std::fill(limiterQueueValues.begin(), limiterQueueValues.end(), 0.0f);
    std::fill(limiterQueueIndices.begin(), limiterQueueIndices.end(), 0);

    peakCompressor.reset();
    bodyCompressor.reset();
    expanderDetector.reset();
    deEsserDetector.reset();
    fullBandDetector.reset();
    delayWriteIndex = 0;
    limiterQueueHead = 0;
    limiterQueueCount = 0;
    limiterSampleIndex = 0;
    controlSampleCounter = 0;
    noiseAnalysisSquareSum = 0.0;
    noiseAnalysisSampleCount = 0;
    expanderGain = 1.0f;
    deEsserGain = 1.0f;
    limiterGain = 1.0f;
    noiseFloorDb = -60.0f;
    currentAmount = std::clamp(targetAmount.load(), 0.0f, 100.0f);
    lastSettingsAmount = -1.0f;
    lastSettingsNoiseFloor = -1000.0f;
    inputLevelDb.store(minimumDb);
    outputLevelDb.store(minimumDb);
    gainReductionDb.store(0.0f);
    learnedNoiseFloorDb.store(noiseFloorDb);
}

void RapReadyDSP::setAmount(float newAmountPercent) noexcept
{
    targetAmount.store(std::isfinite(newAmountPercent) ? std::clamp(newAmountPercent, 0.0f, 100.0f) : 0.0f);
}

float RapReadyDSP::dbToGain(float decibels) noexcept
{
    return std::pow(10.0f, decibels / 20.0f);
}

float RapReadyDSP::gainToDb(float gain) noexcept
{
    return gain > 0.00001f ? 20.0f * std::log10(gain) : minimumDb;
}

float RapReadyDSP::smoothStep(float value) noexcept
{
    const auto x = std::clamp(value, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

void RapReadyDSP::updateStageSettings(float smoothedAmount) noexcept
{
    lastSettingsAmount = smoothedAmount;
    lastSettingsNoiseFloor = noiseFloorDb;
    strength = smoothStep(smoothedAmount * 0.01f);
    const auto highPassHz = 45.0f + 45.0f * strength;
    const auto lowPassHz = 20000.0f - 2000.0f * std::max(0.0f, (strength - 0.70f) / 0.30f);

    for (auto& channel : channels)
    {
        channel.highPass.setHighPass(sampleRate, highPassHz, 0.707f);
        channel.mudCut.setPeak(sampleRate, 260.0f, 0.85f, -2.2f * strength);
        channel.boxCut.setPeak(sampleRate, 620.0f, 1.0f, -1.2f * strength);
        channel.presence.setPeak(sampleRate, 3600.0f, 0.75f, 1.3f * strength);
        channel.airShelf.setHighShelf(sampleRate, 10500.0f, 1.0f * strength);
        channel.lowPass.setLowPass(sampleRate, lowPassHz, 0.707f);
    }

    expanderThresholdDb = std::clamp(noiseFloorDb + 8.0f, -56.0f, -42.0f);
    expanderMaxAttenuationDb = 12.0f * strength;
    expanderRatio = 1.0f + 0.8f * strength;
    expanderDetector.setTimes(sampleRate, 3.0f, 90.0f);
    expanderOpenCoefficient = timeCoefficient(sampleRate, 4.0f);
    expanderCloseCoefficient = timeCoefficient(sampleRate, 160.0f);

    peakCompressor.configure(sampleRate, -8.0f - 10.0f * strength, 1.0f + 3.0f * strength, 5.0f,
                             7.0f - 4.0f * strength, 55.0f, 2.5f * strength);
    bodyCompressor.configure(sampleRate, -10.0f - 12.0f * strength, 1.0f + 1.3f * strength, 6.0f, 35.0f,
                             130.0f, 4.0f * strength);

    const auto splitHz = 5800.0f;
    deEsserSplitCoefficient = static_cast<float>(std::exp(-2.0 * pi * splitHz / sampleRate));
    deEsserMaximumReductionDb = 4.5f * strength;
    deEsserDetector.setTimes(sampleRate, 0.8f, 65.0f);
    fullBandDetector.setTimes(sampleRate, 2.0f, 80.0f);
    deEsserAttackCoefficient = timeCoefficient(sampleRate, 1.0f);
    deEsserReleaseCoefficient = timeCoefficient(sampleRate, 75.0f);

    saturationDrive = 1.0f + 0.8f * strength;
    saturationMix = 0.12f * strength;
    outputGain = dbToGain(1.2f * strength);
    limiterCeiling = dbToGain(-0.3f - 0.9f * strength);
    limiterReleaseCoefficient = timeCoefficient(sampleRate, 85.0f);
}

void RapReadyDSP::updateNoiseEstimate(float blockRmsDb, int sampleCount) noexcept
{
    if (blockRmsDb >= -35.0f || sampleCount <= 0)
        return;

    const auto seconds = static_cast<float>(sampleCount / sampleRate);
    const auto timeSeconds = blockRmsDb < noiseFloorDb ? 1.0f : 18.0f;
    const auto coefficient = std::exp(-seconds / timeSeconds);
    noiseFloorDb = std::clamp(coefficient * noiseFloorDb + (1.0f - coefficient) * blockRmsDb, -80.0f, -35.0f);
    learnedNoiseFloorDb.store(noiseFloorDb);
}

float RapReadyDSP::calculateExpanderGain(float linkedPeak) noexcept
{
    const auto levelDb = gainToDb(expanderDetector.process(linkedPeak));
    const auto distanceBelow = std::max(0.0f, expanderThresholdDb - levelDb);
    const auto attenuationDb = std::min(expanderMaxAttenuationDb, distanceBelow * (expanderRatio - 1.0f));
    const auto wanted = strength < 0.0001f ? 1.0f : dbToGain(-attenuationDb);
    const auto coefficient = wanted > expanderGain ? expanderOpenCoefficient : expanderCloseCoefficient;
    expanderGain = coefficient * expanderGain + (1.0f - coefficient) * wanted;
    return expanderGain;
}

float RapReadyDSP::calculateDeEsserGain(float linkedFullPeak, float linkedHighPeak) noexcept
{
    const auto highDb = gainToDb(deEsserDetector.process(linkedHighPeak));
    const auto fullDb = gainToDb(fullBandDetector.process(linkedFullPeak));
    const auto absoluteExcess = highDb + 36.0f;
    const auto relativeExcess = (highDb - fullDb) + 15.0f;
    const auto triggerDb = std::max(0.0f, std::min(absoluteExcess, relativeExcess));
    const auto wanted =
        strength < 0.0001f ? 1.0f : dbToGain(-std::min(deEsserMaximumReductionDb, triggerDb * 0.75f));
    const auto coefficient = wanted < deEsserGain ? deEsserAttackCoefficient : deEsserReleaseCoefficient;
    deEsserGain = coefficient * deEsserGain + (1.0f - coefficient) * wanted;
    return deEsserGain;
}

float RapReadyDSP::calculateLimiterGain(float linkedPeak) noexcept
{
    if (limiterQueueValues.empty())
        return 1.0f;

    const auto capacity = static_cast<int>(limiterQueueValues.size());
    const auto oldestAllowed = limiterSampleIndex > static_cast<std::uint64_t>(lookaheadSamples)
                                   ? limiterSampleIndex - static_cast<std::uint64_t>(lookaheadSamples)
                                   : 0;
    while (limiterQueueCount > 0 &&
           limiterQueueIndices[static_cast<std::size_t>(limiterQueueHead)] < oldestAllowed)
    {
        limiterQueueHead = (limiterQueueHead + 1) % capacity;
        --limiterQueueCount;
    }

    while (limiterQueueCount > 0)
    {
        const auto tail = (limiterQueueHead + limiterQueueCount - 1) % capacity;
        if (limiterQueueValues[static_cast<std::size_t>(tail)] > linkedPeak)
            break;
        --limiterQueueCount;
    }

    const auto insertion = (limiterQueueHead + limiterQueueCount) % capacity;
    limiterQueueValues[static_cast<std::size_t>(insertion)] = linkedPeak;
    limiterQueueIndices[static_cast<std::size_t>(insertion)] = limiterSampleIndex;
    ++limiterQueueCount;

    const auto windowPeak = limiterQueueValues[static_cast<std::size_t>(limiterQueueHead)];
    ++limiterSampleIndex;
    const auto wanted = strength < 0.0001f || windowPeak <= limiterCeiling
                            ? 1.0f
                            : limiterCeiling / std::max(windowPeak, 0.000001f);
    limiterGain = wanted < limiterGain
                      ? wanted
                      : limiterReleaseCoefficient * limiterGain + (1.0f - limiterReleaseCoefficient) * wanted;
    return limiterGain;
}

void RapReadyDSP::process(juce::AudioBuffer<float>& buffer) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const auto channelCount = std::min(activeChannels, buffer.getNumChannels());
    const auto sampleCount = buffer.getNumSamples();
    if (channelCount <= 0 || sampleCount <= 0 || delayLines[0].empty() || limiterQueueValues.empty())
        return;

    for (auto channel = channelCount; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, sampleCount);

    float blockInputPeak = 0.0f;
    float blockOutputPeak = 0.0f;
    float maximumReduction = 0.0f;

    std::array<float*, maxChannels> writePointers{};
    for (auto channel = 0; channel < channelCount; ++channel)
        writePointers[static_cast<std::size_t>(channel)] = buffer.getWritePointer(channel);

    for (auto sample = 0; sample < sampleCount; ++sample)
    {
        const auto wantedAmount = targetAmount.load();
        currentAmount =
            amountSmoothingCoefficient * currentAmount + (1.0f - amountSmoothingCoefficient) * wantedAmount;
        if ((controlSampleCounter++ & 31U) == 0U && (std::abs(currentAmount - lastSettingsAmount) > 0.001f ||
                                                     std::abs(noiseFloorDb - lastSettingsNoiseFloor) > 0.05f))
            updateStageSettings(currentAmount);

        std::array<float, maxChannels> values{};
        std::array<float, maxChannels> dryValues{};
        float linkedInput = 0.0f;
        for (auto channel = 0; channel < channelCount; ++channel)
        {
            const auto rawInput = writePointers[static_cast<std::size_t>(channel)][sample];
            const auto input = std::isfinite(rawInput) ? std::clamp(rawInput, -8.0f, 8.0f) : 0.0f;
            dryValues[static_cast<std::size_t>(channel)] = input;
            blockInputPeak = std::max(blockInputPeak, std::abs(input));
            noiseAnalysisSquareSum += static_cast<double>(input) * input;

            auto value = channels[static_cast<std::size_t>(channel)].highPass.process(input);
            value = channels[static_cast<std::size_t>(channel)].mudCut.process(value);
            value = channels[static_cast<std::size_t>(channel)].boxCut.process(value);
            values[static_cast<std::size_t>(channel)] = value;
            linkedInput = std::max(linkedInput, std::abs(value));
        }
        noiseAnalysisSampleCount += channelCount;
        if (noiseAnalysisSampleCount >= noiseFrameSamples * channelCount)
        {
            const auto noiseRms = std::sqrt(noiseAnalysisSquareSum / noiseAnalysisSampleCount);
            updateNoiseEstimate(gainToDb(static_cast<float>(noiseRms)), noiseFrameSamples);
            noiseAnalysisSquareSum = 0.0;
            noiseAnalysisSampleCount = 0;
        }

        const auto expansion = calculateExpanderGain(linkedInput);
        for (auto channel = 0; channel < channelCount; ++channel)
            values[static_cast<std::size_t>(channel)] *= expansion;

        float peakReductionDb = 0.0f;
        const auto peakGain = peakCompressor.calculateGain(linkedInput * expansion, peakReductionDb);
        for (auto channel = 0; channel < channelCount; ++channel)
            values[static_cast<std::size_t>(channel)] *= peakGain;

        float bodyLinkedPeak = 0.0f;
        for (auto channel = 0; channel < channelCount; ++channel)
            bodyLinkedPeak = std::max(bodyLinkedPeak, std::abs(values[static_cast<std::size_t>(channel)]));
        float bodyReductionDb = 0.0f;
        const auto bodyGain = bodyCompressor.calculateGain(bodyLinkedPeak, bodyReductionDb);
        for (auto channel = 0; channel < channelCount; ++channel)
            values[static_cast<std::size_t>(channel)] *= bodyGain;

        float fullPeak = 0.0f;
        float highPeak = 0.0f;
        std::array<float, maxChannels> lows{};
        std::array<float, maxChannels> highs{};
        for (auto channel = 0; channel < channelCount; ++channel)
        {
            auto& channelState = channels[static_cast<std::size_t>(channel)];
            channelState.deEsserLowState1 =
                deEsserSplitCoefficient * channelState.deEsserLowState1 +
                (1.0f - deEsserSplitCoefficient) * values[static_cast<std::size_t>(channel)];
            channelState.deEsserLowState2 = deEsserSplitCoefficient * channelState.deEsserLowState2 +
                                            (1.0f - deEsserSplitCoefficient) * channelState.deEsserLowState1;
            lows[static_cast<std::size_t>(channel)] = channelState.deEsserLowState2;
            highs[static_cast<std::size_t>(channel)] =
                values[static_cast<std::size_t>(channel)] - channelState.deEsserLowState2;
            fullPeak = std::max(fullPeak, std::abs(values[static_cast<std::size_t>(channel)]));
            highPeak = std::max(highPeak, std::abs(highs[static_cast<std::size_t>(channel)]));
        }
        const auto deEssGain = calculateDeEsserGain(fullPeak, highPeak);

        const auto compressionReduction = -(peakReductionDb + bodyReductionDb);
        const auto makeup = dbToGain(std::min(2.5f, compressionReduction * 0.35f));
        const auto bypassMix = std::clamp(strength * 50.0f, 0.0f, 1.0f);
        const auto dynamicsGain = expansion * peakGain * bodyGain * deEssGain;

        float linkedPreLimiterPeak = 0.0f;
        for (auto channel = 0; channel < channelCount; ++channel)
        {
            auto value = lows[static_cast<std::size_t>(channel)] +
                         highs[static_cast<std::size_t>(channel)] * deEssGain;
            value = channels[static_cast<std::size_t>(channel)].presence.process(value);
            value = channels[static_cast<std::size_t>(channel)].airShelf.process(value);

            const auto soft = std::tanh(saturationDrive * value) / saturationDrive;
            value = value + (soft - value) * saturationMix;
            value *= makeup * outputGain;
            value = channels[static_cast<std::size_t>(channel)].lowPass.process(value);
            value = dryValues[static_cast<std::size_t>(channel)] +
                    (value - dryValues[static_cast<std::size_t>(channel)]) * bypassMix;
            values[static_cast<std::size_t>(channel)] = value;
            linkedPreLimiterPeak = std::max(linkedPreLimiterPeak, std::abs(value));
        }

        const auto finalLimiterGain = calculateLimiterGain(linkedPreLimiterPeak);
        const auto effectiveLimiterGain = 1.0f + (finalLimiterGain - 1.0f) * bypassMix;
        const auto effectiveDynamicsGain = 1.0f + (dynamicsGain - 1.0f) * bypassMix;
        maximumReduction =
            std::max(maximumReduction,
                     gainToDb(1.0f / std::max(effectiveDynamicsGain * effectiveLimiterGain, 0.0001f)));

        const auto delaySize = static_cast<int>(delayLines[0].size());
        const auto readIndex = (delayWriteIndex + 1) % delaySize;
        for (auto channel = 0; channel < channelCount; ++channel)
        {
            auto& line = delayLines[static_cast<std::size_t>(channel)];
            line[static_cast<std::size_t>(delayWriteIndex)] = values[static_cast<std::size_t>(channel)];
            const auto output = line[static_cast<std::size_t>(readIndex)] * effectiveLimiterGain;
            writePointers[static_cast<std::size_t>(channel)][sample] = output;
            blockOutputPeak = std::max(blockOutputPeak, std::abs(output));
        }
        delayWriteIndex = readIndex;
    }

    inputLevelDb.store(gainToDb(blockInputPeak));
    outputLevelDb.store(gainToDb(blockOutputPeak));
    gainReductionDb.store(maximumReduction);
}
} // namespace rapready
