#pragma once

#include "../envelopes/Trajectory.h"
#include "../generators/SineDriver.h"
#include "../generators/TransientGenerator.h"
#include "../parameters/KickParams.h"

#include <cstdint>

namespace KickDrum {

/** A single deterministic, one-shot kick voice. */
class Voice {
public:
    Voice();

    void initialize(float sampleRate);
    void setSampleRate(float sampleRate);
    void setParams(const KickParams& params);
    const KickParams& getParams() const { return params_; }

    void trigger(int note, float velocity);
    void release();
    void stop();
    bool isActive() const { return active_; }

    int getNote() const { return note_; }
    std::uint64_t getAge() const { return age_; }
    float getCurrentPitchHz() const;
    float getCurrentAmplitudeDb() const;

    void setPitchBend(float bendValue, float bendRange);
    float getPitchBendValue() const { return pitchBendValue_; }
    float getPitchBendRange() const { return pitchBendRange_; }

    float renderSample();

private:
    float currentTimeMs() const;

    SineDriver sineDriver_;
    TransientGenerator transient_;
    PitchTrajectory pitchTrajectory_;
    AmplitudeTrajectory amplitudeTrajectory_;
    KickParams params_;
    int note_;
    float velocity_;
    std::uint64_t age_;
    float sampleRate_;
    float pitchBendValue_;
    float pitchBendRange_;
    bool initialized_;
    bool active_;
};

} // namespace KickDrum
