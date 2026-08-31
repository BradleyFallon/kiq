#include "Voice.h"

#include <algorithm>
#include <cmath>

namespace KickDrum {
namespace {
constexpr float kPi = 3.14159265358979323846f;

float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float wrapCycles(float cycles) {
    cycles -= std::floor(cycles);
    return cycles < 0.0f ? cycles + 1.0f : cycles;
}
}

Voice::Voice()
    : pitchTrajectory_(Trajectory::Scale::Logarithmic)
    , amplitudeTrajectory_(Trajectory::Scale::Linear)
    , params_(kDefaultKickParams)
    , note_(60)
    , velocity_(1.0f)
    , outputGainCurrent_(kDefaultKickParams.outputGain)
    , outputGainStep_(0.0f)
    , age_(0)
    , outputGainRampSamplesRemaining_(0)
    , sampleRate_(48000.0f)
    , pitchBendValue_(0.0f)
    , pitchBendRange_(2.0f)
    , fundamentalStartPhase_(0.0f)
    , nextSampleLayer_(nullptr)
    , activeSampleLayer_(nullptr)
    , nextSampleLayerRevision_(0)
    , activeSampleLayerRevision_(0)
    , sampleLayerPosition_(0.0)
    , sampleLayerIncrement_(1.0)
    , activeSampleLayerDurationMs_(0.0f)
    , initialized_(false)
    , active_(false) {
    setParams(params_);
}

void Voice::initialize(float sampleRate) {
    sampleRate_ = std::isfinite(sampleRate) && sampleRate > 0.0f
                      ? sampleRate
                      : 48000.0f;
    membrane_.initialize(sampleRate_);
    transient_.initialize(sampleRate_);
    initialized_ = true;
    stop();
}

void Voice::setSampleRate(float sampleRate) {
    initialize(sampleRate);
}

void Voice::setParams(const KickParams& params) {
    params_ = sanitizeKickParams(params);
    pitchTrajectory_.setPoints(params_.pitch);
    amplitudeTrajectory_.setPoints(params_.amplitude);
    transient_.setParams(params_.transient);
    membrane_.setStrikePosition(params_.strikePosition);
    fundamentalStartPhase_ = calculateFundamentalStartPhase();
    membrane_.setFundamentalPhase(fundamentalStartPhase_);
    outputGainCurrent_ = params_.outputGain;
    outputGainStep_ = 0.0f;
    outputGainRampSamplesRemaining_ = 0;
}

void Voice::setSampleLayer(const SampleLayerData* sampleLayer,
                           std::uint64_t revision) {
    nextSampleLayer_ = sampleLayer;
    nextSampleLayerRevision_ = sampleLayer ? revision : 0;
}

void Voice::setOutputGain(float gain) {
    const float target = std::isfinite(gain)
                             ? std::clamp(gain, 0.0f, 1.0f)
                             : kDefaultKickParams.outputGain;
    if (std::abs(params_.outputGain - target) <= 1.0e-6f) {
        return;
    }
    params_.outputGain = target;
    if (!initialized_ || !active_) {
        outputGainCurrent_ = params_.outputGain;
        outputGainStep_ = 0.0f;
        outputGainRampSamplesRemaining_ = 0;
        return;
    }

    outputGainRampSamplesRemaining_ = std::max(
        1, static_cast<int>(std::lround(sampleRate_ * 0.005f)));
    outputGainStep_ =
        (params_.outputGain - outputGainCurrent_) /
        static_cast<float>(outputGainRampSamplesRemaining_);
}

void Voice::trigger(int note, float velocity) {
    if (!initialized_) {
        return;
    }

    note_ = std::clamp(note, 0, 127);
    velocity_ = std::isfinite(velocity)
                    ? std::clamp(velocity, 0.0f, 1.0f)
                    : 0.0f;
    age_ = 0;
    activeSampleLayer_ = nextSampleLayer_;
    activeSampleLayerRevision_ = nextSampleLayerRevision_;
    sampleLayerPosition_ = 0.0;
    sampleLayerIncrement_ =
        activeSampleLayer_ &&
                std::isfinite(activeSampleLayer_->sourceSampleRate) &&
                activeSampleLayer_->sourceSampleRate > 0.0f
            ? static_cast<double>(activeSampleLayer_->sourceSampleRate) /
                  static_cast<double>(sampleRate_)
            : 1.0;
    activeSampleLayerDurationMs_ =
        activeSampleLayer_ && !activeSampleLayer_->samples.empty() &&
                params_.sampleLevel > 0.0f &&
                std::isfinite(activeSampleLayer_->sourceSampleRate) &&
                activeSampleLayer_->sourceSampleRate > 0.0f
            ? static_cast<float>(activeSampleLayer_->samples.size()) * 1000.0f /
                  activeSampleLayer_->sourceSampleRate
            : 0.0f;
    membrane_.setFundamentalPhase(fundamentalStartPhase_);
    membrane_.trigger();
    transient_.trigger();
    outputGainCurrent_ = params_.outputGain;
    outputGainStep_ = 0.0f;
    outputGainRampSamplesRemaining_ = 0;
    active_ = velocity_ > 0.0f;
}

void Voice::release() {
    // Kick hits are one-shots. A note-off must not truncate the body.
}

void Voice::stop() {
    active_ = false;
    age_ = 0;
    membrane_.trigger();
    transient_.trigger();
}

void Voice::setPitchBend(float bendValue, float bendRange) {
    pitchBendValue_ = std::isfinite(bendValue)
                          ? std::clamp(bendValue, -1.0f, 1.0f)
                          : 0.0f;
    pitchBendRange_ = std::isfinite(bendRange)
                          ? std::clamp(bendRange, 0.0f, 24.0f)
                          : 2.0f;
}

float Voice::currentTimeMs() const {
    return static_cast<float>(age_) * 1000.0f / sampleRate_;
}

float Voice::getCurrentPitchHz() const {
    const float bendRatio =
        std::pow(2.0f, (pitchBendValue_ * pitchBendRange_) / 12.0f);
    return pitchTrajectory_.valueAt(currentTimeMs()) * bendRatio;
}

float Voice::getCurrentAmplitudeDb() const {
    return amplitudeTrajectory_.valueAt(currentTimeMs());
}

float Voice::calculateFundamentalStartPhase() const {
    const double targetCycles = static_cast<double>(params_.phaseDegrees) / 360.0;
    if (params_.phaseLockMs < 0.0f) {
        return wrapCycles(static_cast<float>(targetCycles));
    }

    // Fixed-cost Gauss-Legendre integration calculates the nominal phase
    // accumulated along the curved pitch trajectory. Splitting at every
    // trajectory point keeps the result precise through the steep attack
    // without any per-sample work or unbounded loop on the audio thread.
    constexpr std::array<double, 8> nodes {
        -0.9602898564975363, -0.7966664774136267,
        -0.5255324099163290, -0.1834346424956498,
         0.1834346424956498,  0.5255324099163290,
         0.7966664774136267,  0.9602898564975363,
    };
    constexpr std::array<double, 8> weights {
        0.1012285362903763, 0.2223810344533745,
        0.3137066458778873, 0.3626837833783620,
        0.3626837833783620, 0.3137066458778873,
        0.2223810344533745, 0.1012285362903763,
    };

    const double lockMs = static_cast<double>(params_.phaseLockMs);
    double accumulatedCycles = 0.0;
    double intervalStart = 0.0;
    for (std::size_t segment = 1; segment <= kTrajectoryPointCount; ++segment) {
        const double intervalEnd = segment < kTrajectoryPointCount
                                       ? std::min(lockMs,
                                                  static_cast<double>(
                                                      params_.pitch[segment].timeMs))
                                       : lockMs;
        if (intervalEnd > intervalStart) {
            const double midpoint = (intervalStart + intervalEnd) * 0.5;
            const double halfWidth = (intervalEnd - intervalStart) * 0.5;
            double weightedHz = 0.0;
            for (std::size_t index = 0; index < nodes.size(); ++index) {
                const double timeMs = midpoint + halfWidth * nodes[index];
                weightedHz += weights[index] * static_cast<double>(
                    pitchTrajectory_.valueAt(static_cast<float>(timeMs)));
            }
            accumulatedCycles += weightedHz * halfWidth / 1000.0;
        }
        intervalStart = intervalEnd;
        if (intervalStart >= lockMs) {
            break;
        }
    }
    return wrapCycles(static_cast<float>(targetCycles - accumulatedCycles));
}

float Voice::sampleLayerDurationMs() const {
    return activeSampleLayerDurationMs_;
}

float Voice::renderSampleLayer() {
    if (!activeSampleLayer_ || activeSampleLayer_->samples.empty() ||
        params_.sampleLevel <= 0.0f || !std::isfinite(sampleLayerPosition_) ||
        sampleLayerPosition_ < 0.0) {
        return 0.0f;
    }

    const auto& samples = activeSampleLayer_->samples;
    if (sampleLayerPosition_ >= static_cast<double>(samples.size())) {
        return 0.0f;
    }

    const auto index = static_cast<std::size_t>(sampleLayerPosition_);
    const auto nextIndex = std::min(index + 1, samples.size() - 1);
    const float fraction = static_cast<float>(
        sampleLayerPosition_ - static_cast<double>(index));
    float value = samples[index] + (samples[nextIndex] - samples[index]) * fraction;

    // A short end window prevents a non-zero final source sample from making
    // a discontinuity when resampling reaches the end of the buffer.
    const double fadeSamples = std::min<double>(
        static_cast<double>(samples.size()),
        std::max(1.0, static_cast<double>(activeSampleLayer_->sourceSampleRate) *
                          0.002));
    const double remaining = static_cast<double>(samples.size()) - sampleLayerPosition_;
    if (remaining < fadeSamples) {
        const float fade = static_cast<float>(
            std::clamp(remaining / fadeSamples, 0.0, 1.0));
        value *= std::sin(fade * kPi * 0.5f);
    }

    sampleLayerPosition_ += sampleLayerIncrement_;
    return std::isfinite(value) ? value * params_.sampleLevel : 0.0f;
}

float Voice::renderSample() {
    if (!initialized_ || !active_) {
        return 0.0f;
    }

    const float timeMs = currentTimeMs();
    const float bodyDurationMs = amplitudeTrajectory_.durationMs();
    const float voiceDurationMs = std::max(
        {bodyDurationMs + kTailFadeMs, transient_.durationMs(),
         sampleLayerDurationMs()});
    if (timeMs >= voiceDurationMs) {
        active_ = false;
        return 0.0f;
    }

    float bodyTailGain = 1.0f;
    if (timeMs > bodyDurationMs) {
        const float fade = std::clamp(
            (timeMs - bodyDurationMs) / kTailFadeMs, 0.0f, 1.0f);
        bodyTailGain =
            0.5f * (1.0f + std::cos(3.14159265358979323846f * fade));
    }

    if (outputGainRampSamplesRemaining_ > 0) {
        outputGainCurrent_ += outputGainStep_;
        --outputGainRampSamplesRemaining_;
        if (outputGainRampSamplesRemaining_ == 0) {
            outputGainCurrent_ = params_.outputGain;
            outputGainStep_ = 0.0f;
        }
    }

    const float body = membrane_.renderSample(getCurrentPitchHz(), timeMs) *
                       dbToLinear(getCurrentAmplitudeDb()) * bodyTailGain *
                       params_.membraneLevel;
    const auto transientLayers = transient_.renderLayers(timeMs);
    const float sample = renderSampleLayer();
    ++age_;
    const float mixed = body + transientLayers.impact + transientLayers.air + sample;
    return (std::isfinite(mixed) ? mixed : 0.0f) * velocity_ * outputGainCurrent_;
}

} // namespace KickDrum
