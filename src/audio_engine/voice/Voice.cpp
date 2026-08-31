#include "Voice.h"

#include <algorithm>
#include <cmath>

namespace KickDrum {
namespace {
float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
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
    , initialized_(false)
    , active_(false) {
    setParams(params_);
}

void Voice::initialize(float sampleRate) {
    sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
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
    outputGainCurrent_ = params_.outputGain;
    outputGainStep_ = 0.0f;
    outputGainRampSamplesRemaining_ = 0;
}

void Voice::setOutputGain(float gain) {
    const float target = std::clamp(gain, 0.0f, 1.0f);
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
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    age_ = 0;
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
    pitchBendValue_ = std::clamp(bendValue, -1.0f, 1.0f);
    pitchBendRange_ = std::clamp(bendRange, 0.0f, 24.0f);
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

float Voice::renderSample() {
    if (!initialized_ || !active_) {
        return 0.0f;
    }

    const float timeMs = currentTimeMs();
    const float bodyDurationMs = amplitudeTrajectory_.durationMs();
    const float voiceDurationMs =
        std::max(bodyDurationMs + kTailFadeMs, transient_.durationMs());
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
                       dbToLinear(getCurrentAmplitudeDb()) * bodyTailGain;
    const float transient = transient_.renderSample(timeMs);
    ++age_;
    return (body + transient) * velocity_ * outputGainCurrent_;
}

} // namespace KickDrum
