#include "MIDIHandler.h"
#include <algorithm>
#include <utility>

namespace KickDrum {

MIDIHandler::MIDIHandler(VoiceAllocator* voiceAllocator, ParameterManager* parameterManager)
    : voiceAllocator_(voiceAllocator)
    , parameterManager_(parameterManager)
    , ccLearnActive_(false)
    , ccLearnParameterId_("")
    , currentPitchBend_(0.0f)
    , pitchBendRange_(2.0f)  // Default ±2 semitones
{
}

void MIDIHandler::processMIDIMessage(const MIDIMessage& message) {
    // Route message based on type
    if (message.isNoteOn()) {
        // Note-on with non-zero velocity
        handleNoteOn(message.data1, message.data2);
    } else if (message.isNoteOff()) {
        // Note-off or note-on with velocity 0
        handleNoteOff(message.data1);
    } else if (message.type == MIDIMessageType::CC) {
        // Control Change message
        handleCC(message.data1, message.data2);
    } else if (message.type == MIDIMessageType::PITCH_BEND) {
        // Pitch Bend message
        handlePitchBend(message.data1, message.data2);
    }
    // Other message types are not handled
}

void MIDIHandler::handleNoteOn(int note, int velocity) {
    // Normalize velocity from [0-127] to [0.0-1.0]
    float normalizedVelocity = normalizeVelocity(velocity);

    if (noteOnCallback_) {
        noteOnCallback_(note, normalizedVelocity);
        return;
    }
    if (voiceAllocator_ == nullptr) {
        return;
    }
    
    // Allocate and trigger a voice
    Voice* voice = voiceAllocator_->allocateVoice(note, normalizedVelocity);
    
    // Apply current pitch bend to the newly allocated voice
    if (voice != nullptr) {
        voice->setPitchBend(currentPitchBend_, pitchBendRange_);
    }
}

void MIDIHandler::handleNoteOff(int note) {
    if (voiceAllocator_ == nullptr) {
        return;
    }
    
    // Release the voice playing this note
    voiceAllocator_->releaseVoice(note);
}

void MIDIHandler::handleCC(int ccNumber, int ccValue) {
    // Handle CC learn mode first
    if (ccLearnActive_) {
        // Map this CC to the learn target parameter
        mapCCToParameter(ccNumber, ccLearnParameterId_);
        // Disable learn mode
        disableCCLearn();
        // Continue to process the CC value
    }
    
    // Check if this CC is mapped to a parameter
    auto it = ccMappings_.find(ccNumber);
    if (it != ccMappings_.end() && parameterManager_ != nullptr) {
        const std::string& parameterId = it->second;
        
        // Normalize CC value from [0-127] to [0.0-1.0]
        float normalizedValue = normalizeCCValue(ccValue);
        
        // Update the parameter using normalized value
        parameterManager_->setParameterNormalized(parameterId, normalizedValue);
    }
}

void MIDIHandler::handlePitchBend(int lsb, int msb) {
    if (voiceAllocator_ == nullptr) {
        return;
    }
    
    // Combine 7-bit LSB and MSB into 14-bit value
    int pitchBend14bit = (msb << 7) | lsb;
    
    // Center value is 8192 (0x2000)
    // Range is 0-16383 (0x0000-0x3FFF)
    const int centerValue = 8192;
    const float maxRange = 8192.0f;
    
    // Normalize to [-1.0, 1.0]
    currentPitchBend_ = (pitchBend14bit - centerValue) / maxRange;
    
    // Apply pitch bend to all active voices
    for (int i = 0; i < voiceAllocator_->getNumVoices(); ++i) {
        Voice& voice = voiceAllocator_->getVoice(i);
        if (voice.isActive()) {
            voice.setPitchBend(currentPitchBend_, pitchBendRange_);
        }
    }
}

bool MIDIHandler::mapCCToParameter(int ccNumber, const std::string& parameterId) {
    // Validate CC number range
    if (ccNumber < 0 || ccNumber > 127) {
        return false;
    }
    
    // Check if parameter exists (if we have a parameter manager)
    if (parameterManager_ != nullptr) {
        if (!parameterManager_->hasParameter(parameterId)) {
            return false;
        }
    }
    
    // Create the mapping
    ccMappings_[ccNumber] = parameterId;
    return true;
}

void MIDIHandler::unmapCC(int ccNumber) {
    ccMappings_.erase(ccNumber);
}

void MIDIHandler::clearAllCCMappings() {
    ccMappings_.clear();
}

std::string MIDIHandler::getMappedParameter(int ccNumber) const {
    auto it = ccMappings_.find(ccNumber);
    if (it != ccMappings_.end()) {
        return it->second;
    }
    return "";
}

bool MIDIHandler::isCCMapped(int ccNumber) const {
    return ccMappings_.find(ccNumber) != ccMappings_.end();
}

bool MIDIHandler::enableCCLearn(const std::string& parameterId) {
    // Check if parameter exists (if we have a parameter manager)
    if (parameterManager_ != nullptr) {
        if (!parameterManager_->hasParameter(parameterId)) {
            return false;
        }
    }
    
    // Enable learn mode
    ccLearnActive_ = true;
    ccLearnParameterId_ = parameterId;
    return true;
}

void MIDIHandler::disableCCLearn() {
    ccLearnActive_ = false;
    ccLearnParameterId_ = "";
}

void MIDIHandler::setParameterManager(ParameterManager* parameterManager) {
    parameterManager_ = parameterManager;
}

void MIDIHandler::setVoiceAllocator(VoiceAllocator* voiceAllocator) {
    voiceAllocator_ = voiceAllocator;
}

void MIDIHandler::setNoteOnCallback(NoteOnCallback callback) {
    noteOnCallback_ = std::move(callback);
}

void MIDIHandler::setPitchBendRange(float semitones) {
    // Clamp to reasonable range (0 to 24 semitones)
    pitchBendRange_ = std::max(0.0f, std::min(24.0f, semitones));
}

float MIDIHandler::normalizeVelocity(int velocity) const {
    // Clamp velocity to valid MIDI range [0-127]
    int clampedVelocity = std::max(0, std::min(127, velocity));
    
    // Normalize to [0.0-1.0]
    return static_cast<float>(clampedVelocity) / 127.0f;
}

float MIDIHandler::normalizeCCValue(int ccValue) const {
    // Clamp CC value to valid MIDI range [0-127]
    int clampedValue = std::max(0, std::min(127, ccValue));
    
    // Normalize to [0.0-1.0]
    return static_cast<float>(clampedValue) / 127.0f;
}

} // namespace KickDrum
