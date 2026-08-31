#pragma once

#include "NoiseGenerator.h"
#include "../parameters/KickParams.h"

namespace KickDrum {

struct TransientLayers {
    float impact = 0.0f;
    float air = 0.0f;
};

class TransientGenerator {
public:
    TransientGenerator();

    void initialize(float sampleRate);
    void setParams(const TransientParams& params);
    void trigger();
    /** Render independently gain-controlled impact and air layers. */
    TransientLayers renderLayers(float timeMs);
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
