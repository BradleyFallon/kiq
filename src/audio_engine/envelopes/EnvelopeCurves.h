#pragma once

#include <cmath>

namespace KickDrum {

/**
 * @brief Envelope curve types for shaping envelope segments
 * 
 * These curve types determine how an envelope transitions from one value
 * to another over time. Each curve type provides a different character:
 * - LINEAR: Constant rate of change (straight line)
 * - EXPONENTIAL: Accelerating change (starts slow, ends fast)
 * - LOGARITHMIC: Decelerating change (starts fast, ends slow)
 * - CUSTOM: User-defined curve shape
 */
enum class CurveType {
    LINEAR,
    EXPONENTIAL,
    LOGARITHMIC,
    CUSTOM
};

/**
 * @brief Apply a curve shape to normalized time
 * 
 * Takes a normalized time value (0.0 to 1.0) and applies the specified
 * curve shape to it. The output is also in the range [0.0, 1.0].
 * 
 * This function is used to shape envelope segments, allowing for different
 * attack, decay, and release characteristics.
 * 
 * @param t Normalized time (0.0 to 1.0)
 * @param curveType The type of curve to apply
 * @return Curved value (0.0 to 1.0)
 * 
 * @note Input values outside [0.0, 1.0] are clamped to this range
 * 
 * Examples:
 * - applyCurve(0.5, CurveType::LINEAR) returns 0.5
 * - applyCurve(0.5, CurveType::EXPONENTIAL) returns 0.25 (slower at start)
 * - applyCurve(0.5, CurveType::LOGARITHMIC) returns ~0.707 (faster at start)
 */
float applyCurve(float t, CurveType curveType);

/**
 * @brief Apply a linear curve (identity function)
 * 
 * Returns the input value unchanged. This provides a constant rate of change.
 * 
 * @param t Normalized time (0.0 to 1.0)
 * @return Same as input t
 */
float applyLinearCurve(float t);

/**
 * @brief Apply an exponential curve
 * 
 * Returns t^2, which creates an accelerating curve. The envelope starts
 * slowly and speeds up toward the end. This is useful for smooth attacks
 * that build gradually.
 * 
 * @param t Normalized time (0.0 to 1.0)
 * @return t^2 (0.0 to 1.0)
 */
float applyExponentialCurve(float t);

/**
 * @brief Apply a logarithmic curve
 * 
 * Returns sqrt(t), which creates a decelerating curve. The envelope starts
 * quickly and slows down toward the end. This is useful for natural-sounding
 * decays and releases.
 * 
 * @param t Normalized time (0.0 to 1.0)
 * @return sqrt(t) (0.0 to 1.0)
 */
float applyLogarithmicCurve(float t);

/**
 * @brief Apply a custom curve
 * 
 * Currently implements a cubic curve (t^3) for demonstration purposes.
 * In a full implementation, this could be user-configurable or use
 * a lookup table for arbitrary curve shapes.
 * 
 * @param t Normalized time (0.0 to 1.0)
 * @return Custom curved value (0.0 to 1.0)
 */
float applyCustomCurve(float t);

} // namespace KickDrum
