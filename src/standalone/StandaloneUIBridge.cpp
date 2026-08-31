#include "StandaloneUIBridge.h"

#include "AudioEngine.h"
#include "ParameterEventQueue.h"
#include "audio/CoreAudioInterface.h"
#include "midi/CoreMIDIInterface.h"
#include "midi/MIDIHandler.h"

#include <algorithm>
#include <string>

namespace KickDrum::Standalone {

namespace {

std::size_t parameterIndex(KickParameterId id) {
    return static_cast<std::size_t>(id);
}

} // namespace

StandaloneUIBridge::StandaloneUIBridge() {
    for (const auto& spec : kKickParameterSpecs) {
        values_[parameterIndex(spec.id)] = getDefaultKickParameter(spec.id);
    }
}

StandaloneUIBridge::~StandaloneUIBridge() {
    shutdown();
}

bool StandaloneUIBridge::initialize() {
    shutdown();

    audioEngine_ = std::make_unique<AudioEngine>();
    audioInterface_ = std::make_unique<CoreAudioInterface>(audioEngine_.get());
    if (!audioInterface_->initialize()) {
        shutdown();
        return false;
    }

    midiHandler_ = std::make_unique<MIDIHandler>(
        audioEngine_->getVoiceAllocator(), audioEngine_->getParameterManager());
    midiInterface_ = std::make_unique<CoreMIDIInterface>(midiHandler_.get());
    if (midiInterface_->initialize()) {
        const auto devices = midiInterface_->getAvailableDevices();
        if (!devices.empty()) {
            midiInterface_->connectToDeviceByIndex(0);
        }
    }

    if (!audioInterface_->start()) {
        shutdown();
        return false;
    }
    return true;
}

void StandaloneUIBridge::shutdown() {
    if (audioInterface_) {
        audioInterface_->stop();
    }
    if (midiInterface_) {
        midiInterface_->disconnect();
    }
    midiInterface_.reset();
    midiHandler_.reset();
    audioInterface_.reset();
    audioEngine_.reset();
}

float StandaloneUIBridge::getParameter(KickParameterId id) {
    const auto index = parameterIndex(id);
    return index < values_.size() ? values_[index] : 0.0f;
}

void StandaloneUIBridge::beginParameterEdit(KickParameterId) {
}

void StandaloneUIBridge::performParameterEdit(KickParameterId id, float plainValue) {
    const auto* spec = findKickParameterSpec(id);
    if (!spec) {
        return;
    }

    const float value = std::clamp(plainValue, spec->minimum, spec->maximum);
    values_[parameterIndex(id)] = value;
    if (audioEngine_) {
        audioEngine_->getParameterEventQueue()->addEvent(
            std::string(spec->key), value, 0);
    }
}

void StandaloneUIBridge::endParameterEdit(KickParameterId) {
}

void StandaloneUIBridge::triggerAudition() {
    if (audioEngine_) {
        audioEngine_->enqueueNoteOn(36, 1.0f);
    }
}

float StandaloneUIBridge::getOutputPeak() {
    return audioEngine_ ? audioEngine_->getOutputPeak() : 0.0f;
}

bool StandaloneUIBridge::getOutputClip() {
    return audioEngine_ && audioEngine_->getOutputClip();
}

} // namespace KickDrum::Standalone
