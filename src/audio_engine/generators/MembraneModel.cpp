#include "MembraneModel.h"

#include "../parameters/KickParams.h"

#include <algorithm>
#include <cmath>

namespace KickDrum {
namespace {

// First roots for the (0,1), (1,1), and (2,1) modes of an ideal circular
// membrane. A compact Bessel series is sufficient here because it runs only
// when strike position changes, never per audio sample.
constexpr std::array<float, 3> kModeRoots {
    2.4048256f, 3.8317060f, 5.1356225f,
};

float besselJ(unsigned order, float x) {
    const double halfX = static_cast<double>(x) * 0.5;
    double factorial = 1.0;
    for (unsigned value = 2; value <= order; ++value) {
        factorial *= static_cast<double>(value);
    }

    double term = std::pow(halfX, static_cast<int>(order)) / factorial;
    double sum = term;
    for (unsigned index = 1; index <= 14; ++index) {
        term *= -(halfX * halfX) /
                (static_cast<double>(index) * static_cast<double>(index + order));
        sum += term;
    }
    return static_cast<float>(sum);
}

} // namespace

void MembraneModel::initialize(float sampleRate) {
    const float validSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0f
                                      ? sampleRate
                                      : 48000.0f;
    for (auto& mode : modes_) {
        mode.initialize(validSampleRate);
    }
    initialized_ = true;
    setStrikePosition(strikePosition_);
    trigger();
}

void MembraneModel::setStrikePosition(float normalizedPosition) {
    if (!std::isfinite(normalizedPosition)) {
        normalizedPosition = kDefaultKickParams.strikePosition;
    }
    strikePosition_ = std::clamp(normalizedPosition, 0.0f, 1.0f);
    // Keep the UI's far-right stop slightly inside the fixed rim so it remains
    // useful instead of landing on the membrane's all-mode displacement node.
    const float radius = strikePosition_ * 0.85f;
    modeGains_[0] = besselJ(0, kModeRoots[0] * radius);
    modeGains_[1] = 0.35f * besselJ(1, kModeRoots[1] * radius);
    modeGains_[2] = 0.22f * besselJ(2, kModeRoots[2] * radius);
}

void MembraneModel::setFundamentalPhase(float normalizedCycles) {
    if (!std::isfinite(normalizedCycles)) {
        normalizedCycles = 0.0f;
    }
    fundamentalStartPhase_ = normalizedCycles - std::floor(normalizedCycles);
    if (fundamentalStartPhase_ < 0.0f) {
        fundamentalStartPhase_ += 1.0f;
    }
}

void MembraneModel::trigger() {
    for (std::size_t index = 0; index < modes_.size(); ++index) {
        modes_[index].reset();
        // A force impulse establishes membrane velocity, so each modal impulse
        // response normally begins at its zero crossing. The fundamental can
        // be rotated or locked independently without disturbing the physical
        // relative phase of the shorter upper modes.
        modes_[index].setPhase(index == 0 ? fundamentalStartPhase_ : 0.0f);
    }
}

float MembraneModel::renderSample(float fundamentalHz, float timeMs) {
    if (!initialized_) {
        return 0.0f;
    }

    const float baseFrequency = std::isfinite(fundamentalHz)
                                    ? std::max(fundamentalHz, 0.0f)
                                    : 0.0f;
    const float safeTimeMs = std::isfinite(timeMs) ? std::max(timeMs, 0.0f) : 0.0f;
    for (std::size_t index = 0; index < modes_.size(); ++index) {
        modes_[index].setFrequency(baseFrequency * kModeRatios[index]);
    }

    const float secondModeGain = modeGains_[1] * std::exp(-safeTimeMs / 34.0f);
    const float thirdModeGain = modeGains_[2] * std::exp(-safeTimeMs / 16.0f);

    return modeGains_[0] * modes_[0].generate() +
           secondModeGain * modes_[1].generate() +
           thirdModeGain * modes_[2].generate();
}

float MembraneModel::getFundamentalPhase() const {
    return modes_[0].getPhase();
}

} // namespace KickDrum
