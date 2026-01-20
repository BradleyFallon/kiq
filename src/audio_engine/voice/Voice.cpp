#include "Voice.h"
#include "../utils/DSPUtils.h"
#include <cmath>

namespace KickDrum {

Voice::Voice()
    : amplitudeEnvelope_(44100.0f)  // Default sample rate
    , pitchEnvelope_(44100.0f)      // Default sample rate
    , basePitch_(50.0f)             // Default base pitch (50Hz)
    , sineLevel_(0.8f)              // Default sine level
    , harmonicLevel_(0.3f)          // Default harmonic level
    , noiseLevel_(0.2f)             // Default noise level
    , harmonicRatio_(2.0f)          // Default harmonic ratio (2x)
    , harmonicModDepth_(0.5f)       // Default harmonic mod depth
    , noiseModDepth_(0.7f)          // Default noise mod depth
    , pitchTrackingEnabled_(true)   // Pitch tracking enabled by default
    , pitchBendValue_(0.0f)         // No pitch bend by default
    , pitchBendRange_(2.0f)         // Default ±2 semitones
    , note_(60)                     // Default MIDI note (middle C)
    , velocity_(1.0f)               // Default velocity (full)
    , age_(0)                       // Voice age starts at 0
    , sampleRate_(44100.0f)         // Default sample rate
    , initialized_(false)           // Not initialized yet
{
}

void Voice::initialize(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Initialize generators
    sineDriver_.initialize(sampleRate);
    harmonicMembrane_.initialize(sampleRate);
    // NoiseGenerator doesn't need sample rate initialization
    
    // Initialize envelopes
    amplitudeEnvelope_.setSampleRate(sampleRate);
    pitchEnvelope_.setSampleRate(sampleRate);
    
    // Set initial frequencies
    sineDriver_.setFrequency(basePitch_);
    harmonicMembrane_.setBaseFrequency(basePitch_);
    harmonicMembrane_.setRatio(harmonicRatio_);
    
    // Set initial modulation depths
    ringModHarmonic_.setDepth(harmonicModDepth_);
    ringModNoise_.setDepth(noiseModDepth_);
    
    initialized_ = true;
}

void Voice::trigger(int note, float velocity) {
    // Store note and velocity
    note_ = note;
    velocity_ = velocity;
    age_ = 0;
    
    // If pitch tracking is enabled, set pitch from MIDI note
    if (pitchTrackingEnabled_) {
        setPitchFromMIDINote(note);
    }
    
    // Reset all generators to ensure consistent phase
    sineDriver_.reset();
    harmonicMembrane_.reset();
    noiseGenerator_.reset();
    
    // Trigger envelopes
    amplitudeEnvelope_.trigger();
    pitchEnvelope_.trigger();
}

void Voice::release() {
    // Release envelopes
    amplitudeEnvelope_.release();
    pitchEnvelope_.release();
}

bool Voice::isActive() const {
    // Voice is active if amplitude envelope is active
    return amplitudeEnvelope_.isActive();
}

void Voice::setBasePitch(float pitch) {
    basePitch_ = pitch;
    // Update sine driver frequency immediately
    // (will be modulated by pitch envelope during rendering)
    sineDriver_.setFrequency(pitch);
    harmonicMembrane_.setBaseFrequency(pitch);
}

void Voice::setSineLevel(float level) {
    sineLevel_ = level;
}

void Voice::setHarmonicLevel(float level) {
    harmonicLevel_ = level;
}

void Voice::setNoiseLevel(float level) {
    noiseLevel_ = level;
}

void Voice::setHarmonicRatio(float ratio) {
    harmonicRatio_ = ratio;
    harmonicMembrane_.setRatio(ratio);
}

void Voice::setHarmonicModDepth(float depth) {
    harmonicModDepth_ = depth;
    ringModHarmonic_.setDepth(depth);
}

void Voice::setNoiseModDepth(float depth) {
    noiseModDepth_ = depth;
    ringModNoise_.setDepth(depth);
}

void Voice::setPitchTrackingEnabled(bool enabled) {
    pitchTrackingEnabled_ = enabled;
}

void Voice::setPitchFromMIDINote(int note) {
    // Convert MIDI note to frequency using standard MIDI tuning
    float frequency = DSPUtils::midiNoteToFrequency(note);
    
    // Set the base pitch to the calculated frequency
    setBasePitch(frequency);
}

void Voice::setPitchBend(float bendValue, float bendRange) {
    // Store pitch bend parameters
    pitchBendValue_ = bendValue;
    pitchBendRange_ = bendRange;
}

float Voice::renderSample() {
    if (!initialized_ || !isActive()) {
        return 0.0f;
    }
    
    // Step 1: Advance all envelopes
    amplitudeEnvelope_.advance();
    pitchEnvelope_.advance();
    age_++;
    
    // Step 2: Calculate current pitch with pitch envelope modulation and pitch bend
    float pitchOffset = pitchEnvelope_.getValue();
    
    // Apply pitch bend: convert semitones to frequency ratio
    // Formula: frequency_ratio = 2^(semitones/12)
    float pitchBendSemitones = pitchBendValue_ * pitchBendRange_;
    float pitchBendRatio = std::pow(2.0f, pitchBendSemitones / 12.0f);
    
    // Apply both pitch envelope and pitch bend to base pitch
    float currentPitch = (basePitch_ + pitchOffset) * pitchBendRatio;
    
    // Update generator frequencies
    sineDriver_.setFrequency(currentPitch);
    harmonicMembrane_.setBaseFrequency(currentPitch);
    
    // Step 3: Generate samples from all three generators
    float sineSample = sineDriver_.generate();
    float harmonicSample = harmonicMembrane_.generate();
    float noiseSample = noiseGenerator_.generate();
    
    // Step 4: Apply ring modulation to harmonic and noise
    float modulatedHarmonic = ringModHarmonic_.process(sineSample, harmonicSample);
    float modulatedNoise = ringModNoise_.process(sineSample, noiseSample);
    
    // Step 5: Mix generators with their respective levels
    float mixed = (sineSample * sineLevel_) +
                  (modulatedHarmonic * harmonicLevel_) +
                  (modulatedNoise * noiseLevel_);
    
    // Step 6: Apply amplitude envelope and velocity scaling
    float amplitudeEnv = amplitudeEnvelope_.getValue();
    float output = mixed * amplitudeEnv * velocity_;
    
    return output;
}

void Voice::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update all components with new sample rate
    sineDriver_.initialize(sampleRate);
    harmonicMembrane_.initialize(sampleRate);
    amplitudeEnvelope_.setSampleRate(sampleRate);
    pitchEnvelope_.setSampleRate(sampleRate);
}

} // namespace KickDrum
