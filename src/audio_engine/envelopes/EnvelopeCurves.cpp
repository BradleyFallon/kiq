#include "EnvelopeCurves.h"
#include <algorithm>
#include <cmath>

namespace KickDrum {

float applyCurve(float t, CurveType curveType) {
    // Clamp input to [0.0, 1.0] range
    t = std::clamp(t, 0.0f, 1.0f);
    
    switch (curveType) {
        case CurveType::LINEAR:
            return applyLinearCurve(t);
        
        case CurveType::EXPONENTIAL:
            return applyExponentialCurve(t);
        
        case CurveType::LOGARITHMIC:
            return applyLogarithmicCurve(t);
        
        case CurveType::CUSTOM:
            return applyCustomCurve(t);
        
        default:
            // Default to linear if unknown curve type
            return applyLinearCurve(t);
    }
}

float applyLinearCurve(float t) {
    // Linear curve is just the identity function
    // Provides constant rate of change
    return t;
}

float applyExponentialCurve(float t) {
    // Exponential curve: t^2
    // Creates an accelerating curve (starts slow, ends fast)
    // This is useful for smooth attacks that build gradually
    return t * t;
}

float applyLogarithmicCurve(float t) {
    // Logarithmic curve: sqrt(t)
    // Creates a decelerating curve (starts fast, ends slow)
    // This is useful for natural-sounding decays and releases
    return std::sqrt(t);
}

float applyCustomCurve(float t) {
    // Custom curve: t^3 (cubic)
    // This provides an even more pronounced exponential curve
    // In a full implementation, this could be:
    // - User-configurable with a curve parameter
    // - Based on a lookup table for arbitrary shapes
    // - A more complex mathematical function
    return t * t * t;
}

} // namespace KickDrum
