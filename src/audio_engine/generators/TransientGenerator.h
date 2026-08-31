#pragma once

#include "NoiseGenerator.h"
#include "../parameters/KickParams.h"

namespace KickDrum {

class TransientGenerator {
public:
    TransientGenerator();

    void initialize(float sampleRate);
    void setParams(const TransientParams& params);
    void trigger();
    float renderSample(float timeMs);
    float durationMs() const;

private:
    void updateFilterCoefficient();

    NoiseGenerator noise_;
    TransientParams params_;
    float sampleRate_;
    float lowPassState_;
    float lowBandState_;
    float lowPassCoefficient_;
    float lowBandCoefficient_;
    float contactDurationMs_;
};

} // namespace KickDrum
