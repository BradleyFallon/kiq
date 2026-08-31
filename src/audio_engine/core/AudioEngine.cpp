#include "AudioEngine.h"

#include "../parameters/KickParams.h"
#include "../parameters/ParameterEventQueue.h"
#include "../parameters/ParameterManager.h"
#include "../utils/DSPUtils.h"
#include "../voice/VoiceAllocator.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
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
    std::atomic<std::uint32_t> pendingNoteOn {0};
    std::atomic<float> outputPeak {0.0f};
    std::atomic<bool> outputClip {false};

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
    pImpl->pendingNoteOn.store(0, std::memory_order_release);
    pImpl->outputPeak.store(0.0f, std::memory_order_release);
    pImpl->outputClip.store(false, std::memory_order_release);
}

void AudioEngine::prepare(std::size_t maxSamplesPerBlock) {
    if (pImpl->monoBuffer.size() < maxSamplesPerBlock) {
        pImpl->monoBuffer.resize(maxSamplesPerBlock);
    }
    pImpl->currentEvents.reserve(64);
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

    const std::uint32_t packedNote =
        pImpl->pendingNoteOn.exchange(0, std::memory_order_acquire);
    if (packedNote != 0) {
        const int note = static_cast<int>((packedNote & 0xffu) - 1u);
        const float velocity = static_cast<float>((packedNote >> 8u) & 0xffffu) /
                               65535.0f;
        pImpl->voiceAllocator->allocateVoice(note, velocity);
    }

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

    float rawPeak = 0.0f;
    for (std::size_t sample = 0; sample < numSamples; ++sample) {
        rawPeak = std::max(rawPeak, std::abs(pImpl->monoBuffer[sample]));
    }

    if (pImpl->enableSoftClipping) {
        DSPUtils::softClipBuffer(pImpl->monoBuffer.data(), numSamples);
    }

    float renderedPeak = 0.0f;
    for (std::size_t sample = 0; sample < numSamples; ++sample) {
        renderedPeak = std::max(renderedPeak, std::abs(pImpl->monoBuffer[sample]));
    }
    const float clampedPeak = std::clamp(renderedPeak, 0.0f, 1.0f);
    float latchedPeak = pImpl->outputPeak.load(std::memory_order_relaxed);
    while (clampedPeak > latchedPeak &&
           !pImpl->outputPeak.compare_exchange_weak(
               latchedPeak, clampedPeak,
               std::memory_order_release, std::memory_order_relaxed)) {
    }
    if (rawPeak >= 1.0f) {
        pImpl->outputClip.store(true, std::memory_order_release);
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

void AudioEngine::enqueueNoteOn(int note, float velocity) {
    const std::uint32_t encodedNote =
        static_cast<std::uint32_t>(std::clamp(note, 0, 127) + 1);
    const std::uint32_t encodedVelocity = static_cast<std::uint32_t>(
        std::lround(std::clamp(velocity, 0.0f, 1.0f) * 65535.0f));
    pImpl->pendingNoteOn.store(encodedNote | (encodedVelocity << 8u),
                               std::memory_order_release);
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

float AudioEngine::getOutputPeak() const {
    return pImpl->outputPeak.exchange(0.0f, std::memory_order_acq_rel);
}

bool AudioEngine::getOutputClip() const {
    return pImpl->outputClip.exchange(false, std::memory_order_acq_rel);
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
