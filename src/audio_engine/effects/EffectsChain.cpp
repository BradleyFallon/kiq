#include "EffectsChain.h"

namespace KickDrum {

EffectsChain::EffectsChain()
    : compressor_()
    , reverb_()
    , compressorBypassed_(false)
    , reverbBypassed_(false)
    , initialized_(false)
{
}

void EffectsChain::initialize(float sampleRate) {
    compressor_.initialize(sampleRate);
    reverb_.initialize(sampleRate);
    initialized_ = true;
}

Compressor& EffectsChain::getCompressor() {
    return compressor_;
}

const Compressor& EffectsChain::getCompressor() const {
    return compressor_;
}

Reverb& EffectsChain::getReverb() {
    return reverb_;
}

const Reverb& EffectsChain::getReverb() const {
    return reverb_;
}

void EffectsChain::setCompressorBypassed(bool bypassed) {
    compressorBypassed_ = bypassed;
}

bool EffectsChain::isCompressorBypassed() const {
    return compressorBypassed_;
}

void EffectsChain::setReverbBypassed(bool bypassed) {
    reverbBypassed_ = bypassed;
}

bool EffectsChain::isReverbBypassed() const {
    return reverbBypassed_;
}

float EffectsChain::process(float input) {
    float output = input;
    
    // Apply compressor (first in chain) if not bypassed
    if (!compressorBypassed_) {
        output = compressor_.process(output);
    }
    
    // Apply reverb (second in chain) if not bypassed
    if (!reverbBypassed_) {
        output = reverb_.process(output);
    }
    
    return output;
}

void EffectsChain::reset() {
    compressor_.reset();
    reverb_.reset();
}

bool EffectsChain::isInitialized() const {
    return initialized_;
}

} // namespace KickDrum
