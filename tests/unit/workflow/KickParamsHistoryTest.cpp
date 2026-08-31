#include <gtest/gtest.h>

#include "audio_engine/workflow/KickParamsHistory.h"

using namespace KickDrum;

TEST(KickParamsHistoryTest, UndoesAndRedoesWholeSnapshots) {
    KickParamsHistory history;
    KickParams first = history.current();
    first.pitch[0].value = 330.0f;
    ASSERT_TRUE(history.record(first));

    KickParams second = first;
    second.transient.impactLevel = 0.75f;
    second.outputStage.saturation = 0.65f;
    ASSERT_TRUE(history.record(second));
    EXPECT_EQ(history.undoCount(), 2u);

    KickParams restored {};
    ASSERT_TRUE(history.undo(restored));
    EXPECT_FLOAT_EQ(restored.pitch[0].value, 330.0f);
    EXPECT_FLOAT_EQ(restored.transient.impactLevel,
                    kDefaultKickParams.transient.impactLevel);
    ASSERT_TRUE(history.redo(restored));
    EXPECT_FLOAT_EQ(restored.transient.impactLevel, 0.75f);
    EXPECT_FLOAT_EQ(restored.outputStage.saturation, 0.65f);
}

TEST(KickParamsHistoryTest, NewEditAfterUndoDiscardsRedoBranch) {
    KickParamsHistory history;
    KickParams state = history.current();
    state.outputGain = 0.6f;
    ASSERT_TRUE(history.record(state));
    state.outputGain = 0.4f;
    ASSERT_TRUE(history.record(state));

    KickParams restored {};
    ASSERT_TRUE(history.undo(restored));
    EXPECT_TRUE(history.canRedo());
    restored.outputGain = 0.7f;
    ASSERT_TRUE(history.record(restored));
    EXPECT_FALSE(history.canRedo());
    EXPECT_FLOAT_EQ(history.current().outputGain, 0.7f);
}

TEST(KickParamsHistoryTest, RetainsOnlyConfiguredNumberOfUndoSteps) {
    KickParamsHistory history(2);
    KickParams state = history.current();
    for (const float pitch : {300.0f, 400.0f, 500.0f}) {
        state.pitch[0].value = pitch;
        ASSERT_TRUE(history.record(state));
    }

    EXPECT_EQ(history.undoCount(), 2u);
    KickParams restored {};
    ASSERT_TRUE(history.undo(restored));
    EXPECT_FLOAT_EQ(restored.pitch[0].value, 400.0f);
    ASSERT_TRUE(history.undo(restored));
    EXPECT_FLOAT_EQ(restored.pitch[0].value, 300.0f);
    EXPECT_FALSE(history.undo(restored));
}

TEST(KickParamsHistoryTest, SanitizesAndIgnoresDuplicateSnapshots) {
    KickParamsHistory history;
    KickParams state = history.current();
    state.outputGain = 4.0f;
    ASSERT_TRUE(history.record(state));
    EXPECT_FLOAT_EQ(history.current().outputGain, 1.0f);

    state.outputGain = 1.0f;
    EXPECT_FALSE(history.record(state));
    EXPECT_EQ(history.undoCount(), 1u);
}

TEST(KickParamsHistoryTest, ResetClearsBothDirections) {
    KickParamsHistory history;
    KickParams state = history.current();
    state.strikePosition = 0.8f;
    history.record(state);

    history.reset(kDefaultKickParams);
    EXPECT_FALSE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
    EXPECT_FLOAT_EQ(history.current().strikePosition,
                    kDefaultKickParams.strikePosition);
}
