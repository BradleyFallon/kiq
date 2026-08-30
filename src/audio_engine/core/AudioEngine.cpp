#include "AudioEngine.h"

#include "../parameters/KickParams.h"
#include "../parameters/ParameterEventQueue.h"
#include "../parameters/ParameterManager.h"
#include "../utils/DSPUtils.h"
#include "../voice/VoiceAllocator.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

namespace KickDrum {

class AudioEngine::Impl {
public:
    float sampleRate = 48000.0f;
    KickParams params = kDefaultKickParams;
    std::unique_ptr<VoiceAllocator> voiceAllocator;
    std::unique_ptr<ParameterManager> parameterManager;
    std::unique_ptr<ParameterEventQueue> parameterEventQueue;
    bool enableSoftClipping = true;
    bool enableNaNDetection = true;
    std::vector<float> monoBuffer;
    std::vector<ParameterEvent> currentEvents;

    bool applyParameter(const std::string& parameterId, float value) {
        const auto* spec = findKickParameterSpec(parameterId);
        if (!spec) {
            return false;
        }

        setKickParameter(params, spec->id, value);
        params = sanitizeKickParams(params);
        voiceAllocator->setParams(params);

        if (parameterManager) {
            for (const auto& parameterSpec : kKickParameterSpecs) {
                parameterManager->setParameterValue(
                    std::string(parameterSpec.key),
                    getKickParameter(params, parameterSpec.id));
            }
        }
        return true;
    }
};

AudioEngine::AudioEngine()
    : pImpl(std::make_unique<Impl>()) {
    pImpl->voiceAllocator = std::make_unique<VoiceAllocator>();
    pImpl->parameterEventQueue = std::make_unique<ParameterEventQueue>();
    pImpl->parameterManager = std::make_unique<ParameterManager>();
    pImpl->parameterManager->registerAllSynthesisParameters();
}

AudioEngine::~AudioEngine() = default;

void AudioEngine::initialize(float sampleRate) {
    pImpl->sampleRate = sampleRate > 0.0f ? sampleRate : 48000.0f;
    pImpl->voiceAllocator->initialize(pImpl->sampleRate);
    pImpl->voiceAllocator->setParams(pImpl->params);
    for (const auto& spec : kKickParameterSpecs) {
        pImpl->parameterManager->setParameterValue(
            std::string(spec.key), getKickParameter(pImpl->params, spec.id));
    }
    pImpl->parameterEventQueue->clear();
}

void AudioEngine::processBlock(float* outputBuffer, std::size_t numSamples,
                               std::size_t numChannels) {
    if (!outputBuffer || numSamples == 0 || numChannels == 0) {
        return;
    }

    if (pImpl->monoBuffer.size() < numSamples) {
        pImpl->monoBuffer.resize(numSamples);
    }
    std::fill_n(pImpl->monoBuffer.data(), numSamples, 0.0f);
    pImpl->parameterEventQueue->getEventsForBuffer(pImpl->currentEvents);

    std::size_t currentSample = 0;
    std::size_t eventIndex = 0;
    while (currentSample < numSamples) {
        while (eventIndex < pImpl->currentEvents.size() &&
               pImpl->currentEvents[eventIndex].sampleOffset <= currentSample) {
            const auto& event = pImpl->currentEvents[eventIndex++];
            pImpl->applyParameter(event.parameterId, event.value);
        }

        const std::size_t nextEventSample =
            eventIndex < pImpl->currentEvents.size()
                ? std::min<std::size_t>(pImpl->currentEvents[eventIndex].sampleOffset,
                                        numSamples)
                : numSamples;
        const std::size_t samplesToRender = nextEventSample - currentSample;
        if (samplesToRender > 0) {
            pImpl->voiceAllocator->renderBuffer(
                pImpl->monoBuffer.data() + currentSample,
                static_cast<int>(samplesToRender));
            currentSample = nextEventSample;
        }
    }

    // Events exactly at the end establish state for the next block.
    while (eventIndex < pImpl->currentEvents.size()) {
        const auto& event = pImpl->currentEvents[eventIndex++];
        pImpl->applyParameter(event.parameterId, event.value);
    }

    if (pImpl->enableNaNDetection) {
        std::size_t invalidIndex = 0;
        if (!DSPUtils::isBufferValid(pImpl->monoBuffer.data(), numSamples, &invalidIndex)) {
            const std::size_t invalidCount =
                DSPUtils::sanitizeBuffer(pImpl->monoBuffer.data(), numSamples);
            pImpl->voiceAllocator->releaseAll();
            std::cerr << "AudioEngine: reset " << invalidCount
                      << " invalid samples at index " << invalidIndex << '\n';
        }
    }

    if (pImpl->enableSoftClipping) {
        DSPUtils::softClipBuffer(pImpl->monoBuffer.data(), numSamples);
    }

    if (numChannels == 1) {
        std::memcpy(outputBuffer, pImpl->monoBuffer.data(), numSamples * sizeof(float));
        return;
    }
    for (std::size_t sample = 0; sample < numSamples; ++sample) {
        for (std::size_t channel = 0; channel < numChannels; ++channel) {
            outputBuffer[sample * numChannels + channel] = pImpl->monoBuffer[sample];
        }
    }
}

void AudioEngine::noteOn(int note, float velocity) {
    pImpl->voiceAllocator->allocateVoice(note, velocity);
}

void AudioEngine::noteOff(int note) {
    pImpl->voiceAllocator->releaseVoice(note);
}

void AudioEngine::allNotesOff() {
    pImpl->voiceAllocator->releaseAll();
}

ParameterManager* AudioEngine::getParameterManager() {
    return pImpl->parameterManager.get();
}

float AudioEngine::getSampleRate() const {
    return pImpl->sampleRate;
}

void AudioEngine::setOutputGain(float gain) {
    pImpl->applyParameter("outputGain", gain);
}

float AudioEngine::getOutputGain() const {
    return pImpl->params.outputGain;
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

VoiceAllocator* AudioEngine::getVoiceAllocator() {
    return pImpl->voiceAllocator.get();
}

ParameterEventQueue* AudioEngine::getParameterEventQueue() {
    return pImpl->parameterEventQueue.get();
}

void AudioEngine::setParameter(const std::string& parameterId, float value) {
    pImpl->applyParameter(parameterId, value);
}

} // namespace KickDrum
