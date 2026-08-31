#pragma once

#include "SineDriver.h"

#include <array>

namespace KickDrum {

/** Lightweight modal model of a struck circular membrane. */
class MembraneModel {
public:
    void initialize(float sampleRate);
    void setStrikePosition(float normalizedPosition);
    /** Set the fundamental's phase at the instant of the next trigger. */
    void setFundamentalPhase(float normalizedCycles);
    void trigger();

    float renderSample(float fundamentalHz, float timeMs);

    float getStrikePosition() const { return strikePosition_; }
    float getFundamentalPhase() const;

private:
    static constexpr std::array<float, 3> kModeRatios {
        1.0f, 1.59334f, 2.13555f,
    };

    std::array<SineDriver, 3> modes_;
    std::array<float, 3> modeGains_ {1.0f, 0.0f, 0.0f};
    float strikePosition_ = 0.22f;
    float fundamentalStartPhase_ = 0.0f;
    bool initialized_ = false;
};

} // namespace KickDrum
