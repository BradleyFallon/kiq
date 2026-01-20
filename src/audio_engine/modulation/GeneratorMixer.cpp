#include "GeneratorMixer.h"
#include <algorithm> // for std::clamp

namespace KickDrum {

GeneratorMixer::GeneratorMixer()
    : sineLevel_(0.0f)
    , harmonicLevel_(0.0f)
    , noiseLevel_(0.0f)
{
}

void GeneratorMixer::setSineLevel(float level) {
    // Clamp level to valid range [0.0, 1.0]
    sineLevel_ = std::clamp(level, 0.0f, 1.0f);
}

float GeneratorMixer::getSineLevel() const {
    return sineLevel_;
}

void GeneratorMixer::setHarmonicLevel(float level) {
    // Clamp level to valid range [0.0, 1.0]
    harmonicLevel_ = std::clamp(level, 0.0f, 1.0f);
}

float GeneratorMixer::getHarmonicLevel() const {
    return harmonicLevel_;
}

void GeneratorMixer::setNoiseLevel(float level) {
    // Clamp level to valid range [0.0, 1.0]
    noiseLevel_ = std::clamp(level, 0.0f, 1.0f);
}

float GeneratorMixer::getNoiseLevel() const {
    return noiseLevel_;
}

float GeneratorMixer::mix(float sine, float modulatedHarmonic, float modulatedNoise) {
    // Mix the three generator sources with independent level controls
    // Formula: output = sine × sineLevel + harmonics × harmonicLevel + noise × noiseLevel
    //
    // This implements Requirements 1.7, 4.2, 4.3, 4.4:
    //   - 1.7: Mix direct sine, ring-modulated harmonics, and ring-modulated noise
    //   - 4.2: Sine driver level parameter (0% to 100%)
    //   - 4.3: Harmonic level parameter (0% to 100%)
    //   - 4.4: Noise level parameter (0% to 100%)
    
    float output = sine * sineLevel_ 
                 + modulatedHarmonic * harmonicLevel_ 
                 + modulatedNoise * noiseLevel_;
    
    return output;
}

} // namespace KickDrum
