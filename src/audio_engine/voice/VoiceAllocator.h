#pragma once

#include "Voice.h"

#include <array>

namespace KickDrum {

/** Small overlap pool for one-shot kick hits. */
class VoiceAllocator {
public:
    // Nine voices cover the maximum 2.005 s body at the fastest 240 BPM
    // audition interval without hard-stealing a still-ringing hit.
    static constexpr int kMaxVoices = 9;

    VoiceAllocator();

    void initialize(float sampleRate);
    void setSampleRate(float sampleRate);
    void setParams(const KickParams& params);
    /** Update global post-voice state without rebuilding idle voices. */
    void setOutputStageParams(const OutputStageParams& params);
    /** Stage sample data for voices allocated after this call. */
    void setSampleLayer(const SampleLayerData* sampleLayer,
                        std::uint64_t revision = 0);
    const KickParams& getParams() const { return params_; }

    Voice* allocateVoice(int note, float velocity);
    void releaseVoice(int note);
    void releaseAll();
    void renderBuffer(float* buffer, int numSamples);

    int getNumVoices() const { return static_cast<int>(voices_.size()); }
    Voice& getVoice(int index) { return voices_[static_cast<std::size_t>(index)]; }
    const Voice& getVoice(int index) const {
        return voices_[static_cast<std::size_t>(index)];
    }
    int getNumActiveVoices() const;
    std::array<std::uint64_t, kMaxVoices>
    activeSampleLayerRevisions() const;

private:
    Voice* findIdleVoice();
    Voice* findOldestVoice();

    std::array<Voice, kMaxVoices> voices_;
    KickParams params_;
    const SampleLayerData* sampleLayer_;
    std::uint64_t sampleLayerRevision_;
    float sampleRate_;
    bool initialized_;
};

} // namespace KickDrum
