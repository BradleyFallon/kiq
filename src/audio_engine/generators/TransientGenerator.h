#pragma once

#include "NoiseGenerator.h"
#include "../parameters/KickParams.h"

#include <cstdint>

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
    float lowPassCoefficient_;
    std::uint32_t sampleIndex_;
};

} // namespace KickDrum
