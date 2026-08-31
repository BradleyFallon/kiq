#include "ParameterEventQueue.h"
#include <algorithm>
#include <utility>

namespace KickDrum {

ParameterEventQueue::ParameterEventQueue() {
    // Reserve space for typical event count to avoid reallocations
    events_.reserve(32);
}

void ParameterEventQueue::addEvent(const ParameterEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    ParameterEvent sequenced = event;
    sequenced.order = nextOrder_++;
    events_.push_back(std::move(sequenced));
}

void ParameterEventQueue::addEvent(const std::string& parameterId, float value, uint32_t sampleOffset) {
    addEvent(ParameterEvent(parameterId, value, sampleOffset));
}

void ParameterEventQueue::addEvents(const std::vector<ParameterEvent>& events) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.reserve(events_.size() + events.size());
    for (const auto& event : events) {
        ParameterEvent sequenced = event;
        sequenced.order = nextOrder_++;
        events_.push_back(std::move(sequenced));
    }
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
    // Explicit sequence numbers preserve producer order at equal offsets
    // without std::stable_sort's potential temporary allocation on audio.
    std::sort(outEvents.begin(), outEvents.end());
}

void ParameterEventQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
    nextOrder_ = 0;
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
