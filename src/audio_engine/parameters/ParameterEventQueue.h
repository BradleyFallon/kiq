#pragma once

#include "ParameterEvent.h"
#include <cstdint>
#include <mutex>
#include <vector>

namespace KickDrum {

/**
 * @brief Thread-safe queue for sample-accurate parameter events
 * 
 * The ParameterEventQueue stores parameter change events that should be
 * processed at specific sample positions within audio buffers. This enables
 * sample-accurate parameter automation with minimal latency.
 * 
 * Thread Safety:
 * - addEvent() can be called from the UI thread
 * - getEventsForBuffer() is called from the audio thread
 * - The audio consumer uses a non-blocking lock attempt; a contended batch is
 *   deferred by one buffer rather than stalling realtime rendering
 * 
 * Requirements validated:
 * - 2.10: Update synthesis within 10 milliseconds of parameter change
 */
class ParameterEventQueue {
public:
    /**
     * @brief Construct a new Parameter Event Queue
     */
    ParameterEventQueue();
    
    /**
     * @brief Add a parameter event to the queue
     * 
     * Thread-safe: Can be called from any thread (typically UI thread).
     * 
     * @param event Parameter event to add
     */
    void addEvent(const ParameterEvent& event);
    
    /**
     * @brief Add a parameter event with explicit parameters
     * 
     * Thread-safe: Can be called from any thread (typically UI thread).
     * 
     * @param parameterId Parameter ID
     * @param value New parameter value
     * @param sampleOffset Sample offset within next buffer (default: 0)
     */
    void addEvent(const std::string& parameterId, float value, uint32_t sampleOffset = 0);

    /**
     * Add a complete producer-side batch under one lock.
     *
     * This is used for state restoration so the audio thread observes either
     * the old state or the whole replacement, never a partially enqueued
     * trajectory.
     */
    void addEvents(const std::vector<ParameterEvent>& events);
    
    /**
     * @brief Get all events for the current buffer and clear the queue
     * 
     * This should be called at the start of each audio buffer processing.
     * Events are sorted by sample offset before being returned.
     * 
     * Thread-safe: Should be called from audio thread only.
     * 
     * @param outEvents Output vector to receive events (will be cleared first)
     */
    void getEventsForBuffer(std::vector<ParameterEvent>& outEvents);
    
    /**
     * @brief Clear all pending events
     * 
     * Thread-safe: Can be called from any thread.
     */
    void clear();
    
    /**
     * @brief Get the number of pending events
     * 
     * Thread-safe: Can be called from any thread.
     * 
     * @return Number of events in queue
     */
    size_t getEventCount() const;
    
    /**
     * @brief Check if the queue is empty
     * 
     * Thread-safe: Can be called from any thread.
     * 
     * @return true if queue is empty
     */
    bool isEmpty() const;
    
private:
    std::vector<ParameterEvent> events_;  ///< Pending parameter events
    mutable std::mutex mutex_;            ///< Mutex for thread safety
    std::uint64_t nextOrder_ = 0;         ///< Equal-offset producer ordering
};

} // namespace KickDrum
