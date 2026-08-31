#pragma once

#include <string>
#include <cstdint>

namespace KickDrum {

/**
 * @brief Represents a parameter change event with sample-accurate timing
 * 
 * ParameterEvent stores a parameter change that should occur at a specific
 * sample position within an audio buffer. This enables sample-accurate
 * parameter automation and control.
 * 
 * Requirements validated:
 * - 2.10: Update synthesis within 10 milliseconds of parameter change
 */
struct ParameterEvent {
    /**
     * @brief Parameter ID (e.g., "pitch0Hz", "outputGain")
     */
    std::string parameterId;
    
    /**
     * @brief New parameter value
     */
    float value;
    
    /**
     * @brief Sample offset within the buffer where this change should occur
     * 
     * For example, if sampleOffset is 256 in a 512-sample buffer,
     * the parameter change will be applied starting at sample 256.
     */
    uint32_t sampleOffset;

    /** Producer order assigned by ParameterEventQueue. */
    std::uint64_t order;
    
    /**
     * @brief Construct a parameter event
     * 
     * @param id Parameter ID
     * @param val New value
     * @param offset Sample offset within buffer (default: 0)
     */
    ParameterEvent(const std::string& id, float val, uint32_t offset = 0)
        : parameterId(id)
        , value(val)
        , sampleOffset(offset)
        , order(0)
    {
    }
    
    /**
     * @brief Default constructor
     */
    ParameterEvent()
        : parameterId("")
        , value(0.0f)
        , sampleOffset(0)
        , order(0)
    {
    }
    
    /**
     * @brief Compare events by sample offset (for sorting)
     */
    bool operator<(const ParameterEvent& other) const {
        return sampleOffset < other.sampleOffset ||
               (sampleOffset == other.sampleOffset && order < other.order);
    }
};

} // namespace KickDrum
