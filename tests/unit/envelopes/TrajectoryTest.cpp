#include <gtest/gtest.h>

#include "audio_engine/envelopes/Trajectory.h"

#include <cmath>

using namespace KickDrum;

TEST(TrajectoryTest, ReturnsExactPitchNodeValues) {
    PitchTrajectory trajectory(Trajectory::Scale::Logarithmic);
    trajectory.setPoints(kDefaultKickParams.pitch);

    for (const auto& point : kDefaultKickParams.pitch) {
        EXPECT_FLOAT_EQ(trajectory.valueAt(point.timeMs), point.value);
    }
}

TEST(TrajectoryTest, LogPitchInterpolatesFrequencyRatios) {
    PitchTrajectory trajectory(Trajectory::Scale::Logarithmic);
    trajectory.setPoints({{{0.0f, 200.0f, 0.0f},
                           {10.0f, 50.0f, 0.0f},
                           {20.0f, 50.0f, 0.0f},
                           {30.0f, 50.0f, 0.0f}}});

    EXPECT_NEAR(trajectory.valueAt(5.0f), 100.0f, 0.001f);
}

TEST(TrajectoryTest, DefaultPitchIsPositiveAndMonotonicallyDescending) {
    PitchTrajectory trajectory(Trajectory::Scale::Logarithmic);
    trajectory.setPoints(kDefaultKickParams.pitch);

    float previous = trajectory.valueAt(0.0f);
    for (float timeMs = 0.1f; timeMs <= trajectory.durationMs(); timeMs += 0.1f) {
        const float current = trajectory.valueAt(timeMs);
        EXPECT_GT(current, 0.0f);
        EXPECT_LE(current, previous + 0.0001f);
        previous = current;
    }
}

TEST(TrajectoryTest, ReturnsExactAmplitudeNodeValues) {
    AmplitudeTrajectory trajectory(Trajectory::Scale::Linear);
    trajectory.setPoints(kDefaultKickParams.amplitude);

    for (const auto& point : kDefaultKickParams.amplitude) {
        EXPECT_FLOAT_EQ(trajectory.valueAt(point.timeMs), point.value);
    }
}
