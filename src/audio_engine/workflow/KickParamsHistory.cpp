#include "KickParamsHistory.h"

namespace KickDrum {

KickParamsHistory::KickParamsHistory(std::size_t undoLimit,
                                     const KickParams& initial)
    : undoLimit_(undoLimit) {
    reset(initial);
}

void KickParamsHistory::reset(const KickParams& initial) {
    states_.clear();
    states_.push_back(sanitizeKickParams(initial));
    cursor_ = 0;
}

bool KickParamsHistory::record(const KickParams& params) {
    const KickParams snapshot = sanitizeKickParams(params);
    if (equal(states_[cursor_], snapshot)) {
        return false;
    }

    states_.erase(states_.begin() + static_cast<std::ptrdiff_t>(cursor_ + 1),
                  states_.end());
    states_.push_back(snapshot);

    const std::size_t maximumStates = undoLimit_ + 1;
    if (states_.size() > maximumStates) {
        const std::size_t excess = states_.size() - maximumStates;
        states_.erase(states_.begin(),
                      states_.begin() + static_cast<std::ptrdiff_t>(excess));
    }
    cursor_ = states_.size() - 1;
    return true;
}

bool KickParamsHistory::canUndo() const {
    return cursor_ > 0;
}

bool KickParamsHistory::canRedo() const {
    return cursor_ + 1 < states_.size();
}

bool KickParamsHistory::undo(KickParams& restored) {
    if (!canUndo()) {
        return false;
    }
    restored = states_[--cursor_];
    return true;
}

bool KickParamsHistory::redo(KickParams& restored) {
    if (!canRedo()) {
        return false;
    }
    restored = states_[++cursor_];
    return true;
}

const KickParams& KickParamsHistory::current() const {
    return states_[cursor_];
}

std::size_t KickParamsHistory::undoCount() const {
    return cursor_;
}

std::size_t KickParamsHistory::redoCount() const {
    return states_.size() - cursor_ - 1;
}

bool KickParamsHistory::equal(const KickParams& left, const KickParams& right) {
    for (const auto& spec : kKickParameterSpecs) {
        if (getKickParameter(left, spec.id) !=
            getKickParameter(right, spec.id)) {
            return false;
        }
    }
    return true;
}

} // namespace KickDrum
