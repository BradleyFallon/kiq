#pragma once

#include "../envelopes/Trajectory.h"
#include "../generators/MembraneModel.h"
#include "../generators/TransientGenerator.h"
#include "../include/SampleLayerData.h"
#include "../parameters/KickParams.h"

#include <cstdint>

namespace KickDrum {

/** A single deterministic, one-shot kick voice. */
class Voice {
public:
    static constexpr float kTailFadeMs = 5.0f;

    Voice();

    void initialize(float sampleRate);
    void setSampleRate(float sampleRate);
    void setParams(const KickParams& params);
    /** Stage immutable sample data for the next trigger. Caller owns lifetime. */
    void setSampleLayer(const SampleLayerData* sampleLayer,
                        std::uint64_t revision = 0);
    void setOutputGain(float gain);
    const KickParams& getParams() const { return params_; }

    void trigger(int note, float velocity);
    void release();
    void stop();
    bool isActive() const { return active_; }

    int getNote() const { return note_; }
    std::uint64_t getAge() const { return age_; }
    float getCurrentPitchHz() const;
    float getCurrentAmplitudeDb() const;
    std::uint64_t getActiveSampleLayerRevision() const {
        return active_ && activeSampleLayer_ ? activeSampleLayerRevision_ : 0;
    }

    void setPitchBend(float bendValue, float bendRange);
    float getPitchBendValue() const { return pitchBendValue_; }
    float getPitchBendRange() const { return pitchBendRange_; }

    float renderSample();

private:
    float currentTimeMs() const;
    float calculateFundamentalStartPhase() const;
    float renderSampleLayer();
    float sampleLayerDurationMs() const;

    MembraneModel membrane_;
    TransientGenerator transient_;
    PitchTrajectory pitchTrajectory_;
    AmplitudeTrajectory amplitudeTrajectory_;
    KickParams params_;
    int note_;
    float velocity_;
    float outputGainCurrent_;
    float outputGainStep_;
    std::uint64_t age_;
    int outputGainRampSamplesRemaining_;
    float sampleRate_;
    float pitchBendValue_;
    float pitchBendRange_;
    float fundamentalStartPhase_;
    const SampleLayerData* nextSampleLayer_;
    const SampleLayerData* activeSampleLayer_;
    std::uint64_t nextSampleLayerRevision_;
    std::uint64_t activeSampleLayerRevision_;
    double sampleLayerPosition_;
    double sampleLayerIncrement_;
    float activeSampleLayerDurationMs_;
    bool initialized_;
    bool active_;
};

} // namespace KickDrum
