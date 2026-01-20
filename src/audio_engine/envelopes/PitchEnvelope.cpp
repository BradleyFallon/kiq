#include "PitchEnvelope.h"
#include <algorithm>

namespace KickDrum {

PitchEnvelope::PitchEnvelope(float sampleRate)
    : envelope(new DualPhaseEnvelope(sampleRate))
    , depth(500.0f)  // Default depth of 500Hz
{
    // Configure the envelope for typical pitch envelope behavior
    // Fast attack, exponential decay to create the characteristic kick drum pitch sweep
    envelope->setWarmUpDuration(0.0f);  // No warm-up for pitch envelope
    envelope->setAttack(0.001f);        // Very fast attack (1ms)
    envelope->setDecay(0.1f);           // 100ms decay
    envelope->setSustain(0.0f);         // Decay to zero (base pitch)
    envelope->setRelease(0.05f);        // 50ms release
    
    // Use exponential curves for natural pitch decay
    envelope->setAttackCurve(CurveType::EXPONENTIAL);
    envelope->setDecayCurve(CurveType::EXPONENTIAL);
    envelope->setReleaseCurve(CurveType::EXPONENTIAL);
}

PitchEnvelope::~PitchEnvelope() {
    delete envelope;
}

// ============================================================================
// Pitch Envelope Parameters
// ============================================================================

void PitchEnvelope::setDepth(float newDepth) {
    // Clamp to valid range (0Hz to 2000Hz)
    depth = std::clamp(newDepth, 0.0f, 2000.0f);
}

// ============================================================================
// Envelope Control
// ============================================================================

void PitchEnvelope::trigger() {
    envelope->trigger();
}

void PitchEnvelope::release() {
    envelope->release();
}

void PitchEnvelope::reset() {
    envelope->reset();
}

void PitchEnvelope::advance() {
    envelope->advance();
}

float PitchEnvelope::getValue() const {
    // Get the envelope value (0.0 to 1.0)
    float envelopeValue = envelope->getValue();
    
    // Apply depth to convert to frequency offset in Hz
    // When envelope is at peak (1.0), pitch offset is at maximum (depth)
    // When envelope is at zero (0.0), pitch offset is zero (base pitch)
    return envelopeValue * depth;
}

bool PitchEnvelope::isActive() const {
    return envelope->isActive();
}

void PitchEnvelope::setSampleRate(float sampleRate) {
    envelope->setSampleRate(sampleRate);
}

} // namespace KickDrum
