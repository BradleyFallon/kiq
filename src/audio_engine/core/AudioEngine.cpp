#include "AudioEngine.h"
#include "../voice/VoiceAllocator.h"
#include "../effects/EffectsChain.h"
#include "../parameters/ParameterManager.h"
#include "../parameters/ParameterEventQueue.h"
#include "../envelopes/DualPhaseEnvelope.h"
#include "../utils/DSPUtils.h"
#include <memory>
#include <cstring>
#include <iostream>
#include <cmath>

namespace KickDrum {

class AudioEngine::Impl {
public:
    float sampleRate = 48000.0f;
    std::unique_ptr<VoiceAllocator> voiceAllocator;
    std::unique_ptr<ParameterManager> parameterManager;
    std::unique_ptr<EffectsChain> effectsChain;
    std::unique_ptr<ParameterEventQueue> parameterEventQueue;
    
    // Master output level (0.0 to 1.0)
    float masterLevel = 0.8f;
    
    // Safety features
    bool enableSoftClipping = true;
    bool enableNaNDetection = true;
    
    // Error tracking
    size_t nanDetectionCount = 0;
    size_t softClipCount = 0;
    
    // Temporary mono buffer for voice rendering
    std::vector<float> monoBuffer;
    
    // Temporary buffer for parameter events
    std::vector<ParameterEvent> currentEvents;
    
    // Apply a parameter change to all voices
    void applyParameterToVoices(const std::string& parameterId, float value);
    
    // Apply a parameter change to effects chain
    void applyParameterToEffects(const std::string& parameterId, float value);
};

AudioEngine::AudioEngine() : pImpl(std::make_unique<Impl>()) {
    pImpl->voiceAllocator = std::make_unique<VoiceAllocator>();
    pImpl->effectsChain = std::make_unique<EffectsChain>();
    pImpl->parameterEventQueue = std::make_unique<ParameterEventQueue>();
}

AudioEngine::~AudioEngine() = default;

void AudioEngine::initialize(float sampleRate) {
    pImpl->sampleRate = sampleRate;
    
    // Initialize voice allocator
    pImpl->voiceAllocator->initialize(sampleRate);
    
    // Initialize effects chain
    pImpl->effectsChain->initialize(sampleRate);
    
    // Initialize parameter manager
    pImpl->parameterManager = std::make_unique<ParameterManager>();
    pImpl->parameterManager->registerAllSynthesisParameters();
}

void AudioEngine::processBlock(float* outputBuffer, size_t numSamples, size_t numChannels) {
    // Validate inputs
    if (outputBuffer == nullptr || numSamples == 0 || numChannels == 0) {
        return;
    }
    
    // Ensure mono buffer is large enough
    if (pImpl->monoBuffer.size() < numSamples) {
        pImpl->monoBuffer.resize(numSamples);
    }
    
    // Get parameter events for this buffer
    pImpl->parameterEventQueue->getEventsForBuffer(pImpl->currentEvents);
    
    // If no events, process the entire buffer at once (fast path)
    if (pImpl->currentEvents.empty()) {
        // Step 1: Process voices (render to mono buffer)
        pImpl->voiceAllocator->renderBuffer(pImpl->monoBuffer.data(), static_cast<int>(numSamples));
        
        // Step 2: Apply effects chain (process mono buffer in-place)
        for (size_t i = 0; i < numSamples; ++i) {
            pImpl->monoBuffer[i] = pImpl->effectsChain->process(pImpl->monoBuffer[i]);
        }
    }
    else {
        // Process buffer in chunks between parameter events (sample-accurate path)
        size_t currentSample = 0;
        size_t eventIndex = 0;
        
        while (currentSample < numSamples) {
            // Find the next event within this buffer
            size_t nextEventSample = numSamples;
            if (eventIndex < pImpl->currentEvents.size()) {
                const auto& event = pImpl->currentEvents[eventIndex];
                // Clamp event offset to buffer size
                nextEventSample = std::min(static_cast<size_t>(event.sampleOffset), numSamples);
            }
            
            // Process samples up to the next event
            size_t samplesToProcess = nextEventSample - currentSample;
            if (samplesToProcess > 0) {
                // Render voices for this chunk
                float* chunkBuffer = pImpl->monoBuffer.data() + currentSample;
                
                // Clear chunk buffer
                for (size_t i = 0; i < samplesToProcess; ++i) {
                    chunkBuffer[i] = 0.0f;
                }
                
                // Mix all active voices into chunk
                for (int v = 0; v < pImpl->voiceAllocator->getNumVoices(); ++v) {
                    Voice& voice = pImpl->voiceAllocator->getVoice(v);
                    if (voice.isActive()) {
                        for (size_t i = 0; i < samplesToProcess; ++i) {
                            chunkBuffer[i] += voice.renderSample();
                        }
                    }
                }
                
                // Apply effects to chunk
                for (size_t i = 0; i < samplesToProcess; ++i) {
                    chunkBuffer[i] = pImpl->effectsChain->process(chunkBuffer[i]);
                }
                
                currentSample = nextEventSample;
            }
            
            // Apply parameter event if we've reached one
            if (eventIndex < pImpl->currentEvents.size() && 
                currentSample == static_cast<size_t>(pImpl->currentEvents[eventIndex].sampleOffset)) {
                const auto& event = pImpl->currentEvents[eventIndex];
                
                // Update parameter in manager
                pImpl->parameterManager->setParameterValue(event.parameterId, event.value);
                
                // Apply to voices
                pImpl->applyParameterToVoices(event.parameterId, event.value);
                
                // Apply to effects
                pImpl->applyParameterToEffects(event.parameterId, event.value);
                
                eventIndex++;
            }
        }
    }
    
    // Step 3: Apply master level
    DSPUtils::applyMasterLevel(pImpl->monoBuffer.data(), numSamples, pImpl->masterLevel);
    
    // Step 4: Check for NaN/infinity and recover if needed
    if (pImpl->enableNaNDetection) {
        size_t invalidIndex = 0;
        if (!DSPUtils::isBufferValid(pImpl->monoBuffer.data(), numSamples, &invalidIndex)) {
            // Invalid values detected - log error and sanitize
            std::cerr << "AudioEngine: NaN/Infinity detected at sample " << invalidIndex 
                      << " - resetting synthesis state" << std::endl;
            
            // Sanitize the buffer (replace invalid values with zero)
            size_t invalidCount = DSPUtils::sanitizeBuffer(pImpl->monoBuffer.data(), numSamples);
            pImpl->nanDetectionCount += invalidCount;
            
            // Reset synthesis state to prevent further issues
            pImpl->voiceAllocator->releaseAll();
            pImpl->effectsChain->reset();
            
            // Log context for debugging
            std::cerr << "AudioEngine: Reset " << invalidCount << " invalid samples. "
                      << "Total NaN detections: " << pImpl->nanDetectionCount << std::endl;
        }
    }
    
    // Step 5: Apply soft clipping to prevent hard clipping
    if (pImpl->enableSoftClipping) {
        // Check if any samples exceed ±1.0 before clipping
        bool needsClipping = false;
        for (size_t i = 0; i < numSamples; ++i) {
            if (std::abs(pImpl->monoBuffer[i]) > 1.0f) {
                needsClipping = true;
                pImpl->softClipCount++;
            }
        }
        
        // Apply soft clipping
        DSPUtils::softClipBuffer(pImpl->monoBuffer.data(), numSamples);
        
        // Log if clipping occurred (only occasionally to avoid spam)
        if (needsClipping && (pImpl->softClipCount % 1000 == 0)) {
            std::cerr << "AudioEngine: Soft clipping applied (total: " 
                      << pImpl->softClipCount << " samples)" << std::endl;
        }
    }
    
    // Step 6: Copy mono buffer to output (handle mono/stereo)
    if (numChannels == 1) {
        // Mono output - direct copy
        std::memcpy(outputBuffer, pImpl->monoBuffer.data(), numSamples * sizeof(float));
    } else if (numChannels == 2) {
        // Stereo output - duplicate mono to both channels
        for (size_t i = 0; i < numSamples; ++i) {
            outputBuffer[i * 2] = pImpl->monoBuffer[i];      // Left
            outputBuffer[i * 2 + 1] = pImpl->monoBuffer[i];  // Right
        }
    } else {
        // Multi-channel output - duplicate mono to all channels
        for (size_t i = 0; i < numSamples; ++i) {
            for (size_t ch = 0; ch < numChannels; ++ch) {
                outputBuffer[i * numChannels + ch] = pImpl->monoBuffer[i];
            }
        }
    }
}

void AudioEngine::noteOn(int note, float velocity) {
    if (pImpl->voiceAllocator) {
        pImpl->voiceAllocator->allocateVoice(note, velocity);
    }
}

void AudioEngine::noteOff(int note) {
    if (pImpl->voiceAllocator) {
        pImpl->voiceAllocator->releaseVoice(note);
    }
}

void AudioEngine::allNotesOff() {
    if (pImpl->voiceAllocator) {
        pImpl->voiceAllocator->releaseAll();
    }
}

ParameterManager* AudioEngine::getParameterManager() {
    return pImpl->parameterManager.get();
}

float AudioEngine::getSampleRate() const {
    return pImpl->sampleRate;
}

void AudioEngine::setMasterLevel(float level) {
    // Clamp to valid range
    pImpl->masterLevel = DSPUtils::clamp(level, 0.0f, 1.0f);
}

float AudioEngine::getMasterLevel() const {
    return pImpl->masterLevel;
}

void AudioEngine::setSoftClippingEnabled(bool enable) {
    pImpl->enableSoftClipping = enable;
}

bool AudioEngine::isSoftClippingEnabled() const {
    return pImpl->enableSoftClipping;
}

void AudioEngine::setNaNDetectionEnabled(bool enable) {
    pImpl->enableNaNDetection = enable;
}

bool AudioEngine::isNaNDetectionEnabled() const {
    return pImpl->enableNaNDetection;
}

EffectsChain* AudioEngine::getEffectsChain() {
    return pImpl->effectsChain.get();
}

VoiceAllocator* AudioEngine::getVoiceAllocator() {
    return pImpl->voiceAllocator.get();
}

ParameterEventQueue* AudioEngine::getParameterEventQueue() {
    return pImpl->parameterEventQueue.get();
}

void AudioEngine::setParameter(const std::string& parameterId, float value) {
    // Update parameter manager
    if (pImpl->parameterManager) {
        pImpl->parameterManager->setParameterValue(parameterId, value);
    }
    
    // Schedule immediate event (offset 0) for sample-accurate application
    if (pImpl->parameterEventQueue) {
        pImpl->parameterEventQueue->addEvent(parameterId, value, 0);
    }
}

// Helper method to apply parameter changes to voices
void AudioEngine::Impl::applyParameterToVoices(const std::string& parameterId, float value) {
    if (!voiceAllocator) {
        return;
    }
    
    // Apply parameter to all voices
    for (int i = 0; i < voiceAllocator->getNumVoices(); ++i) {
        Voice& voice = voiceAllocator->getVoice(i);
        
        // Generator parameters
        if (parameterId == "basePitch") {
            voice.setBasePitch(value);
        }
        else if (parameterId == "sineLevel") {
            voice.setSineLevel(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "harmonicRatio") {
            voice.setHarmonicRatio(value);
        }
        else if (parameterId == "harmonicLevel") {
            voice.setHarmonicLevel(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "harmonicModDepth") {
            voice.setHarmonicModDepth(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "noiseLevel") {
            voice.setNoiseLevel(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "noiseModDepth") {
            voice.setNoiseModDepth(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "pitchTracking") {
            voice.setPitchTrackingEnabled(value > 0.5f);  // 0 = off, 1 = on
        }
        // Envelope parameters
        else if (parameterId == "warmUpDuration") {
            voice.getAmplitudeEnvelope().setWarmUpDuration(value / 1000.0f);  // Convert ms to seconds
        }
        else if (parameterId == "warmUpStartFreq") {
            voice.getAmplitudeEnvelope().setWarmUpStartFrequency(value);
        }
        else if (parameterId == "warmUpAmplitude") {
            voice.getAmplitudeEnvelope().setWarmUpAmplitude(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "attack") {
            voice.getAmplitudeEnvelope().setAttack(value / 1000.0f);  // Convert ms to seconds
        }
        else if (parameterId == "decay") {
            voice.getAmplitudeEnvelope().setDecay(value / 1000.0f);  // Convert ms to seconds
        }
        else if (parameterId == "sustain") {
            voice.getAmplitudeEnvelope().setSustain(value / 100.0f);  // Convert from percentage
        }
        else if (parameterId == "release") {
            voice.getAmplitudeEnvelope().setRelease(value / 1000.0f);  // Convert ms to seconds
        }
        else if (parameterId == "pitchEnvelopeDepth") {
            voice.getPitchEnvelope().setDepth(value);
        }
        else if (parameterId == "attackCurve") {
            voice.getAmplitudeEnvelope().setAttackCurve(static_cast<CurveType>(static_cast<int>(value)));
        }
        else if (parameterId == "decayCurve") {
            voice.getAmplitudeEnvelope().setDecayCurve(static_cast<CurveType>(static_cast<int>(value)));
        }
        else if (parameterId == "releaseCurve") {
            voice.getAmplitudeEnvelope().setReleaseCurve(static_cast<CurveType>(static_cast<int>(value)));
        }
    }
}

// Helper method to apply parameter changes to effects
void AudioEngine::Impl::applyParameterToEffects(const std::string& parameterId, float value) {
    if (!effectsChain) {
        return;
    }
    
    // Compressor parameters
    if (parameterId == "compressorThreshold") {
        effectsChain->getCompressor().setThreshold(value);
    }
    else if (parameterId == "compressorRatio") {
        effectsChain->getCompressor().setRatio(value);
    }
    else if (parameterId == "compressorAttack") {
        effectsChain->getCompressor().setAttack(value / 1000.0f);  // Convert ms to seconds
    }
    else if (parameterId == "compressorRelease") {
        effectsChain->getCompressor().setRelease(value / 1000.0f);  // Convert ms to seconds
    }
    else if (parameterId == "compressorMix") {
        effectsChain->getCompressor().setMix(value / 100.0f);  // Convert from percentage
    }
    // Reverb parameters
    else if (parameterId == "reverbRoomSize") {
        effectsChain->getReverb().setRoomSize(value / 100.0f);  // Convert from percentage
    }
    else if (parameterId == "reverbDecayTime") {
        effectsChain->getReverb().setDecayTime(value);
    }
    else if (parameterId == "reverbDamping") {
        effectsChain->getReverb().setDamping(value / 100.0f);  // Convert from percentage
    }
    else if (parameterId == "reverbMix") {
        effectsChain->getReverb().setMix(value / 100.0f);  // Convert from percentage
    }
    // Master level
    else if (parameterId == "masterLevel") {
        masterLevel = value / 100.0f;  // Convert from percentage
    }
}

} // namespace KickDrum
