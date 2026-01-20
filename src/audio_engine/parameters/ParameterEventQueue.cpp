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
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear output vector
    outEvents.clear();
    
    // Move events to output vector
    outEvents = std::move(events_);
    
    // Clear the internal queue
    events_.clear();
    
    // Sort events by sample offset to ensure they're processed in order
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
