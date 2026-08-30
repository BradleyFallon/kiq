#include "TransientGenerator.h"

#include <algorithm>
#include <cmath>

namespace KickDrum {
namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kMinus60Db = 6.907755278982137f;
}

TransientGenerator::TransientGenerator()
    : params_(kDefaultKickParams.transient)
    , sampleRate_(48000.0f)
    , lowPassState_(0.0f)
    , lowPassCoefficient_(0.0f)
    , sampleIndex_(0) {
    updateFilterCoefficient();
}

void TransientGenerator::initialize(float sampleRate) {
    sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    updateFilterCoefficient();
    trigger();
}

void TransientGenerator::setParams(const TransientParams& params) {
    params_.clickLevel = std::clamp(params.clickLevel, 0.0f, 1.0f);
    params_.noiseLevel = std::clamp(params.noiseLevel, 0.0f, 1.0f);
    params_.noiseDecayMs = std::clamp(params.noiseDecayMs, 1.0f, 50.0f);
    params_.noiseToneHz = std::clamp(params.noiseToneHz, 200.0f, 16000.0f);
    updateFilterCoefficient();
}

void TransientGenerator::trigger() {
    noise_.reset();
    lowPassState_ = 0.0f;
    sampleIndex_ = 0;
}

float TransientGenerator::renderSample(float timeMs) {
    float click = 0.0f;
    if (sampleIndex_ == 0) {
        click = params_.clickLevel;
    } else if (sampleIndex_ == 1) {
        click = -0.5f * params_.clickLevel;
    }

    float noise = 0.0f;
    if (timeMs < params_.noiseDecayMs) {
        const float rawNoise = noise_.generate();
        lowPassState_ = (1.0f - lowPassCoefficient_) * rawNoise +
                        lowPassCoefficient_ * lowPassState_;
        const float envelope =
            std::exp(-kMinus60Db * timeMs / std::max(params_.noiseDecayMs, 0.001f));
        noise = lowPassState_ * envelope * params_.noiseLevel;
    }

    ++sampleIndex_;
    return click + noise;
}

float TransientGenerator::durationMs() const {
    return params_.noiseDecayMs;
}

void TransientGenerator::updateFilterCoefficient() {
    const float cutoff = std::min(params_.noiseToneHz, sampleRate_ * 0.45f);
    lowPassCoefficient_ = std::exp(-kTwoPi * cutoff / sampleRate_);
}

} // namespace KickDrum
