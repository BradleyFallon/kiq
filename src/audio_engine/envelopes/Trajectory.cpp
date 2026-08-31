#include "Trajectory.h"

#include <algorithm>
#include <cmath>

namespace KickDrum {

Trajectory::Trajectory(Scale scale)
    : points_(scale == Scale::Logarithmic ? kDefaultKickParams.pitch
                                         : kDefaultKickParams.amplitude)
    , scale_(scale) {
}

void Trajectory::setPoints(const std::array<CurvePoint, kTrajectoryPointCount>& points) {
    points_ = points;
    points_[0].timeMs = 0.0f;
    for (std::size_t index = 0; index < points_.size(); ++index) {
        points_[index].curve = std::clamp(points_[index].curve, -1.0f, 1.0f);
        if (index > 0) {
            points_[index].timeMs =
                std::max(points_[index].timeMs, points_[index - 1].timeMs + 0.01f);
        }
    }
}

float Trajectory::valueAt(float timeMs) const {
    if (timeMs <= points_.front().timeMs) {
        return points_.front().value;
    }
    if (timeMs >= points_.back().timeMs) {
        return points_.back().value;
    }

    for (std::size_t index = 0; index + 1 < points_.size(); ++index) {
        const auto& start = points_[index];
        const auto& end = points_[index + 1];
        if (timeMs > end.timeMs) {
            continue;
        }

        const float duration = end.timeMs - start.timeMs;
        const float normalized = duration > 0.0f ? (timeMs - start.timeMs) / duration : 1.0f;
        const float u = shape(normalized, start.curve);

        if (scale_ == Scale::Logarithmic) {
            const float startValue = std::max(start.value, 0.001f);
            const float endValue = std::max(end.value, 0.001f);
            return std::exp(std::log(startValue) +
                            (std::log(endValue) - std::log(startValue)) * u);
        }
        return start.value + (end.value - start.value) * u;
    }

    return points_.back().value;
}

float Trajectory::durationMs() const {
    return points_.back().timeMs;
}

float Trajectory::shape(float normalizedTime, float curve) {
    const float u = std::clamp(normalizedTime, 0.0f, 1.0f);
    const float amount = std::clamp(curve, -1.0f, 1.0f);
    if (std::abs(amount) < 1.0e-6f) {
        return u;
    }
    if (amount > 0.0f) {
        return std::pow(u, 1.0f + amount * 4.0f);
    }
    return 1.0f - std::pow(1.0f - u, 1.0f - amount * 4.0f);
}

} // namespace KickDrum
