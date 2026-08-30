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
    , age_(0)
    , sampleRate_(48000.0f)
    , pitchBendValue_(0.0f)
    , pitchBendRange_(2.0f)
    , initialized_(false)
    , active_(false) {
    setParams(params_);
}

void Voice::initialize(float sampleRate) {
    sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    sineDriver_.initialize(sampleRate_);
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
}

void Voice::trigger(int note, float velocity) {
    if (!initialized_) {
        return;
    }

    note_ = std::clamp(note, 0, 127);
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    age_ = 0;
    sineDriver_.reset();
    sineDriver_.setPhase(params_.startPhase);
    transient_.trigger();
    active_ = velocity_ > 0.0f;
}

void Voice::release() {
    // Kick hits are one-shots. A note-off must not truncate the body.
}

void Voice::stop() {
    active_ = false;
    age_ = 0;
    sineDriver_.reset();
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
    const float durationMs =
        std::max(amplitudeTrajectory_.durationMs(), transient_.durationMs());
    if (timeMs > durationMs) {
        active_ = false;
        return 0.0f;
    }

    sineDriver_.setFrequency(getCurrentPitchHz());
    const float body = sineDriver_.generate() * dbToLinear(getCurrentAmplitudeDb());
    const float transient = transient_.renderSample(timeMs);
    ++age_;
    return (body + transient) * velocity_ * params_.outputGain;
}

} // namespace KickDrum
