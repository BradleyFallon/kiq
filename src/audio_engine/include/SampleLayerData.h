#pragma once

#include <vector>

namespace KickDrum {

/** Immutable mono audio used by the optional sample layer. */
struct SampleLayerData {
    std::vector<float> samples;
    float sourceSampleRate = 48000.0f;
};

} // namespace KickDrum
