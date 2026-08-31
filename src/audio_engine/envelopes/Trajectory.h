#pragma once

#include "../parameters/KickParams.h"

#include <array>

namespace KickDrum {

class Trajectory {
public:
    enum class Scale {
        Linear,
        Logarithmic,
    };

    explicit Trajectory(Scale scale = Scale::Linear);

    void setPoints(const std::array<CurvePoint, kTrajectoryPointCount>& points);
    float valueAt(float timeMs) const;
    float durationMs() const;

    static float shape(float normalizedTime, float curve);

private:
    std::array<CurvePoint, kTrajectoryPointCount> points_;
    Scale scale_;
};

using PitchTrajectory = Trajectory;
using AmplitudeTrajectory = Trajectory;

} // namespace KickDrum
