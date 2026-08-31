#pragma once

#include "../parameters/KickParams.h"

#include <cstddef>
#include <vector>

namespace KickDrum {

/**
 * Bounded snapshot history for editor-level undo and redo.
 *
 * Call record() once at the end of a user gesture. The history owns complete,
 * sanitized KickParams snapshots, so restoring a state never depends on the
 * order in which individual parameters were edited.
 */
class KickParamsHistory {
public:
    static constexpr std::size_t kDefaultUndoLimit = 30;

    explicit KickParamsHistory(
        std::size_t undoLimit = kDefaultUndoLimit,
        const KickParams& initial = kDefaultKickParams);

    void reset(const KickParams& initial = kDefaultKickParams);

    /** Record a new state. Returns false when it is identical to current(). */
    bool record(const KickParams& params);

    bool canUndo() const;
    bool canRedo() const;
    bool undo(KickParams& restored);
    bool redo(KickParams& restored);

    const KickParams& current() const;
    std::size_t undoCount() const;
    std::size_t redoCount() const;
    std::size_t undoLimit() const { return undoLimit_; }

private:
    static bool equal(const KickParams& left, const KickParams& right);

    std::vector<KickParams> states_;
    std::size_t cursor_ = 0;
    std::size_t undoLimit_ = kDefaultUndoLimit;
};

} // namespace KickDrum
