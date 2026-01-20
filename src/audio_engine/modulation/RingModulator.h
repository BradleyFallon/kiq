#pragma once

namespace KickDrum {

/**
 * @brief Ring modulator for creating complex harmonic content
 * 
 * The RingModulator multiplies a carrier signal with a modulator signal
 * to create ring modulation effects. It provides a depth control that
 * blends between the dry (unmodulated) carrier signal and the fully
 * ring-modulated signal.
 * 
 * Ring modulation formula:
 *   output = carrier × (1.0 - depth) + (carrier × modulator) × depth
 * 
 * Where:
 *   - depth = 0.0: fully dry (no modulation)
 *   - depth = 1.0: fully wet (full ring modulation)
 *   - depth in (0.0, 1.0): linear blend between dry and wet
 * 
 * This is used to modulate the Sine Driver with both the Harmonic Membrane
 * and the Noise Generator to create rich harmonic content and texture.
 */
class RingModulator {
public:
    /**
     * @brief Construct a new Ring Modulator with default depth (0.0)
     */
    RingModulator();

    /**
     * @brief Set the modulation depth
     * @param depth Modulation depth in range [0.0, 1.0]
     *              0.0 = fully dry (no modulation)
     *              1.0 = fully wet (full ring modulation)
     */
    void setDepth(float depth);

    /**
     * @brief Get the current modulation depth
     * @return Current depth value in range [0.0, 1.0]
     */
    float getDepth() const;

    /**
     * @brief Process a sample through the ring modulator
     * @param carrier The carrier signal (typically from Sine Driver)
     * @param modulator The modulator signal (from Harmonic Membrane or Noise Generator)
     * @return Processed sample with ring modulation applied according to depth
     */
    float process(float carrier, float modulator);

private:
    float depth_;  ///< Modulation depth (0.0 to 1.0)
};

} // namespace KickDrum
