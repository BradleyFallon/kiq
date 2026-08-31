#include "ReferenceAudio.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <numeric>
#include <utility>

namespace KickDrum {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr float kSilenceFloor = 1.0e-6f;

template <typename T>
T clampFinite(T value, T minimum, T maximum, T fallback) {
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

bool matchesFourCC(const std::uint8_t* bytes, const char* fourCC) {
    return std::memcmp(bytes, fourCC, 4) == 0;
}

std::uint16_t readU16(const std::uint8_t* bytes) {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8u);
}

std::uint32_t readU32(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

WavDecodeResult wavFailure(WavDecodeError error, std::string message) {
    WavDecodeResult result;
    result.error = error;
    result.message = std::move(message);
    return result;
}

KickAnalysisResult analysisFailure(KickAnalysisError error, std::string message) {
    KickAnalysisResult result;
    result.error = error;
    result.message = std::move(message);
    return result;
}

float coefficientForMs(float milliseconds, float sampleRate) {
    if (!(milliseconds > 0.0f) || !(sampleRate > 0.0f)) {
        return 1.0f;
    }
    return 1.0f - std::exp(-1.0f / (milliseconds * 0.001f * sampleRate));
}

std::size_t millisecondsToSamples(float milliseconds, std::uint32_t sampleRate) {
    const double samples = static_cast<double>(milliseconds) *
                           static_cast<double>(sampleRate) * 0.001;
    if (!(samples > 0.0)) {
        return 0;
    }
    return static_cast<std::size_t>(std::llround(samples));
}

float samplesToMilliseconds(std::size_t samples, std::uint32_t sampleRate) {
    return sampleRate > 0
               ? static_cast<float>(1000.0 * static_cast<double>(samples) /
                                    static_cast<double>(sampleRate))
               : 0.0f;
}

float linearToDecibels(float value) {
    return 20.0f * std::log10(std::max(value, 0.001f));
}

float vectorPeak(const std::vector<float>& samples) {
    float peak = 0.0f;
    for (float sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

std::vector<float> makeEnvelope(const std::vector<float>& samples,
                                std::uint32_t sampleRate) {
    std::vector<float> envelope(samples.size(), 0.0f);
    if (samples.empty()) {
        return envelope;
    }

    const float attack = coefficientForMs(0.25f, static_cast<float>(sampleRate));
    const float release = coefficientForMs(8.0f, static_cast<float>(sampleRate));
    float state = 0.0f;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const float magnitude = std::abs(samples[index]);
        const float coefficient = magnitude > state ? attack : release;
        state += coefficient * (magnitude - state);
        envelope[index] = state;
    }
    return envelope;
}

std::size_t findOnset(const std::vector<float>& samples,
                      std::uint32_t sampleRate,
                      float peak) {
    if (samples.empty()) {
        return 0;
    }

    const std::size_t noiseCount =
        std::min(samples.size(), std::max<std::size_t>(1, millisecondsToSamples(5.0f, sampleRate)));
    double noiseEnergy = 0.0;
    for (std::size_t index = 0; index < noiseCount; ++index) {
        noiseEnergy += static_cast<double>(samples[index]) * samples[index];
    }
    const float noiseRms =
        static_cast<float>(std::sqrt(noiseEnergy / static_cast<double>(noiseCount)));
    const float threshold = std::max({peak * 0.015f, noiseRms * 4.0f, 1.0e-5f});
    const std::size_t required = std::max<std::size_t>(1, millisecondsToSamples(0.08f, sampleRate));

    std::size_t run = 0;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        run = std::abs(samples[index]) >= threshold ? run + 1 : 0;
        if (run >= required) {
            const std::size_t found = index + 1 - run;
            const std::size_t preroll = millisecondsToSamples(0.35f, sampleRate);
            return found > preroll ? found - preroll : 0;
        }
    }
    return 0;
}

std::vector<ReferenceWaveformPoint> makeWaveform(
    const std::vector<float>& samples,
    std::uint32_t sampleRate,
    std::size_t requestedPointCount) {
    std::vector<ReferenceWaveformPoint> points;
    if (samples.empty() || requestedPointCount == 0) {
        return points;
    }

    const std::size_t pointCount = std::min(requestedPointCount, samples.size());
    points.reserve(pointCount);
    for (std::size_t point = 0; point < pointCount; ++point) {
        const std::size_t begin = point * samples.size() / pointCount;
        const std::size_t end = std::max(begin + 1, (point + 1) * samples.size() / pointCount);
        float minimum = samples[begin];
        float maximum = samples[begin];
        float strongest = samples[begin];
        double energy = 0.0;
        for (std::size_t index = begin; index < end; ++index) {
            const float sample = samples[index];
            minimum = std::min(minimum, sample);
            maximum = std::max(maximum, sample);
            if (std::abs(sample) > std::abs(strongest)) {
                strongest = sample;
            }
            energy += static_cast<double>(sample) * sample;
        }

        ReferenceWaveformPoint output;
        output.timeMs = samplesToMilliseconds((begin + end - 1) / 2, sampleRate);
        output.sample = strongest;
        output.minimum = minimum;
        output.maximum = maximum;
        output.rms = static_cast<float>(std::sqrt(energy / static_cast<double>(end - begin)));
        points.push_back(output);
    }
    return points;
}

std::vector<float> makePitchSignal(const std::vector<float>& input,
                                   std::uint32_t sampleRate,
                                   float maximumPitch,
                                   std::uint32_t& outputRate) {
    const std::uint32_t factor = std::max<std::uint32_t>(1, sampleRate / 10000u);
    outputRate = sampleRate / factor;

    const float nyquistGuard = static_cast<float>(outputRate) * 0.42f;
    const float cutoff = std::min(nyquistGuard, std::max(1200.0f, maximumPitch * 1.8f));
    const float lowpassCoefficient =
        1.0f - std::exp(-2.0f * static_cast<float>(kPi) * cutoff /
                        static_cast<float>(sampleRate));
    const float dcCoefficient = coefficientForMs(35.0f, static_cast<float>(sampleRate));

    std::vector<float> filtered(input.size(), 0.0f);
    float lowpass = 0.0f;
    float dc = 0.0f;
    for (std::size_t index = 0; index < input.size(); ++index) {
        dc += dcCoefficient * (input[index] - dc);
        lowpass += lowpassCoefficient * ((input[index] - dc) - lowpass);
        filtered[index] = lowpass;
    }

    const std::size_t outputCount = (filtered.size() + factor - 1) / factor;
    std::vector<float> output(outputCount, 0.0f);
    for (std::size_t outputIndex = 0; outputIndex < outputCount; ++outputIndex) {
        const std::size_t begin = outputIndex * factor;
        const std::size_t end = std::min(filtered.size(), begin + factor);
        double sum = 0.0;
        for (std::size_t index = begin; index < end; ++index) {
            sum += filtered[index];
        }
        output[outputIndex] = static_cast<float>(sum / static_cast<double>(end - begin));
    }
    return output;
}

struct PitchEstimate {
    float hertz = 0.0f;
    float confidence = 0.0f;
};

PitchEstimate estimatePitchAt(const std::vector<float>& signal,
                              std::uint32_t sampleRate,
                              std::size_t centerSample,
                              float minimumPitch,
                              float maximumPitch,
                              float windowMs) {
    PitchEstimate estimate;
    if (signal.size() < 16 || sampleRate == 0) {
        return estimate;
    }

    const std::size_t minimumLag = std::max<std::size_t>(2,
        static_cast<std::size_t>(static_cast<float>(sampleRate) / maximumPitch));
    const std::size_t requestedMaximumLag = std::max(minimumLag + 1,
        static_cast<std::size_t>(std::ceil(static_cast<float>(sampleRate) / minimumPitch)));
    const std::size_t requestedWindow = std::max<std::size_t>(32,
        millisecondsToSamples(windowMs, sampleRate));
    const std::size_t windowSize = std::min(signal.size(),
        std::max(requestedWindow, requestedMaximumLag + std::max<std::size_t>(32, requestedWindow / 4)));
    if (windowSize <= minimumLag + 8) {
        return estimate;
    }

    const std::size_t before = windowSize / 4;
    std::size_t start = centerSample > before ? centerSample - before : 0;
    if (start + windowSize > signal.size()) {
        start = signal.size() - windowSize;
    }

    double mean = 0.0;
    for (std::size_t index = 0; index < windowSize; ++index) {
        mean += signal[start + index];
    }
    mean /= static_cast<double>(windowSize);

    const std::size_t maximumLag =
        std::min(requestedMaximumLag, windowSize - 8);
    std::vector<float> correlations(maximumLag + 1, -1.0f);
    float bestCorrelation = -1.0f;
    std::size_t bestLag = minimumLag;
    for (std::size_t lag = minimumLag; lag <= maximumLag; ++lag) {
        const std::size_t count = windowSize - lag;
        double dot = 0.0;
        double firstEnergy = 0.0;
        double secondEnergy = 0.0;
        for (std::size_t index = 0; index < count; ++index) {
            const double first = static_cast<double>(signal[start + index]) - mean;
            const double second = static_cast<double>(signal[start + index + lag]) - mean;
            dot += first * second;
            firstEnergy += first * first;
            secondEnergy += second * second;
        }
        const double denominator = std::sqrt(firstEnergy * secondEnergy);
        const float correlation = denominator > 1.0e-14
                                      ? static_cast<float>(dot / denominator)
                                      : -1.0f;
        correlations[lag] = correlation;
        if (correlation > bestCorrelation) {
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }

    // Prefer the first strong local maximum. This avoids interpreting a
    // fundamental's second period as a sub-octave while retaining tolerance
    // for noisy/reference material.
    const float acceptance = std::max(0.42f, bestCorrelation * 0.88f);
    for (std::size_t lag = minimumLag + 1; lag < maximumLag; ++lag) {
        if (correlations[lag] >= acceptance &&
            correlations[lag] >= correlations[lag - 1] &&
            correlations[lag] > correlations[lag + 1]) {
            bestLag = lag;
            bestCorrelation = correlations[lag];
            break;
        }
    }

    if (!(bestCorrelation > 0.24f)) {
        return estimate;
    }

    float refinedLag = static_cast<float>(bestLag);
    if (bestLag > minimumLag && bestLag < maximumLag) {
        const float left = correlations[bestLag - 1];
        const float center = correlations[bestLag];
        const float right = correlations[bestLag + 1];
        const float denominator = left - 2.0f * center + right;
        if (std::abs(denominator) > 1.0e-6f) {
            refinedLag += std::clamp(0.5f * (left - right) / denominator, -0.5f, 0.5f);
        }
    }

    estimate.hertz = static_cast<float>(sampleRate) / refinedLag;
    estimate.confidence = std::clamp((bestCorrelation - 0.24f) / 0.76f, 0.0f, 1.0f);
    return estimate;
}

std::vector<ReferencePitchPoint> makePitchTrajectory(
    const std::vector<float>& samples,
    const std::vector<float>& envelope,
    std::uint32_t sampleRate,
    const ReferenceAnalysisOptions& options) {
    std::vector<ReferencePitchPoint> points;
    if (samples.empty()) {
        return points;
    }

    std::uint32_t pitchRate = 0;
    const auto pitchSignal = makePitchSignal(samples, sampleRate, options.maximumPitchHz, pitchRate);
    const std::size_t hop = std::max<std::size_t>(1, millisecondsToSamples(options.pitchHopMs, sampleRate));
    const float envelopePeak = *std::max_element(envelope.begin(), envelope.end());

    for (std::size_t sample = 0; sample < samples.size(); sample += hop) {
        const float relativeEnvelope = envelopePeak > 0.0f ? envelope[sample] / envelopePeak : 0.0f;
        if (sample > millisecondsToSamples(15.0f, sampleRate) && relativeEnvelope < 0.004f) {
            break;
        }
        const std::size_t pitchSample = std::min(
            pitchSignal.size() - 1,
            static_cast<std::size_t>(static_cast<double>(sample) * pitchRate / sampleRate));
        auto estimate = estimatePitchAt(pitchSignal,
                                        pitchRate,
                                        pitchSample,
                                        options.minimumPitchHz,
                                        options.maximumPitchHz,
                                        options.pitchWindowMs);
        estimate.confidence *= std::clamp(std::sqrt(relativeEnvelope) * 1.4f, 0.0f, 1.0f);
        if (estimate.confidence >= 0.08f &&
            estimate.hertz >= options.minimumPitchHz &&
            estimate.hertz <= options.maximumPitchHz) {
            points.push_back({samplesToMilliseconds(sample, sampleRate),
                              estimate.hertz,
                              estimate.confidence});
        }
    }

    // Reject isolated octave errors with a small median neighborhood, then
    // lightly smooth in log-frequency so genuine exponential drops survive.
    if (points.size() >= 3) {
        std::vector<float> corrected(points.size(), 0.0f);
        for (std::size_t index = 0; index < points.size(); ++index) {
            const float before = points[index > 0 ? index - 1 : index].hertz;
            const float current = points[index].hertz;
            const float after = points[index + 1 < points.size() ? index + 1 : index].hertz;
            std::array<float, 3> neighborhood {before, current, after};
            std::sort(neighborhood.begin(), neighborhood.end());
            const float median = neighborhood[1];
            corrected[index] =
                (current > median * 1.8f || current < median * 0.55f) ? median : current;
        }

        float smoothedLog = std::log(std::max(corrected.front(), 1.0f));
        for (std::size_t index = 0; index < points.size(); ++index) {
            const float currentLog = std::log(std::max(corrected[index], 1.0f));
            const float blend = index == 0 ? 1.0f : 0.68f;
            smoothedLog += blend * (currentLog - smoothedLog);
            points[index].hertz = std::exp(smoothedLog);
        }
    }
    return points;
}

std::vector<ReferenceAmplitudePoint> makeAmplitudeTrajectory(
    const std::vector<float>& envelope,
    std::uint32_t sampleRate,
    float hopMs) {
    std::vector<ReferenceAmplitudePoint> points;
    if (envelope.empty()) {
        return points;
    }
    const std::size_t hop = std::max<std::size_t>(1, millisecondsToSamples(hopMs, sampleRate));
    for (std::size_t sample = 0; sample < envelope.size(); sample += hop) {
        points.push_back({samplesToMilliseconds(sample, sampleRate),
                          envelope[sample],
                          linearToDecibels(envelope[sample])});
    }
    if ((envelope.size() - 1) % hop != 0) {
        points.push_back({samplesToMilliseconds(envelope.size() - 1, sampleRate),
                          envelope.back(),
                          linearToDecibels(envelope.back())});
    }
    return points;
}

float rmsInRange(const std::vector<float>& samples,
                 std::size_t begin,
                 std::size_t end) {
    begin = std::min(begin, samples.size());
    end = std::min(std::max(end, begin), samples.size());
    if (begin == end) {
        return 0.0f;
    }
    double energy = 0.0;
    for (std::size_t index = begin; index < end; ++index) {
        energy += static_cast<double>(samples[index]) * samples[index];
    }
    return static_cast<float>(std::sqrt(energy / static_cast<double>(end - begin)));
}

float spectralCentroid(const std::vector<float>& samples,
                       std::uint32_t sampleRate,
                       std::size_t sampleCount) {
    sampleCount = std::min(sampleCount, samples.size());
    if (sampleCount < 4 || sampleRate == 0) {
        return 0.0f;
    }

    std::size_t transformSize = 64;
    while (transformSize < sampleCount && transformSize < 2048) {
        transformSize *= 2;
    }
    sampleCount = std::min(sampleCount, transformSize);
    double weightedFrequency = 0.0;
    double totalMagnitude = 0.0;
    for (std::size_t bin = 1; bin <= transformSize / 2; ++bin) {
        double real = 0.0;
        double imaginary = 0.0;
        for (std::size_t index = 0; index < sampleCount; ++index) {
            const double window = sampleCount > 1
                ? 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(index) /
                                      static_cast<double>(sampleCount - 1))
                : 1.0;
            const double phase = 2.0 * kPi * static_cast<double>(bin * index) /
                                 static_cast<double>(transformSize);
            real += samples[index] * window * std::cos(phase);
            imaginary -= samples[index] * window * std::sin(phase);
        }
        const double magnitude = std::sqrt(real * real + imaginary * imaginary);
        const double frequency = static_cast<double>(bin) * sampleRate / transformSize;
        weightedFrequency += frequency * magnitude;
        totalMagnitude += magnitude;
    }
    return totalMagnitude > 1.0e-12
               ? static_cast<float>(weightedFrequency / totalMagnitude)
               : 0.0f;
}

struct SeparationWork {
    SeparatedKickComponents components;
    std::vector<float> highpassed;
    float airDecayMs = 7.0f;
};

SeparationWork separateComponents(const std::vector<float>& samples,
                                  std::uint32_t sampleRate,
                                  float settledPitchHz) {
    SeparationWork work;
    auto& output = work.components;
    output.transient.resize(samples.size(), 0.0f);
    output.body.resize(samples.size(), 0.0f);
    work.highpassed.resize(samples.size(), 0.0f);
    if (samples.empty()) {
        return work;
    }

    const float cutoff = std::clamp(settledPitchHz * 5.0f, 550.0f, 1800.0f);
    const float coefficient = 1.0f - std::exp(
        -2.0f * static_cast<float>(kPi) * cutoff / static_cast<float>(sampleRate));
    float lowpass = 0.0f;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        lowpass += coefficient * (samples[index] - lowpass);
        work.highpassed[index] = samples[index] - lowpass;
    }

    const std::size_t frame = std::max<std::size_t>(1, millisecondsToSamples(1.5f, sampleRate));
    const std::size_t searchEnd = std::min(samples.size(), millisecondsToSamples(30.0f, sampleRate));
    float peakHighRms = 0.0f;
    std::size_t peakFrame = 0;
    for (std::size_t begin = 0; begin < searchEnd; begin += frame) {
        const float highRms = rmsInRange(work.highpassed, begin, begin + frame);
        if (highRms > peakHighRms) {
            peakHighRms = highRms;
            peakFrame = begin;
        }
    }

    std::size_t transientEnd = std::min(samples.size(), millisecondsToSamples(8.0f, sampleRate));
    const std::size_t earliest = std::max(peakFrame + frame,
                                          millisecondsToSamples(2.0f, sampleRate));
    unsigned quietFrames = 0;
    for (std::size_t begin = earliest; begin < searchEnd; begin += frame) {
        const float highRms = rmsInRange(work.highpassed, begin, begin + frame);
        const float totalRms = rmsInRange(samples, begin, begin + frame);
        const float ratio = totalRms > kSilenceFloor ? highRms / totalRms : 0.0f;
        if (highRms < peakHighRms * 0.16f || ratio < 0.23f) {
            ++quietFrames;
            if (quietFrames >= 2) {
                transientEnd = std::min(samples.size(), begin + frame);
                break;
            }
        } else {
            quietFrames = 0;
        }
    }
    transientEnd = std::clamp(transientEnd,
                              std::min(samples.size(), millisecondsToSamples(1.0f, sampleRate)),
                              std::min(samples.size(), millisecondsToSamples(30.0f, sampleRate)));
    output.transientEndSample = transientEnd;

    const std::size_t fadeStart = transientEnd * 2 / 5;
    const float shortDecaySamples = std::max(1.0f, sampleRate * 0.0008f);
    for (std::size_t index = 0; index < samples.size(); ++index) {
        float gate = 0.0f;
        if (index < fadeStart) {
            gate = 1.0f;
        } else if (index < transientEnd && transientEnd > fadeStart) {
            const float position = static_cast<float>(index - fadeStart) /
                                   static_cast<float>(transientEnd - fadeStart);
            gate = 0.5f + 0.5f * std::cos(static_cast<float>(kPi) * position);
        }
        const float broadImpact = 0.18f * std::exp(-static_cast<float>(index) / shortDecaySamples);
        output.transient[index] = work.highpassed[index] * gate + samples[index] * broadImpact;
        output.body[index] = samples[index] - output.transient[index];
    }

    double transientEnergy = 0.0;
    double bodyEnergy = 0.0;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        transientEnergy += static_cast<double>(output.transient[index]) * output.transient[index];
        bodyEnergy += static_cast<double>(output.body[index]) * output.body[index];
    }
    const double componentEnergy = transientEnergy + bodyEnergy;
    if (componentEnergy > 1.0e-20) {
        output.transientEnergyFraction = static_cast<float>(transientEnergy / componentEnergy);
        output.bodyEnergyFraction = static_cast<float>(bodyEnergy / componentEnergy);
    }
    output.transientSpectralCentroidHz = spectralCentroid(
        output.transient, sampleRate, std::max(transientEnd, millisecondsToSamples(4.0f, sampleRate)));

    // Regress the high-frequency residual's log RMS. A straight line in this
    // domain is the time constant of an exponential air/noise decay.
    const std::size_t decayEnd = std::min(samples.size(), millisecondsToSamples(55.0f, sampleRate));
    const std::size_t decayHop = std::max<std::size_t>(1, millisecondsToSamples(2.0f, sampleRate));
    double sumTime = 0.0;
    double sumLog = 0.0;
    double sumTimeSquared = 0.0;
    double sumTimeLog = 0.0;
    std::size_t regressionCount = 0;
    for (std::size_t begin = std::min(peakFrame, decayEnd);
         begin + decayHop <= decayEnd;
         begin += decayHop) {
        const float value = rmsInRange(work.highpassed, begin, begin + decayHop);
        if (value <= std::max(peakHighRms * 0.01f, 1.0e-7f)) {
            continue;
        }
        const double timeMs = samplesToMilliseconds(begin + decayHop / 2, sampleRate);
        const double logValue = std::log(value);
        sumTime += timeMs;
        sumLog += logValue;
        sumTimeSquared += timeMs * timeMs;
        sumTimeLog += timeMs * logValue;
        ++regressionCount;
    }
    if (regressionCount >= 3) {
        const double denominator = regressionCount * sumTimeSquared - sumTime * sumTime;
        if (std::abs(denominator) > 1.0e-12) {
            const double slope = (regressionCount * sumTimeLog - sumTime * sumLog) / denominator;
            if (slope < -1.0e-4) {
                work.airDecayMs = static_cast<float>(std::clamp(-1.0 / slope, 1.0, 50.0));
            }
        }
    }
    return work;
}

float interpolatePitch(const std::vector<ReferencePitchPoint>& pitch, float timeMs) {
    if (pitch.empty()) {
        return 52.0f;
    }
    if (timeMs <= pitch.front().timeMs) {
        return pitch.front().hertz;
    }
    for (std::size_t index = 1; index < pitch.size(); ++index) {
        if (timeMs <= pitch[index].timeMs) {
            const auto& first = pitch[index - 1];
            const auto& second = pitch[index];
            const float span = second.timeMs - first.timeMs;
            const float amount = span > 0.0f ? (timeMs - first.timeMs) / span : 0.0f;
            const float firstLog = std::log(std::max(first.hertz, 1.0f));
            const float secondLog = std::log(std::max(second.hertz, 1.0f));
            return std::exp(firstLog + amount * (secondLog - firstLog));
        }
    }
    return pitch.back().hertz;
}

float envelopeAtMs(const std::vector<float>& envelope,
                   std::uint32_t sampleRate,
                   float timeMs) {
    if (envelope.empty()) {
        return 0.0f;
    }
    const std::size_t sample = std::min(envelope.size() - 1,
                                        millisecondsToSamples(timeMs, sampleRate));
    return envelope[sample];
}

float besselJ(unsigned order, float x) {
    const double halfX = static_cast<double>(x) * 0.5;
    double factorial = 1.0;
    for (unsigned value = 2; value <= order; ++value) {
        factorial *= static_cast<double>(value);
    }
    double term = std::pow(halfX, static_cast<int>(order)) / factorial;
    double sum = term;
    for (unsigned index = 1; index <= 14; ++index) {
        term *= -(halfX * halfX) /
                (static_cast<double>(index) * static_cast<double>(index + order));
        sum += term;
    }
    return static_cast<float>(sum);
}

struct ModalProjection {
    std::array<float, 3> magnitude {};
    float fundamentalSin = 0.0f;
    float fundamentalCos = 0.0f;
    float confidence = 0.0f;
};

ModalProjection projectMembraneModes(const std::vector<float>& body,
                                     const std::vector<float>& envelope,
                                     const std::vector<ReferencePitchPoint>& pitch,
                                     std::uint32_t sampleRate) {
    ModalProjection projection;
    if (body.empty() || pitch.empty() || sampleRate == 0) {
        return projection;
    }

    constexpr std::array<float, 3> ratios {1.0f, 1.59334f, 2.13555f};
    constexpr std::array<float, 3> decayMs {
        std::numeric_limits<float>::infinity(), 34.0f, 16.0f,
    };
    const std::size_t count = std::min(body.size(), millisecondsToSamples(90.0f, sampleRate));
    const float envelopePeak = *std::max_element(envelope.begin(), envelope.end());
    std::array<double, 3> sinDot {};
    std::array<double, 3> cosDot {};
    std::array<double, 3> sinNorm {};
    std::array<double, 3> cosNorm {};
    double phase = 0.0;
    double bodyEnergy = 0.0;

    for (std::size_t index = 0; index < count; ++index) {
        const float timeMs = samplesToMilliseconds(index, sampleRate);
        const float frequency = interpolatePitch(pitch, timeMs);
        phase += 2.0 * kPi * frequency / static_cast<double>(sampleRate);
        const float amplitude = envelopePeak > 0.0f ? envelope[index] / envelopePeak : 0.0f;
        const float analysisWindow = count > 1
            ? std::sin(static_cast<float>(kPi) * static_cast<float>(index) /
                       static_cast<float>(count - 1))
            : 1.0f;
        const float weight = amplitude * analysisWindow;
        bodyEnergy += static_cast<double>(body[index]) * body[index] * analysisWindow;
        for (std::size_t mode = 0; mode < ratios.size(); ++mode) {
            const float decay = std::isfinite(decayMs[mode])
                ? std::exp(-timeMs / decayMs[mode])
                : 1.0f;
            const double basisScale = weight * decay;
            const double sine = basisScale * std::sin(phase * ratios[mode]);
            const double cosine = basisScale * std::cos(phase * ratios[mode]);
            sinDot[mode] += body[index] * sine;
            cosDot[mode] += body[index] * cosine;
            sinNorm[mode] += sine * sine;
            cosNorm[mode] += cosine * cosine;
        }
    }

    for (std::size_t mode = 0; mode < ratios.size(); ++mode) {
        const float sinCoefficient = sinNorm[mode] > 1.0e-12
            ? static_cast<float>(sinDot[mode] / sinNorm[mode])
            : 0.0f;
        const float cosCoefficient = cosNorm[mode] > 1.0e-12
            ? static_cast<float>(cosDot[mode] / cosNorm[mode])
            : 0.0f;
        projection.magnitude[mode] =
            std::sqrt(sinCoefficient * sinCoefficient + cosCoefficient * cosCoefficient);
        if (mode == 0) {
            projection.fundamentalSin = sinCoefficient;
            projection.fundamentalCos = cosCoefficient;
        }
    }
    const double projectedEnergy =
        static_cast<double>(projection.magnitude[0]) * projection.magnitude[0] * count * 0.5;
    projection.confidence = bodyEnergy > 1.0e-12
        ? static_cast<float>(std::clamp(projectedEnergy / bodyEnergy, 0.0, 1.0))
        : 0.0f;
    return projection;
}

float fitStrikePosition(const ModalProjection& projection, float& confidence) {
    confidence = 0.0f;
    if (projection.magnitude[0] < 1.0e-6f) {
        return 0.25f;
    }

    const float observedSecond = std::clamp(
        projection.magnitude[1] / projection.magnitude[0], 0.0f, 2.0f);
    const float observedThird = std::clamp(
        projection.magnitude[2] / projection.magnitude[0], 0.0f, 2.0f);
    constexpr std::array<float, 3> roots {2.4048256f, 3.8317060f, 5.1356225f};
    float bestPosition = 0.25f;
    float bestError = std::numeric_limits<float>::max();
    for (unsigned step = 0; step <= 200; ++step) {
        const float position = static_cast<float>(step) / 200.0f;
        const float radius = position * 0.85f;
        const float fundamental = std::max(std::abs(besselJ(0, roots[0] * radius)), 1.0e-4f);
        const float second = std::abs(0.35f * besselJ(1, roots[1] * radius)) / fundamental;
        const float third = std::abs(0.22f * besselJ(2, roots[2] * radius)) / fundamental;
        const float secondError = std::log1p(second) - std::log1p(observedSecond);
        const float thirdError = std::log1p(third) - std::log1p(observedThird);
        const float error = secondError * secondError + 0.65f * thirdError * thirdError;
        if (error < bestError) {
            bestError = error;
            bestPosition = position;
        }
    }
    confidence = projection.confidence * std::exp(-4.0f * bestError);
    return bestPosition;
}

float averagePitchConfidence(const std::vector<ReferencePitchPoint>& pitch) {
    if (pitch.empty()) {
        return 0.0f;
    }
    double sum = 0.0;
    for (const auto& point : pitch) {
        sum += point.confidence;
    }
    return static_cast<float>(sum / static_cast<double>(pitch.size()));
}

FittedPhysicalKick fitPhysicalKick(const std::vector<float>& samples,
                                   const std::vector<float>& envelope,
                                   const std::vector<ReferencePitchPoint>& pitch,
                                   const SeparationWork& separation,
                                   std::uint32_t sampleRate,
                                   float sourcePeak,
                                   float activeDurationMs) {
    FittedPhysicalKick fit;
    const float duration = std::clamp(activeDurationMs, 10.0f, 2000.0f);

    float secondPitchTime = std::min(18.0f, std::max(0.1f, duration * 0.12f));
    float settleTime = std::min(65.0f, std::max(secondPitchTime + 1.0f, duration * 0.3f));
    if (pitch.size() >= 4) {
        for (std::size_t index = 1; index + 2 < pitch.size(); ++index) {
            if (pitch[index].timeMs < secondPitchTime + 5.0f) {
                continue;
            }
            const float minimum = std::min({pitch[index].hertz,
                                            pitch[index + 1].hertz,
                                            pitch[index + 2].hertz});
            const float maximum = std::max({pitch[index].hertz,
                                            pitch[index + 1].hertz,
                                            pitch[index + 2].hertz});
            if (minimum > 0.0f && maximum / minimum < 1.08f) {
                settleTime = pitch[index].timeMs;
                break;
            }
        }
    }
    settleTime = std::clamp(settleTime, secondPitchTime + 1.0f, std::min(300.0f, duration - 0.02f));
    const float pitchEndTime = std::clamp(duration, settleTime + 0.01f, 1000.0f);

    float initialPitch = interpolatePitch(pitch, 0.0f);
    if (pitch.size() >= 2 && pitch.front().timeMs > 0.0f && pitch.front().timeMs < 25.0f) {
        const float span = pitch[1].timeMs - pitch[0].timeMs;
        if (span > 0.0f) {
            const float slope = (std::log(pitch[1].hertz) - std::log(pitch[0].hertz)) / span;
            initialPitch = std::exp(std::log(pitch[0].hertz) - slope * pitch[0].timeMs);
        }
    }
    fit.pitchHz = {{{0.0f, std::clamp(initialPitch, 20.0f, 1000.0f), -0.35f},
                    {secondPitchTime,
                     std::clamp(interpolatePitch(pitch, secondPitchTime), 20.0f, 1000.0f),
                     -0.20f},
                    {settleTime,
                     std::clamp(interpolatePitch(pitch, settleTime), 20.0f, 1000.0f),
                     0.0f},
                    {pitchEndTime,
                     std::clamp(interpolatePitch(pitch, pitchEndTime), 20.0f, 1000.0f),
                     0.0f}}};

    const float envelopePeak = envelope.empty()
        ? 0.0f
        : *std::max_element(envelope.begin(), envelope.end());
    const std::size_t attackSearchEnd = std::min(
        envelope.size(), std::max<std::size_t>(1, millisecondsToSamples(20.0f, sampleRate)));
    std::size_t peakSample = 0;
    for (std::size_t index = 1; index < attackSearchEnd; ++index) {
        if (envelope[index] > envelope[peakSample]) {
            peakSample = index;
        }
    }
    const float peakUpper = std::max(0.1f, std::min(20.0f, duration - 2.0f));
    const float peakTime = std::clamp(samplesToMilliseconds(peakSample, sampleRate),
                                      0.1f,
                                      peakUpper);
    const float decayUpper = std::max(peakTime + 0.01f,
                                      std::min(500.0f, duration - 0.01f));
    const float decayTime = std::clamp(std::max(peakTime + 1.0f, duration * 0.30f),
                                       peakTime + 0.01f,
                                       decayUpper);
    const float amplitudeEndTime = std::clamp(duration, decayTime + 0.01f, 2000.0f);
    const auto relativeDb = [&](float timeMs) {
        return envelopePeak > 0.0f
            ? std::clamp(linearToDecibels(envelopeAtMs(envelope, sampleRate, timeMs) /
                                          envelopePeak), -60.0f, 6.0f)
            : -60.0f;
    };
    fit.amplitudeDb = {{{0.0f, relativeDb(0.0f), -0.60f},
                        {peakTime, 0.0f, 0.15f},
                        {decayTime, relativeDb(decayTime), 0.25f},
                        {amplitudeEndTime, relativeDb(amplitudeEndTime), 0.0f}}};

    const ModalProjection modal = projectMembraneModes(
        separation.components.body, envelope, pitch, sampleRate);
    float strikeConfidence = 0.0f;
    fit.strikePosition = fitStrikePosition(modal, strikeConfidence);
    fit.fundamentalSinProjection = modal.fundamentalSin;
    fit.fundamentalCosProjection = modal.fundamentalCos;
    fit.phaseAtOnsetDegrees = static_cast<float>(
        std::atan2(modal.fundamentalCos, modal.fundamentalSin) * 180.0 / kPi);
    if (fit.phaseAtOnsetDegrees < 0.0f) {
        fit.phaseAtOnsetDegrees += 360.0f;
    }
    fit.phaseReferenceTimeMs = 0.0f;
    fit.phaseConfidence = modal.confidence * averagePitchConfidence(pitch);

    fit.impactLevel = std::clamp(
        0.08f + 0.92f * std::sqrt(separation.components.transientEnergyFraction),
        0.0f,
        1.0f);
    const float postImpactHigh = rmsInRange(
        separation.highpassed,
        std::min(separation.components.transientEndSample, separation.highpassed.size()),
        std::min(separation.highpassed.size(), millisecondsToSamples(45.0f, sampleRate)));
    const float postImpactTotal = rmsInRange(
        samples,
        std::min(separation.components.transientEndSample, samples.size()),
        std::min(samples.size(), millisecondsToSamples(45.0f, sampleRate)));
    fit.airLevel = postImpactTotal > kSilenceFloor
        ? std::clamp(0.55f * postImpactHigh / postImpactTotal, 0.0f, 1.0f)
        : 0.0f;
    fit.airDecayMs = std::clamp(separation.airDecayMs, 1.0f, 50.0f);
    fit.beaterHardnessHz = std::clamp(
        separation.components.transientSpectralCentroidHz, 200.0f, 16000.0f);
    fit.outputGain = std::clamp(sourcePeak, 0.05f, 1.0f);

    const float pitchConfidence = averagePitchConfidence(pitch);
    const float signalConfidence = std::clamp(sourcePeak * 4.0f, 0.0f, 1.0f);
    fit.fitConfidence = std::clamp(
        0.55f * pitchConfidence + 0.25f * strikeConfidence + 0.20f * signalConfidence,
        0.0f,
        1.0f);
    return fit;
}

ExtractedTransientSample makeTransientSample(const SeparatedKickComponents& components,
                                             std::uint32_t sampleRate) {
    ExtractedTransientSample sample;
    sample.sampleRate = sampleRate;
    const std::size_t tail = millisecondsToSamples(2.0f, sampleRate);
    const std::size_t count = std::min(components.transient.size(),
                                       components.transientEndSample + tail);
    sample.monoSamples.assign(components.transient.begin(),
                              components.transient.begin() + count);
    if (sample.monoSamples.empty()) {
        return sample;
    }

    const std::size_t fadeCount = std::min(tail, sample.monoSamples.size());
    const std::size_t fadeStart = sample.monoSamples.size() - fadeCount;
    for (std::size_t index = fadeStart; index < sample.monoSamples.size(); ++index) {
        const float amount = fadeCount > 1
            ? static_cast<float>(sample.monoSamples.size() - 1 - index) /
              static_cast<float>(fadeCount - 1)
            : 0.0f;
        sample.monoSamples[index] *= amount;
    }
    sample.sourcePeak = vectorPeak(sample.monoSamples);
    if (sample.sourcePeak > 1.0e-7f) {
        const float normalization = 0.98f / sample.sourcePeak;
        for (float& value : sample.monoSamples) {
            value *= normalization;
        }
        sample.suggestedGain = sample.sourcePeak / 0.98f;
    }
    return sample;
}

bool validAnalysisOptions(const ReferenceAnalysisOptions& options) {
    return options.waveformPointCount > 0 &&
           options.waveformPointCount <= 65536 &&
           std::isfinite(options.minimumPitchHz) &&
           std::isfinite(options.maximumPitchHz) &&
           options.minimumPitchHz >= 15.0f &&
           options.maximumPitchHz > options.minimumPitchHz &&
           options.maximumPitchHz <= 5000.0f &&
           std::isfinite(options.pitchHopMs) &&
           options.pitchHopMs >= 1.0f &&
           options.pitchHopMs <= 100.0f &&
           std::isfinite(options.pitchWindowMs) &&
           options.pitchWindowMs >= 10.0f &&
           options.pitchWindowMs <= 200.0f &&
           std::isfinite(options.maximumDurationMs) &&
           options.maximumDurationMs >= 20.0f &&
           options.maximumDurationMs <= 10000.0f;
}

} // namespace

double ReferenceAudio::durationSeconds() const {
    return sampleRate > 0
               ? static_cast<double>(monoSamples.size()) / static_cast<double>(sampleRate)
               : 0.0;
}

KickRegion KickRegion::strongestKick() {
    return {};
}

KickRegion KickRegion::samples(std::size_t start, std::size_t count) {
    KickRegion region;
    region.mode = KickRegionMode::ExplicitSamples;
    region.startSample = start;
    region.sampleCount = count;
    return region;
}

WavDecodeResult decodeWav(const std::uint8_t* bytes,
                          std::size_t byteCount,
                          const WavDecodeLimits& limits) {
    if ((bytes == nullptr && byteCount != 0) || byteCount < 12 ||
        !matchesFourCC(bytes, "RIFF") || !matchesFourCC(bytes + 8, "WAVE")) {
        return wavFailure(WavDecodeError::NotRiffWave,
                          "The file is not a little-endian RIFF/WAVE file");
    }
    if (byteCount > limits.maximumFileBytes) {
        return wavFailure(WavDecodeError::FileTooLarge,
                          "The WAV exceeds the configured file-size limit");
    }

    const std::uint32_t riffPayloadSize = readU32(bytes + 4);
    if (riffPayloadSize < 4 || static_cast<std::size_t>(riffPayloadSize) > byteCount - 8) {
        return wavFailure(WavDecodeError::TruncatedChunk,
                          "The RIFF payload extends beyond the supplied bytes");
    }
    const std::size_t riffEnd = 8 + static_cast<std::size_t>(riffPayloadSize);

    const std::uint8_t* format = nullptr;
    std::size_t formatSize = 0;
    const std::uint8_t* data = nullptr;
    std::size_t dataSize = 0;
    for (std::size_t position = 12; position + 8 <= riffEnd;) {
        const std::uint32_t chunkSize32 = readU32(bytes + position + 4);
        const std::size_t chunkSize = static_cast<std::size_t>(chunkSize32);
        const std::size_t payload = position + 8;
        if (chunkSize > riffEnd - payload) {
            return wavFailure(WavDecodeError::TruncatedChunk,
                              "A WAV chunk extends beyond the RIFF payload");
        }
        if (matchesFourCC(bytes + position, "fmt ") && format == nullptr) {
            format = bytes + payload;
            formatSize = chunkSize;
        } else if (matchesFourCC(bytes + position, "data") && data == nullptr) {
            data = bytes + payload;
            dataSize = chunkSize;
        }

        const std::size_t padding = chunkSize & 1u;
        if (chunkSize > std::numeric_limits<std::size_t>::max() - payload - padding ||
            payload + chunkSize + padding > riffEnd) {
            return wavFailure(WavDecodeError::TruncatedChunk,
                              "A WAV chunk has invalid padding or size");
        }
        position = payload + chunkSize + padding;
    }

    if (format == nullptr) {
        return wavFailure(WavDecodeError::MissingFormatChunk, "The WAV has no format chunk");
    }
    if (data == nullptr) {
        return wavFailure(WavDecodeError::MissingDataChunk, "The WAV has no data chunk");
    }
    if (formatSize < 16) {
        return wavFailure(WavDecodeError::InvalidFormat, "The WAV format chunk is too short");
    }

    std::uint16_t formatTag = readU16(format);
    const std::uint16_t channels = readU16(format + 2);
    const std::uint32_t sampleRate = readU32(format + 4);
    const std::uint16_t blockAlignment = readU16(format + 12);
    const std::uint16_t bitsPerSample = readU16(format + 14);
    std::uint16_t validBits = bitsPerSample;
    if (formatTag == 0xfffeu) {
        if (formatSize < 40 || readU16(format + 16) < 22) {
            return wavFailure(WavDecodeError::InvalidFormat,
                              "The extensible WAV format chunk is incomplete");
        }
        validBits = readU16(format + 18);
        formatTag = readU16(format + 24);
        // KSDATAFORMAT_SUBTYPE_PCM/FLOAT share this canonical GUID tail.
        constexpr std::array<std::uint8_t, 12> canonicalTail {
            0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
            0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
        };
        if (readU16(format + 26) != 0 ||
            !std::equal(canonicalTail.begin(), canonicalTail.end(), format + 28)) {
            return wavFailure(WavDecodeError::UnsupportedEncoding,
                              "The extensible WAV subtype is not PCM or IEEE float");
        }
    }

    const bool isInteger = formatTag == 1;
    const bool isFloat = formatTag == 3;
    if ((!isInteger && !isFloat) ||
        (isInteger && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) ||
        (isFloat && bitsPerSample != 32)) {
        return wavFailure(WavDecodeError::UnsupportedEncoding,
                          "Only PCM16/24/32 and IEEE float32 WAV data is supported");
    }
    if (channels == 0 || channels > 64 || sampleRate < 1000 || sampleRate > 768000 ||
        validBits == 0 || validBits > bitsPerSample ||
        (isFloat && validBits != 32)) {
        return wavFailure(WavDecodeError::InvalidFormat,
                          "The WAV channel count, sample rate, or sample precision is invalid");
    }
    const std::size_t bytesPerSample = bitsPerSample / 8;
    const std::size_t minimumFrameBytes = static_cast<std::size_t>(channels) * bytesPerSample;
    if (blockAlignment < minimumFrameBytes || blockAlignment == 0 || dataSize % blockAlignment != 0) {
        return wavFailure(WavDecodeError::InvalidFormat,
                          "The WAV block alignment or data length is invalid");
    }
    const std::size_t frameCount = dataSize / blockAlignment;
    if (frameCount > limits.maximumFrames) {
        return wavFailure(WavDecodeError::TooManySamples,
                          "The WAV exceeds the configured decoded-frame limit");
    }

    ReferenceAudio audio;
    audio.sampleRate = sampleRate;
    audio.sourceChannelCount = channels;
    audio.sourceBitsPerSample = bitsPerSample;
    audio.sourceEncoding = isFloat ? WavSampleEncoding::IeeeFloat
                                   : WavSampleEncoding::PcmInteger;
    audio.monoSamples.resize(frameCount, 0.0f);
    const unsigned unusedBits = bitsPerSample - validBits;
    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        const std::uint8_t* frameBytes = data + frame * blockAlignment;
        double sum = 0.0;
        for (std::size_t channel = 0; channel < channels; ++channel) {
            const std::uint8_t* sampleBytes = frameBytes + channel * bytesPerSample;
            float value = 0.0f;
            if (isFloat) {
                std::uint32_t raw = readU32(sampleBytes);
                std::memcpy(&value, &raw, sizeof(value));
                if (!std::isfinite(value)) {
                    value = 0.0f;
                }
            } else if (bitsPerSample == 16) {
                std::int32_t integer = static_cast<std::int16_t>(readU16(sampleBytes));
                integer >>= unusedBits;
                value = static_cast<float>(integer) /
                        static_cast<float>(std::uint32_t {1} << (validBits - 1));
            } else if (bitsPerSample == 24) {
                std::int32_t integer = static_cast<std::int32_t>(sampleBytes[0]) |
                                       (static_cast<std::int32_t>(sampleBytes[1]) << 8) |
                                       (static_cast<std::int32_t>(sampleBytes[2]) << 16);
                if ((integer & 0x00800000) != 0) {
                    integer |= static_cast<std::int32_t>(0xff000000);
                }
                integer >>= unusedBits;
                value = static_cast<float>(integer) /
                        static_cast<float>(std::uint32_t {1} << (validBits - 1));
            } else {
                std::int32_t integer = static_cast<std::int32_t>(readU32(sampleBytes));
                integer >>= unusedBits;
                value = static_cast<float>(static_cast<double>(integer) /
                        std::ldexp(1.0, validBits - 1));
            }
            sum += std::clamp(value, -1.0f, 1.0f);
        }
        audio.monoSamples[frame] = static_cast<float>(sum / static_cast<double>(channels));
    }

    WavDecodeResult result;
    result.audio = std::move(audio);
    return result;
}

WavDecodeResult decodeWav(const std::vector<std::uint8_t>& bytes,
                          const WavDecodeLimits& limits) {
    return decodeWav(bytes.data(), bytes.size(), limits);
}

WavDecodeResult loadWavFile(const std::string& path, const WavDecodeLimits& limits) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return wavFailure(WavDecodeError::IoError, "Could not open the WAV file");
    }
    const std::streampos end = stream.tellg();
    if (end < 0) {
        return wavFailure(WavDecodeError::IoError, "Could not determine the WAV file size");
    }
    const auto unsignedSize = static_cast<unsigned long long>(end);
    if (unsignedSize > limits.maximumFileBytes ||
        unsignedSize > std::numeric_limits<std::size_t>::max()) {
        return wavFailure(WavDecodeError::FileTooLarge,
                          "The WAV exceeds the configured file-size limit");
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(unsignedSize));
    stream.seekg(0, std::ios::beg);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()),
                                       static_cast<std::streamsize>(bytes.size()))) {
        return wavFailure(WavDecodeError::IoError, "Could not read the complete WAV file");
    }
    return decodeWav(bytes, limits);
}

SelectedKickRegion selectStrongestKickRegion(
    const ReferenceAudio& audio,
    const ReferenceAnalysisOptions& options) {
    SelectedKickRegion selected;
    if (audio.sampleRate == 0 || audio.monoSamples.empty() || !validAnalysisOptions(options)) {
        return selected;
    }

    const std::size_t maximumCount = std::max<std::size_t>(
        1, millisecondsToSamples(options.maximumDurationMs, audio.sampleRate));
    const auto& samples = audio.monoSamples;
    const float lowpassCoefficient = 1.0f - std::exp(
        -2.0f * static_cast<float>(kPi) * 230.0f / static_cast<float>(audio.sampleRate));
    const float fastCoefficient = coefficientForMs(3.0f, static_cast<float>(audio.sampleRate));
    const float slowCoefficient = coefficientForMs(75.0f, static_cast<float>(audio.sampleRate));
    float lowpassOne = 0.0f;
    float lowpassTwo = 0.0f;
    float lowFast = 0.0f;
    float lowSlow = 0.0f;
    float totalFast = 0.0f;
    float smoothedScore = 0.0f;
    const float scoreCoefficient = coefficientForMs(1.5f, static_cast<float>(audio.sampleRate));
    float bestScore = 0.0f;
    double scoreEnergy = 0.0;
    std::size_t bestSample = 0;

    for (std::size_t index = 0; index < samples.size(); ++index) {
        lowpassOne += lowpassCoefficient * (samples[index] - lowpassOne);
        lowpassTwo += lowpassCoefficient * (lowpassOne - lowpassTwo);
        const float lowEnergy = lowpassTwo * lowpassTwo;
        const float totalEnergy = samples[index] * samples[index];
        lowFast += fastCoefficient * (lowEnergy - lowFast);
        lowSlow += slowCoefficient * (lowEnergy - lowSlow);
        totalFast += fastCoefficient * (totalEnergy - totalFast);
        const float onset = std::max(0.0f, std::sqrt(std::max(lowFast, 0.0f)) -
                                                   std::sqrt(std::max(lowSlow, 0.0f)));
        const float lowFrequencyWeight = std::sqrt(
            std::clamp(lowFast / std::max(totalFast, 1.0e-12f), 0.0f, 1.0f));
        const float score = onset * (0.35f + 0.65f * lowFrequencyWeight) *
                            std::sqrt(std::max(lowFast, 0.0f));
        smoothedScore += scoreCoefficient * (score - smoothedScore);
        scoreEnergy += static_cast<double>(smoothedScore) * smoothedScore;
        if (smoothedScore > bestScore) {
            bestScore = smoothedScore;
            bestSample = index;
        }
    }

    const std::size_t lookback = millisecondsToSamples(35.0f, audio.sampleRate);
    const std::size_t searchStart = bestSample > lookback ? bestSample - lookback : 0;
    const std::size_t onsetFrame = std::max<std::size_t>(
        1, millisecondsToSamples(1.0f, audio.sampleRate));
    const float peakRms = rmsInRange(samples,
                                     bestSample > onsetFrame ? bestSample - onsetFrame : 0,
                                     std::min(samples.size(), bestSample + onsetFrame));
    const float threshold = std::max(peakRms * 0.12f, 1.0e-5f);
    std::size_t onset = bestSample;
    while (onset > searchStart) {
        const std::size_t frameStart = onset > onsetFrame ? onset - onsetFrame : 0;
        if (rmsInRange(samples, frameStart, onset) <= threshold) {
            onset = frameStart;
            break;
        }
        onset = std::max(searchStart, frameStart);
    }
    const std::size_t preroll = millisecondsToSamples(0.5f, audio.sampleRate);
    onset = onset > preroll ? onset - preroll : 0;

    selected.startSample = onset;
    selected.sampleCount = std::min(maximumCount, samples.size() - onset);
    const float scoreRms = samples.empty()
        ? 0.0f
        : static_cast<float>(std::sqrt(scoreEnergy / static_cast<double>(samples.size())));
    selected.confidence = bestScore > 0.0f
        ? std::clamp(bestScore / std::max(bestScore + scoreRms * 3.0f, 1.0e-12f), 0.0f, 1.0f)
        : 0.0f;
    return selected;
}

KickAnalysisResult analyzeReferenceKick(
    const ReferenceAudio& audio,
    KickRegion region,
    const ReferenceAnalysisOptions& options) {
    if (audio.monoSamples.empty()) {
        return analysisFailure(KickAnalysisError::EmptyAudio, "The reference contains no samples");
    }
    if (audio.sampleRate < 1000 || audio.sampleRate > 768000) {
        return analysisFailure(KickAnalysisError::InvalidSampleRate,
                               "The reference sample rate is invalid");
    }
    if (!validAnalysisOptions(options) ||
        options.maximumPitchHz >= static_cast<float>(audio.sampleRate) * 0.45f) {
        return analysisFailure(KickAnalysisError::InvalidOptions,
                               "The reference-analysis options are invalid for this sample rate");
    }

    SelectedKickRegion selected;
    if (region.mode == KickRegionMode::AutoStrongestKick) {
        selected = selectStrongestKickRegion(audio, options);
    } else {
        if (region.startSample >= audio.monoSamples.size()) {
            return analysisFailure(KickAnalysisError::InvalidRegion,
                                   "The selected kick region begins beyond the reference");
        }
        selected.startSample = region.startSample;
        selected.sampleCount = region.sampleCount == 0
            ? audio.monoSamples.size() - region.startSample
            : std::min(region.sampleCount, audio.monoSamples.size() - region.startSample);
        selected.confidence = 1.0f;
    }
    const std::size_t maximumCount = std::max<std::size_t>(
        1, millisecondsToSamples(options.maximumDurationMs, audio.sampleRate));
    selected.sampleCount = std::min(selected.sampleCount, maximumCount);
    if (selected.sampleCount == 0 || selected.startSample >= audio.monoSamples.size()) {
        return analysisFailure(KickAnalysisError::InvalidRegion,
                               "No samples remain in the selected kick region");
    }

    std::vector<float> samples(
        audio.monoSamples.begin() + selected.startSample,
        audio.monoSamples.begin() + selected.startSample + selected.sampleCount);
    const float initialPeak = vectorPeak(samples);
    if (initialPeak < 1.0e-5f) {
        return analysisFailure(KickAnalysisError::SilentRegion,
                               "The selected kick region is effectively silent");
    }

    const std::size_t onset = findOnset(samples, audio.sampleRate, initialPeak);
    if (onset > 0) {
        selected.startSample += onset;
        samples.erase(samples.begin(), samples.begin() + onset);
    }

    // Remove file DC before trajectory/separation work. The source remains
    // amplitude-calibrated; only its non-audio DC component is discarded.
    const double mean = std::accumulate(samples.begin(), samples.end(), 0.0) /
                        static_cast<double>(samples.size());
    for (float& sample : samples) {
        sample = std::clamp(static_cast<float>(sample - mean), -1.0f, 1.0f);
    }
    const float sourcePeak = vectorPeak(samples);
    auto envelope = makeEnvelope(samples, audio.sampleRate);
    const float envelopePeak = *std::max_element(envelope.begin(), envelope.end());
    const float activeThreshold = std::max(envelopePeak * 0.0015f, 1.0e-6f);
    std::size_t lastActive = 0;
    for (std::size_t index = 0; index < envelope.size(); ++index) {
        if (envelope[index] >= activeThreshold) {
            lastActive = index;
        }
    }
    const std::size_t minimumLength = std::min(samples.size(),
                                               millisecondsToSamples(20.0f, audio.sampleRate));
    const std::size_t tail = millisecondsToSamples(5.0f, audio.sampleRate);
    const std::size_t activeCount = std::min(samples.size(),
        std::max(minimumLength, std::min(samples.size(), lastActive + 1 + tail)));
    samples.resize(activeCount);
    envelope.resize(activeCount);
    selected.sampleCount = activeCount;

    KickAnalysisResult result;
    auto& analysis = result.analysis;
    analysis.sourceRegion = selected;
    analysis.analyzedMonoSamples = samples;
    analysis.sourcePeak = sourcePeak;
    analysis.activeDurationMs = samplesToMilliseconds(samples.size(), audio.sampleRate);
    analysis.waveform = makeWaveform(samples, audio.sampleRate, options.waveformPointCount);
    analysis.pitch = makePitchTrajectory(samples, envelope, audio.sampleRate, options);
    analysis.amplitude = makeAmplitudeTrajectory(envelope,
                                                 audio.sampleRate,
                                                 std::max(1.0f, options.pitchHopMs));

    float settledPitch = analysis.pitch.empty()
        ? 52.0f
        : interpolatePitch(analysis.pitch, std::min(analysis.activeDurationMs, 80.0f));
    SeparationWork separation = separateComponents(samples, audio.sampleRate, settledPitch);
    analysis.components = separation.components;
    analysis.fit = fitPhysicalKick(samples,
                                   envelope,
                                   analysis.pitch,
                                   separation,
                                   audio.sampleRate,
                                   sourcePeak,
                                   analysis.activeDurationMs);
    if (options.extractTransientSample) {
        analysis.transientSample = makeTransientSample(analysis.components, audio.sampleRate);
    }
    return result;
}

} // namespace KickDrum
