#pragma once

#include <vector>
#include <cmath>

namespace KickDrum {

/**
 * @brief Freeverb-style reverb effect processor
 * 
 * The Reverb implements a Freeverb-style algorithm using parallel comb filters
 * followed by series allpass filters to create a realistic room ambience effect.
 * 
 * Algorithm structure:
 * 1. Input signal is fed to 8 parallel comb filters with feedback
 * 2. Comb filter outputs are summed
 * 3. Summed signal passes through 4 series allpass filters
 * 4. Damping is applied in the comb filter feedback paths
 * 5. Final output is mixed with dry signal based on mix parameter
 * 
 * Parameters:
 * - Room Size: Controls the size of the simulated space (0.0 to 1.0)
 * - Decay Time: Controls how long the reverb tail lasts (in seconds)
 * - Damping: Controls high-frequency absorption (0.0 to 1.0)
 * - Mix: Dry/wet blend (0.0 = fully dry, 1.0 = fully wet)
 */
class Reverb {
public:
    /**
     * @brief Construct a new Reverb with default parameters
     */
    Reverb();

    /**
     * @brief Destroy the Reverb and free resources
     */
    ~Reverb();

    /**
     * @brief Initialize the reverb with a sample rate
     * @param sampleRate Sample rate in Hz (e.g., 44100, 48000)
     */
    void initialize(float sampleRate);

    /**
     * @brief Set the room size parameter
     * @param roomSize Room size in range [0.0, 1.0]
     *                 0.0 = small room, 1.0 = large hall
     */
    void setRoomSize(float roomSize);

    /**
     * @brief Get the current room size
     * @return Room size in range [0.0, 1.0]
     */
    float getRoomSize() const;

    /**
     * @brief Set the decay time
     * @param decaySeconds Decay time in seconds (typically 0.1 to 10.0)
     *                     Time for reverb to decay by 60dB
     */
    void setDecayTime(float decaySeconds);

    /**
     * @brief Get the current decay time
     * @return Decay time in seconds
     */
    float getDecayTime() const;

    /**
     * @brief Set the damping amount
     * @param damping Damping in range [0.0, 1.0]
     *                0.0 = no damping (bright reverb)
     *                1.0 = maximum damping (dark reverb)
     */
    void setDamping(float damping);

    /**
     * @brief Get the current damping amount
     * @return Damping in range [0.0, 1.0]
     */
    float getDamping() const;

    /**
     * @brief Set the dry/wet mix
     * @param mix Mix amount in range [0.0, 1.0]
     *            0.0 = fully dry (no reverb)
     *            1.0 = fully wet (full reverb)
     */
    void setMix(float mix);

    /**
     * @brief Get the current mix amount
     * @return Mix value in range [0.0, 1.0]
     */
    float getMix() const;

    /**
     * @brief Process a single audio sample through the reverb
     * @param input Input sample
     * @return Reverberated output sample
     */
    float process(float input);

    /**
     * @brief Reset the reverb state
     * 
     * Clears all delay line buffers, useful when starting a new note
     * or clearing the reverb tail.
     */
    void reset();

    /**
     * @brief Check if the reverb is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

private:
    // Comb filter for parallel processing
    class CombFilter {
    public:
        CombFilter();
        void initialize(int bufferSize, float sampleRate);
        void setFeedback(float feedback);
        void setDamping(float damping);
        float process(float input);
        void reset();

    private:
        std::vector<float> buffer_;
        int bufferSize_;
        int bufferIndex_;
        float feedback_;
        float damping_;
        float filterState_;  // For damping filter
    };

    // Allpass filter for series processing
    class AllpassFilter {
    public:
        AllpassFilter();
        void initialize(int bufferSize);
        float process(float input);
        void reset();

    private:
        std::vector<float> buffer_;
        int bufferSize_;
        int bufferIndex_;
        static constexpr float FEEDBACK = 0.5f;
    };

    // Freeverb uses 8 comb filters with prime-number delay lengths
    static constexpr int NUM_COMBS = 8;
    static constexpr int COMB_TUNINGS[NUM_COMBS] = {
        1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
    };

    // Freeverb uses 4 allpass filters
    static constexpr int NUM_ALLPASSES = 4;
    static constexpr int ALLPASS_TUNINGS[NUM_ALLPASSES] = {
        556, 441, 341, 225
    };

    // Scaling factors
    static constexpr float FIXED_GAIN = 0.015f;
    static constexpr float SCALE_WET = 3.0f;
    static constexpr float SCALE_DAMPING = 0.4f;
    static constexpr float SCALE_ROOM = 0.28f;
    static constexpr float OFFSET_ROOM = 0.7f;

    float sampleRate_;
    float roomSize_;
    float decayTime_;
    float damping_;
    float mix_;
    bool initialized_;

    // Filter banks
    CombFilter combFilters_[NUM_COMBS];
    AllpassFilter allpassFilters_[NUM_ALLPASSES];

    /**
     * @brief Update comb filter parameters based on room size and decay time
     */
    void updateCombFilters();

    /**
     * @brief Scale delay time based on sample rate
     * @param tuning Base tuning value at 44.1kHz
     * @return Scaled delay time in samples
     */
    int scaleDelayTime(int tuning) const;
};

} // namespace KickDrum
