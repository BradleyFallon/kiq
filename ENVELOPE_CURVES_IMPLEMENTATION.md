# Envelope Curves Implementation

## Overview

This document describes the implementation of envelope curve functions for the Kick Drum Synthesizer. These curves are used to shape envelope segments (attack, decay, release) to create different sonic characteristics.

## Implementation Summary

**Files Created/Modified:**
- `src/audio_engine/envelopes/EnvelopeCurves.h` - Header file with curve type enum and function declarations
- `src/audio_engine/envelopes/EnvelopeCurves.cpp` - Implementation of curve functions
- `tests/unit/envelopes/EnvelopeCurvesTest.cpp` - Comprehensive unit tests
- `test_envelope_curves_compile.sh` - Standalone compilation and test script

## Curve Types

### 1. Linear Curve
**Function:** `f(t) = t`

The linear curve is the identity function, providing a constant rate of change. This is the simplest curve and creates a straight-line transition.

**Use Cases:**
- Simple, predictable envelopes
- When no curve shaping is desired

**Characteristics:**
- Constant slope
- No acceleration or deceleration

### 2. Exponential Curve
**Function:** `f(t) = t²`

The exponential curve creates an accelerating transition. It starts slowly and speeds up toward the end.

**Use Cases:**
- Smooth, gradual attacks that build momentum
- Creating anticipation in the sound
- Natural-feeling crescendos

**Characteristics:**
- Starts slow (low slope at t=0)
- Accelerates (increasing slope)
- Ends fast (high slope at t=1)
- Always below the linear curve for t ∈ (0, 1)

### 3. Logarithmic Curve
**Function:** `f(t) = √t`

The logarithmic curve creates a decelerating transition. It starts quickly and slows down toward the end.

**Use Cases:**
- Natural-sounding decays and releases
- Mimicking acoustic instrument behavior
- Smooth fade-outs

**Characteristics:**
- Starts fast (high slope at t=0)
- Decelerates (decreasing slope)
- Ends slow (low slope at t=1)
- Always above the linear curve for t ∈ (0, 1)

### 4. Custom Curve
**Function:** `f(t) = t³`

The custom curve provides an even more pronounced exponential characteristic. In the current implementation, it uses a cubic function, but this could be extended to support user-defined curves.

**Use Cases:**
- Extreme envelope shaping
- Special effects
- Future extension point for user-defined curves

**Characteristics:**
- Very slow start
- Very fast end
- More pronounced than exponential curve
- Below exponential curve for t ∈ (0, 1)

## Curve Ordering

At t = 0.5, the curves are ordered from slowest to fastest:

```
Custom (0.125) < Exponential (0.25) < Linear (0.5) < Logarithmic (0.707)
```

This ordering shows how each curve affects the envelope shape:
- **Custom/Exponential**: Delay the envelope change (good for attacks)
- **Linear**: Neutral, constant rate
- **Logarithmic**: Accelerate the envelope change (good for decays)

## Mathematical Properties

All curve functions satisfy these properties:

1. **Boundary Conditions:**
   - `f(0) = 0` (start at zero)
   - `f(1) = 1` (end at one)

2. **Range:**
   - `f(t) ∈ [0, 1]` for all `t ∈ [0, 1]`

3. **Monotonicity:**
   - `f(t₁) ≤ f(t₂)` for all `t₁ ≤ t₂` (non-decreasing)

4. **Continuity:**
   - All curves are continuous and smooth

## Input Clamping

The `applyCurve()` function automatically clamps input values to the valid range [0.0, 1.0]:
- Values below 0 are clamped to 0
- Values above 1 are clamped to 1

This ensures robust behavior even with invalid inputs.

## API Usage

### Basic Usage

```cpp
#include "EnvelopeCurves.h"

using namespace KickDrum;

// Apply a curve to normalized time
float t = 0.5f;  // Halfway through envelope segment

float linear = applyCurve(t, CurveType::LINEAR);        // 0.5
float exponential = applyCurve(t, CurveType::EXPONENTIAL);  // 0.25
float logarithmic = applyCurve(t, CurveType::LOGARITHMIC);  // 0.707
float custom = applyCurve(t, CurveType::CUSTOM);        // 0.125
```

### Direct Function Calls

```cpp
// You can also call the curve functions directly
float linear = applyLinearCurve(0.5f);        // 0.5
float exponential = applyExponentialCurve(0.5f);  // 0.25
float logarithmic = applyLogarithmicCurve(0.5f);  // 0.707
float custom = applyCustomCurve(0.5f);        // 0.125
```

### Envelope Integration Example

```cpp
// Example: Apply curve to envelope segment
float segmentDuration = 0.5f;  // 500ms
float elapsedTime = 0.25f;     // 250ms elapsed
float normalizedTime = elapsedTime / segmentDuration;  // 0.5

// Apply exponential curve for smooth attack
float curvedTime = applyCurve(normalizedTime, CurveType::EXPONENTIAL);

// Use curvedTime to interpolate envelope value
float startLevel = 0.0f;
float endLevel = 1.0f;
float envelopeValue = startLevel + (endLevel - startLevel) * curvedTime;
```

## Testing

### Unit Tests

The implementation includes comprehensive unit tests covering:

1. **Individual Curve Tests:**
   - Boundary values (0 and 1)
   - Midpoint values
   - Monotonicity
   - Range validation

2. **Dispatcher Tests:**
   - Correct routing to curve functions
   - All curve types handled

3. **Input Validation:**
   - Clamping below 0
   - Clamping above 1

4. **Curve Relationships:**
   - Ordering verification
   - Relative positioning

5. **Practical Use Cases:**
   - Attack characteristics
   - Decay characteristics

### Test Results

All tests pass successfully:
```
✓ Linear curve works correctly
✓ Exponential curve works correctly
✓ Logarithmic curve works correctly
✓ Custom curve works correctly
✓ applyCurve() dispatcher works correctly
✓ Input clamping works correctly
✓ Curve ordering is correct
✓ Boundary behavior is correct
✓ All curves stay within valid range [0.0, 1.0]
✓ All curves are monotonically increasing
```

## Requirements Validation

This implementation satisfies the following requirements:

- **Requirement 2.6:** "WHEN configuring any envelope, THE Kick_Synth SHALL allow selection of curve shape (linear, exponential, logarithmic, custom)"
  - ✅ All four curve types implemented

- **Requirement 2.8:** "THE Kick_Synth SHALL allow independent curve shaping for each envelope segment"
  - ✅ Curve functions are independent and can be applied to any segment

- **Requirement 2.9:** "THE Kick_Synth SHALL allow independent curve shaping for each envelope segment"
  - ✅ Same as 2.8 (appears to be duplicate in requirements)

## Visual Representation

Here's how the curves compare over the range [0, 1]:

```
1.0 |                                                    ●
    |                                              ●●●●●●
    |                                        ●●●●●●
    |                                  ●●●●●●
    |                            ●●●●●●
0.7 |                      ●●●●●●  (Logarithmic)
    |                ●●●●●●
    |          ●●●●●●
    |    ●●●●●●
0.5 |  ●●  (Linear)
    | ●
    |●
    |●
0.25|●  (Exponential)
    |●
    |●
    |●
0.0 |●  (Custom)
    +----------------------------------------------------
    0.0                    0.5                        1.0
```

## Performance Considerations

- All curve functions are simple mathematical operations (multiplication, sqrt)
- No branching within individual curve functions
- Minimal computational overhead
- Suitable for real-time audio processing

## Future Enhancements

Possible extensions to the curve system:

1. **Parameterized Curves:**
   - Add curve "tension" parameter to adjust steepness
   - Allow user to control the exponent (t^n where n is adjustable)

2. **Bezier Curves:**
   - Support arbitrary curve shapes using Bezier control points
   - Visual curve editor in UI

3. **Lookup Tables:**
   - Pre-compute curves for faster evaluation
   - Support for arbitrary curve shapes

4. **Additional Curve Types:**
   - S-curves (sigmoid functions)
   - Inverse exponential
   - Stepped curves for special effects

## Integration with Envelope System

These curve functions will be used by:

1. **DualPhaseEnvelope:**
   - Apply curves to attack, decay, and release segments
   - Independent curve selection for each segment

2. **PitchEnvelope:**
   - Shape pitch modulation over time
   - Create natural or exaggerated pitch sweeps

3. **Amplitude Envelope:**
   - Control volume changes with different characteristics
   - Create punchy or smooth amplitude contours

## Conclusion

The envelope curve implementation provides a solid foundation for flexible envelope shaping in the Kick Drum Synthesizer. The four curve types (linear, exponential, logarithmic, custom) cover the most common use cases and can be easily extended in the future.

All tests pass, and the implementation is ready for integration with the DualPhaseEnvelope system.
