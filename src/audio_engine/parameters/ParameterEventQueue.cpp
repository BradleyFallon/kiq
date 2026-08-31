#include "ParameterEventQueue.h"
#include <algorithm>

namespace KickDrum {

ParameterEventQueue::ParameterEventQueue() {
    // Reserve space for typical event count to avoid reallocations
    events_.reserve(32);
}

void ParameterEventQueue::addEvent(const ParameterEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
}

void ParameterEventQueue::addEvent(const std::string& parameterId, float value, uint32_t sampleOffset) {
    addEvent(ParameterEvent(parameterId, value, sampleOffset));
}

void ParameterEventQueue::getEventsForBuffer(std::vector<ParameterEvent>& outEvents) {
    outEvents.clear();

    // Never wait for the UI producer from the realtime audio callback. If the
    // producer owns the queue for this instant, the events remain queued for
    // the next block instead of stalling audio.
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        return;
    }
    outEvents.swap(events_);
    lock.unlock();

    // Sort after releasing the producer lock.
    std::sort(outEvents.begin(), outEvents.end());
}

void ParameterEventQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
}

size_t ParameterEventQueue::getEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}

bool ParameterEventQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.empty();
}

} // namespace KickDrum
