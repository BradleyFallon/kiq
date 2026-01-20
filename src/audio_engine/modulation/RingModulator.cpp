#include "RingModulator.h"
#include <algorithm> // for std::clamp

namespace KickDrum {

RingModulator::RingModulator()
    : depth_(0.0f)
{
}

void RingModulator::setDepth(float depth) {
    // Clamp depth to valid range [0.0, 1.0]
    depth_ = std::clamp(depth, 0.0f, 1.0f);
}

float RingModulator::getDepth() const {
    return depth_;
}

float RingModulator::process(float carrier, float modulator) {
    // Ring modulation formula:
    // output = carrier × (1.0 - depth) + (carrier × modulator) × depth
    //
    // This provides a linear crossfade between:
    //   - Dry signal (carrier) when depth = 0.0
    //   - Fully ring-modulated signal (carrier × modulator) when depth = 1.0
    
    float dry = carrier;
    float wet = carrier * modulator;
    
    // Linear blend based on depth
    return dry * (1.0f - depth_) + wet * depth_;
}

} // namespace KickDrum
