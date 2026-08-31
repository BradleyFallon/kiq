#include "KickSynthProcessor.h"
#include "KickSynthCIDs.h"
#include "AudioEngine.h"
#include "SampleLayerData.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// KickSynthProcessor
//------------------------------------------------------------------------
KickSynthProcessor::KickSynthProcessor()
    : audioEngine_(std::make_unique<KickDrum::AudioEngine>())
{
    // Set the controller class ID (will be defined in KickSynthCIDs.h)
    setControllerClass(kKickSynthControllerUID);
    for (const auto& spec : KickDrum::kKickParameterSpecs)
        stateParameterValues_[static_cast<std::size_t>(spec.id)].store(
            KickDrum::getDefaultKickParameter(spec.id),
            std::memory_order_relaxed);
}

//------------------------------------------------------------------------
KickSynthProcessor::~KickSynthProcessor()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::initialize(FUnknown* context)
{
    // Always call parent initialize first
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
        return result;

    //--- Create Audio In/Out buses
    // We are an instrument plugin, so we don't need audio input
    // Only add stereo output
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    
    //--- Create Event In/Out buses (MIDI)
    addEventInput(STR16("Event In"), 1);

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::terminate()
{
    // Nothing to do here
    return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::setActive(TBool state)
{
    if (state) // Activation
    {
        // Initialize audio engine with current sample rate
        if (processSetup.sampleRate > 0)
        {
            audioEngine_->initialize(static_cast<float>(processSetup.sampleRate));
        }
    }
    else // Deactivation
    {
        auditionPending_.store(false, std::memory_order_release);
        meterPeak_ = 0.0f;
        clipHoldSamplesRemaining_ = 0;
        audioEngine_->clearScheduledEvents();
        // Stop all notes
        audioEngine_->allNotesOff();
    }

    return AudioEffect::setActive(state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::process(ProcessData& data)
{
    // Process parameter changes
    if (data.inputParameterChanges)
    {
        processParameterChanges(data.inputParameterChanges);
    }

    // Process MIDI events
    if (data.inputEvents)
    {
        processMIDIEvents(data.inputEvents);
    }

    if (auditionPending_.exchange(false, std::memory_order_acquire))
    {
        audioEngine_->enqueueNoteOn(36, 1.0f);
    }

    // Check if we have audio output
    if (data.numOutputs == 0 || data.outputs[0].numChannels == 0)
    {
        audioEngine_->flushScheduledEvents();
        publishProcessedState();
        return kResultOk;
    }

    // Get output buffer info
    int32 numChannels = data.outputs[0].numChannels;
    int32 numSamples = data.numSamples;
    float** outputBuffers = data.outputs[0].channelBuffers32;

    // Check for valid output
    if (outputBuffers == nullptr || numSamples <= 0)
    {
        audioEngine_->flushScheduledEvents();
        publishProcessedState();
        return kResultOk;
    }

    // Process audio
    // We need to interleave the output for the audio engine
    // The audio engine expects interleaved samples
    const std::size_t requiredSamples = static_cast<std::size_t>(numSamples) *
                                        static_cast<std::size_t>(numChannels);
    if (interleavedBuffer_.size() < requiredSamples)
        interleavedBuffer_.resize(requiredSamples);
    
    audioEngine_->processBlock(interleavedBuffer_.data(), numSamples, numChannels);
    publishProcessedState();

    // De-interleave back to VST3 format
    for (int32 channel = 0; channel < numChannels; ++channel)
    {
        for (int32 sample = 0; sample < numSamples; ++sample)
        {
            const float value = interleavedBuffer_[sample * numChannels + channel];
            outputBuffers[channel][sample] = value;
        }
    }

    const float blockPeak = audioEngine_->getOutputPeak();
    if (blockPeak >= meterPeak_)
    {
        meterPeak_ = blockPeak;
    }
    else
    {
        constexpr double releaseSeconds = 0.15;
        const double sampleRate = std::max(processSetup.sampleRate, 1.0);
        const float release = static_cast<float>(
            std::exp(-static_cast<double>(numSamples) /
                     (sampleRate * releaseSeconds)));
        meterPeak_ = std::max(blockPeak, meterPeak_ * release);
    }

    if (audioEngine_->getOutputClip())
    {
        clipHoldSamplesRemaining_ = static_cast<int64>(
            std::max(processSetup.sampleRate, 1.0) * 0.5);
    }
    else
    {
        clipHoldSamplesRemaining_ = std::max<int64>(
            0, clipHoldSamplesRemaining_ - static_cast<int64>(numSamples));
    }
    const float normalizedPeak = meterPeak_;
    const bool outputClip = clipHoldSamplesRemaining_ > 0;
    if (data.outputParameterChanges)
    {
        auto publish = [&data](ParamID id, ParamValue value)
        {
            int32 queueIndex = 0;
            if (auto* queue = data.outputParameterChanges->addParameterData(id, queueIndex))
            {
                int32 pointIndex = 0;
                queue->addPoint(0, value, pointIndex);
            }
        };
        if (std::abs(normalizedPeak - previousOutputPeak_) > 1.0e-5f)
            publish(kParamOutputPeak, normalizedPeak);
        if (outputClip != previousOutputClip_)
            publish(kParamOutputClip, outputClip ? 1.0 : 0.0);
    }
    previousOutputPeak_ = normalizedPeak;
    previousOutputClip_ = outputClip;

    return kResultOk;
}

tresult PLUGIN_API KickSynthProcessor::notify(IMessage* message)
{
    if (!message)
        return kInvalidArgument;
    if (message->getMessageID() &&
        std::strcmp(message->getMessageID(), kAuditionMessageId) == 0)
    {
        auditionPending_.store(true, std::memory_order_release);
        return kResultOk;
    }
    if (message->getMessageID() &&
        std::strcmp(message->getMessageID(), kAuditionLoopMessageId) == 0)
    {
        int64 enabled = 0;
        double bpm = 120.0;
        if (auto* attributes = message->getAttributes())
        {
            attributes->getInt(kAuditionLoopEnabledAttribute, enabled);
            attributes->getFloat(kAuditionLoopBpmAttribute, bpm);
        }
        audioEngine_->setAuditionLoop(enabled != 0, static_cast<float>(bpm));
        return kResultOk;
    }
    if (message->getMessageID() &&
        std::strcmp(message->getMessageID(), kSampleLayerMessageId) == 0)
    {
        auto* attributes = message->getAttributes();
        if (!attributes)
            return kInvalidArgument;
        int64 enabled = 0;
        attributes->getInt(kSampleLayerEnabledAttribute, enabled);
        if (enabled == 0)
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            audioEngine_->setSampleLayer(nullptr);
            return kResultOk;
        }

        const void* bytes = nullptr;
        std::uint32_t byteCount = 0;
        if (attributes->getBinary(kSampleLayerDataAttribute, bytes, byteCount) !=
                kResultOk ||
            !bytes)
            return kInvalidArgument;

        constexpr std::size_t headerBytes = sizeof(std::uint32_t) +
                                            sizeof(float) +
                                            sizeof(std::uint32_t);
        if (byteCount < headerBytes)
            return kInvalidArgument;
        const auto* cursor = static_cast<const std::uint8_t*>(bytes);
        std::uint32_t formatVersion = 0;
        float sourceSampleRate = 0.0f;
        std::uint32_t sampleCount = 0;
        std::memcpy(&formatVersion, cursor, sizeof(formatVersion));
        cursor += sizeof(formatVersion);
        std::memcpy(&sourceSampleRate, cursor, sizeof(sourceSampleRate));
        cursor += sizeof(sourceSampleRate);
        std::memcpy(&sampleCount, cursor, sizeof(sampleCount));
        cursor += sizeof(sampleCount);
        if (formatVersion != 1 || sampleCount == 0 ||
            sampleCount > kMaximumSampleLayerSamples ||
            !std::isfinite(sourceSampleRate) ||
            sourceSampleRate < kMinimumSampleLayerRate ||
            sourceSampleRate > kMaximumSampleLayerRate ||
            byteCount != headerBytes + sampleCount * sizeof(float))
            return kInvalidArgument;

        auto layer = std::make_shared<KickDrum::SampleLayerData>();
        layer->sourceSampleRate = sourceSampleRate;
        layer->samples.resize(sampleCount);
        if (sampleCount > 0)
            std::memcpy(layer->samples.data(), cursor, sampleCount * sizeof(float));
        for (float& sample : layer->samples)
            sample = std::isfinite(sample) ? std::clamp(sample, -1.0f, 1.0f) : 0.0f;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            audioEngine_->setSampleLayer(std::move(layer));
        }
        return kResultOk;
    }
    return AudioEffect::notify(message);
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);
    uint32 version = 0;
    uint32 numParams = 0;
    if (!streamer.readInt32u(version) ||
        version == 0 || version > kStateFormatVersion ||
        !streamer.readInt32u(numParams) ||
        numParams > kMaximumStateParameterCount)
        return kResultFalse;

    // Parse the complete chunk before mutating live state. Starting from the
    // authoritative defaults makes old v1 chunks deterministic for every
    // parameter appended since that format was written.
    KickDrum::KickParams loadedParams = KickDrum::kDefaultKickParams;
    for (uint32 i = 0; i < numParams; ++i)
    {
        uint32 idLength = 0;
        if (!streamer.readInt32u(idLength) || idLength == 0 ||
            idLength > kMaximumStateParameterIdBytes)
            return kResultFalse;

        std::string paramId(idLength, '\0');
        if (streamer.readRaw(paramId.data(), idLength) != idLength)
            return kResultFalse;

        double value = 0.0;
        if (!streamer.readDouble(value))
            return kResultFalse;
        if (const auto* spec = KickDrum::findKickParameterSpec(paramId))
            KickDrum::setKickParameter(
                loadedParams, spec->id, static_cast<float>(value));
    }

    std::shared_ptr<const KickDrum::SampleLayerData> loadedSampleLayer;
    if (version >= 2)
    {
        uint32 hasSampleLayer = 0;
        if (!streamer.readInt32u(hasSampleLayer) || hasSampleLayer > 1)
            return kResultFalse;
        if (hasSampleLayer != 0)
        {
            double sourceSampleRate = 0.0;
            uint32 sampleCount = 0;
            if (!streamer.readDouble(sourceSampleRate) ||
                !streamer.readInt32u(sampleCount) ||
                sampleCount == 0 ||
                sampleCount > kMaximumSampleLayerSamples ||
                !std::isfinite(sourceSampleRate) ||
                sourceSampleRate < kMinimumSampleLayerRate ||
                sourceSampleRate > kMaximumSampleLayerRate)
                return kResultFalse;
            auto layer = std::make_shared<KickDrum::SampleLayerData>();
            layer->sourceSampleRate = static_cast<float>(sourceSampleRate);
            layer->samples.resize(sampleCount);
            const TSize sampleBytes = static_cast<TSize>(
                static_cast<std::size_t>(sampleCount) * sizeof(float));
            if (sampleBytes > 0 &&
                streamer.readRaw(layer->samples.data(), sampleBytes) != sampleBytes)
                return kResultFalse;
            for (float& sample : layer->samples)
                sample = std::isfinite(sample)
                             ? std::clamp(sample, -1.0f, 1.0f)
                             : 0.0f;
            loadedSampleLayer = std::move(layer);
        }
    }

    loadedParams = KickDrum::sanitizeKickParams(loadedParams);
    // Publish parameters and sample as one engine transaction. Advancing the
    // serial only after that publication lets getState() distinguish a pending
    // restore from the most recently completed audio block.
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        const std::uint64_t stateSerial =
            audioEngine_->setStateSnapshot(loadedParams, loadedSampleLayer);
        requestedState_ = loadedParams;
        requestedStateSerial_.store(stateSerial, std::memory_order_release);
    }

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    KickDrum::KickParams serializedParams;
    std::shared_ptr<const KickDrum::SampleLayerData> sampleLayer;
    {
        // A requested restore is authoritative until an audio callback reports
        // that it has crossed the corresponding block boundary. Otherwise use
        // the seqlock-protected snapshot of host automation actually processed.
        std::lock_guard<std::mutex> lock(stateMutex_);
        const auto requestedSerial =
            requestedStateSerial_.load(std::memory_order_acquire);
        const auto processedSerial =
            processedStateSerial_.load(std::memory_order_acquire);
        serializedParams = processedSerial < requestedSerial
                               ? requestedState_
                               : readProcessedState();
        sampleLayer = audioEngine_->getSampleLayer();
    }

    IBStreamer streamer(state, kLittleEndian);

    // Write version
    uint32 version = kStateFormatVersion;
    if (!streamer.writeInt32u(version))
        return kResultFalse;

    // Write number of parameters
    if (!streamer.writeInt32u(
            static_cast<uint32>(KickDrum::kKickParameterSpecs.size())))
        return kResultFalse;

    for (const auto& spec : KickDrum::kKickParameterSpecs)
    {
        const std::string_view paramId = spec.key;
        // Write parameter ID length
        if (!streamer.writeInt32u(static_cast<uint32>(paramId.length())))
            return kResultFalse;
        
        // Write parameter ID
        if (streamer.writeRaw(paramId.data(), static_cast<TSize>(paramId.length())) !=
            static_cast<TSize>(paramId.length()))
            return kResultFalse;

        // Write parameter value
        const float value = KickDrum::getKickParameter(serializedParams, spec.id);
        if (!streamer.writeDouble(static_cast<double>(value)))
            return kResultFalse;
    }

    const bool canPersistSample = sampleLayer &&
                                  !sampleLayer->samples.empty() &&
                                  sampleLayer->samples.size() <=
                                      kMaximumSampleLayerSamples;
    if (!streamer.writeInt32u(canPersistSample ? 1u : 0u))
        return kResultFalse;
    if (canPersistSample)
    {
        if (!streamer.writeDouble(sampleLayer->sourceSampleRate) ||
            !streamer.writeInt32u(
                static_cast<uint32>(sampleLayer->samples.size())))
            return kResultFalse;
        const TSize sampleBytes = static_cast<TSize>(
            sampleLayer->samples.size() * sizeof(float));
        if (sampleBytes > 0 &&
            streamer.writeRaw(sampleLayer->samples.data(), sampleBytes) != sampleBytes)
            return kResultFalse;
    }

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::setupProcessing(ProcessSetup& newSetup)
{
    // Store the setup
    tresult result = AudioEffect::setupProcessing(newSetup);
    
    if (result == kResultOk)
    {
        // Initialize audio engine with new sample rate
        audioEngine_->initialize(static_cast<float>(newSetup.sampleRate));
        audioEngine_->prepare(
            static_cast<std::size_t>(std::max(newSetup.maxSamplesPerBlock, 0)));
        interleavedBuffer_.resize(
            static_cast<std::size_t>(std::max(newSetup.maxSamplesPerBlock, 0)) * 2u);
    }

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
    // We support 32-bit and 64-bit processing
    if (symbolicSampleSize == kSample32)
        return kResultTrue;
    
    // We could also support 64-bit if needed
    // if (symbolicSampleSize == kSample64)
    //     return kResultTrue;

    return kResultFalse;
}

//------------------------------------------------------------------------
void KickSynthProcessor::processMIDIEvents(IEventList* events)
{
    if (!events)
        return;

    int32 numEvents = events->getEventCount();
    for (int32 i = 0; i < numEvents; ++i)
    {
        Event event;
        if (events->getEvent(i, event) == kResultOk)
        {
            switch (event.type)
            {
                case Event::kNoteOnEvent:
                {
                    audioEngine_->scheduleNoteOnEvent(
                        event.noteOn.pitch, event.noteOn.velocity,
                        static_cast<std::uint32_t>(
                            std::max(event.sampleOffset, 0)));
                    break;
                }

                case Event::kNoteOffEvent:
                {
                    audioEngine_->scheduleNoteOffEvent(
                        event.noteOff.pitch,
                        static_cast<std::uint32_t>(
                            std::max(event.sampleOffset, 0)));
                    break;
                }

                // We could handle other event types here (pitch bend, CC, etc.)
                default:
                    break;
            }
        }
    }
}

//------------------------------------------------------------------------
void KickSynthProcessor::processParameterChanges(IParameterChanges* changes)
{
    if (!changes)
        return;

    int32 numParamsChanged = changes->getParameterCount();
    for (int32 i = 0; i < numParamsChanged; ++i)
    {
        IParamValueQueue* paramQueue = changes->getParameterData(i);
        if (paramQueue)
        {
            int32 numPoints = paramQueue->getPointCount();

            const auto* mapping = findParameterMapping(paramQueue->getParameterId());
            if (!mapping)
                continue;

            for (int32 point = 0; point < numPoints; ++point)
            {
                ParamValue normalizedValue = 0.0;
                int32 sampleOffset = 0;
                if (paramQueue->getPoint(point, sampleOffset, normalizedValue) != kResultOk)
                    continue;

                const float value = static_cast<float>(
                    denormalizeParameterValue(*mapping, normalizedValue));
                audioEngine_->scheduleParameterEvent(
                    mapping->engineId, value,
                    static_cast<std::uint32_t>(std::max(sampleOffset, 0)));
            }
        }
    }
}

//------------------------------------------------------------------------
void KickSynthProcessor::publishProcessedState() noexcept
{
    const KickDrum::KickParams params = audioEngine_->getParams();
    processedStateEpoch_.fetch_add(1, std::memory_order_acq_rel);
    for (const auto& spec : KickDrum::kKickParameterSpecs)
    {
        stateParameterValues_[static_cast<std::size_t>(spec.id)].store(
            KickDrum::getKickParameter(params, spec.id),
            std::memory_order_relaxed);
    }
    processedStateEpoch_.fetch_add(1, std::memory_order_release);
    processedStateSerial_.store(audioEngine_->getAppliedStateRevision(),
                                std::memory_order_release);
}

//------------------------------------------------------------------------
KickDrum::KickParams KickSynthProcessor::readProcessedState() const noexcept
{
    KickDrum::KickParams params = KickDrum::kDefaultKickParams;
    for (;;)
    {
        const std::uint64_t before =
            processedStateEpoch_.load(std::memory_order_acquire);
        if ((before & 1u) != 0u)
            continue;

        for (const auto& spec : KickDrum::kKickParameterSpecs)
        {
            KickDrum::setKickParameter(
                params, spec.id,
                stateParameterValues_[static_cast<std::size_t>(spec.id)].load(
                    std::memory_order_relaxed));
        }

        const std::uint64_t after =
            processedStateEpoch_.load(std::memory_order_acquire);
        if (before == after && (after & 1u) == 0u)
            return KickDrum::sanitizeKickParams(params);
    }
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
