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
    , lowBandState_(0.0f)
    , lowPassCoefficient_(0.0f)
    , lowBandCoefficient_(0.0f)
    , contactDurationMs_(0.3f) {
    updateFilterCoefficient();
}

void TransientGenerator::initialize(float sampleRate) {
    sampleRate_ = sampleRate > 0.0f ? sampleRate : 48000.0f;
    updateFilterCoefficient();
    trigger();
}

void TransientGenerator::setParams(const TransientParams& params) {
    params_.impactLevel = std::clamp(params.impactLevel, 0.0f, 1.0f);
    params_.airLevel = std::clamp(params.airLevel, 0.0f, 1.0f);
    params_.airDecayMs = std::clamp(params.airDecayMs, 1.0f, 50.0f);
    params_.beaterHardnessHz =
        std::clamp(params.beaterHardnessHz, 200.0f, 16000.0f);
    updateFilterCoefficient();
}

void TransientGenerator::trigger() {
    noise_.reset();
    lowPassState_ = 0.0f;
    lowBandState_ = 0.0f;
}

float TransientGenerator::renderSample(float timeMs) {
    float impact = 0.0f;
    if (timeMs < contactDurationMs_) {
        const float phase = std::clamp(timeMs / contactDurationMs_, 0.0f, 1.0f);
        const float window = std::sin(kTwoPi * 0.5f * phase);
        // Smooth, zero-area force wavelet. Unlike the old two-sample pulse its
        // duration and spectrum do not change with sample rate.
        impact = params_.impactLevel * std::sin(kTwoPi * phase) * window * window;
    }

    float air = 0.0f;
    if (timeMs < params_.airDecayMs) {
        const float rawNoise = noise_.generate();
        lowPassState_ = (1.0f - lowPassCoefficient_) * rawNoise +
                        lowPassCoefficient_ * lowPassState_;
        lowBandState_ = (1.0f - lowBandCoefficient_) * rawNoise +
                        lowBandCoefficient_ * lowBandState_;
        const float attack =
            std::min(timeMs / std::max(contactDurationMs_, 0.001f), 1.0f);
        const float envelope =
            std::exp(-kMinus60Db * timeMs / std::max(params_.airDecayMs, 0.001f));
        air = (lowPassState_ - lowBandState_) * attack * envelope *
              params_.airLevel;
    }

    return impact + air;
}

float TransientGenerator::durationMs() const {
    return std::max(params_.airDecayMs, contactDurationMs_);
}

void TransientGenerator::updateFilterCoefficient() {
    const float cutoff = std::min(params_.beaterHardnessHz, sampleRate_ * 0.45f);
    lowPassCoefficient_ = std::exp(-kTwoPi * cutoff / sampleRate_);
    const float lowCutoff = std::min(160.0f, cutoff * 0.5f);
    lowBandCoefficient_ = std::exp(-kTwoPi * lowCutoff / sampleRate_);

    const float hardness = std::clamp(
        (std::log(params_.beaterHardnessHz) - std::log(200.0f)) /
            (std::log(16000.0f) - std::log(200.0f)),
        0.0f, 1.0f);
    constexpr float softContactMs = 2.5f;
    constexpr float hardContactMs = 0.15f;
    contactDurationMs_ = std::exp(
        std::log(softContactMs) +
        hardness * (std::log(hardContactMs) - std::log(softContactMs)));
}

} // namespace KickDrum
