#include "KickSynthProcessor.h"
#include "KickSynthCIDs.h"
#include "AudioEngine.h"
#include "ParameterEventQueue.h"
#include "ParameterManager.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <algorithm>
#include <cmath>
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
        return kResultOk;
    }

    // Get output buffer info
    int32 numChannels = data.outputs[0].numChannels;
    int32 numSamples = data.numSamples;
    float** outputBuffers = data.outputs[0].channelBuffers32;

    // Check for valid output
    if (outputBuffers == nullptr || numSamples <= 0)
    {
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
    return AudioEffect::notify(message);
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    // Read state from stream
    IBStreamer streamer(state, kLittleEndian);
    
    // Read version
    uint32 version = 0;
    if (!streamer.readInt32u(version))
        return kResultFalse;

    // Read number of parameters
    uint32 numParams = 0;
    if (!streamer.readInt32u(numParams))
        return kResultFalse;

    // Read each parameter
    KickDrum::ParameterManager* paramManager = audioEngine_->getParameterManager();
    if (!paramManager)
        return kResultFalse;

    for (uint32 i = 0; i < numParams; ++i)
    {
        // Read parameter ID length
        uint32 idLength = 0;
        if (!streamer.readInt32u(idLength))
            return kResultFalse;

        // Read parameter ID
        std::string paramId;
        paramId.resize(idLength);
        if (streamer.readRaw(paramId.data(), idLength) != idLength)
            return kResultFalse;

        // Read parameter value
        double value = 0.0;
        if (!streamer.readDouble(value))
            return kResultFalse;

        if (paramManager->hasParameter(paramId))
            audioEngine_->setParameter(paramId, static_cast<float>(value));
    }

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthProcessor::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    // Write state to stream
    IBStreamer streamer(state, kLittleEndian);

    // Write version
    uint32 version = 1;
    if (!streamer.writeInt32u(version))
        return kResultFalse;

    // Get all parameters
    KickDrum::ParameterManager* paramManager = audioEngine_->getParameterManager();
    if (!paramManager)
        return kResultFalse;

    std::vector<std::string> paramIds = paramManager->getParameterIds();
    
    // Write number of parameters
    if (!streamer.writeInt32u(static_cast<uint32>(paramIds.size())))
        return kResultFalse;

    // Write each parameter
    for (const auto& paramId : paramIds)
    {
        // Write parameter ID length
        if (!streamer.writeInt32u(static_cast<uint32>(paramId.length())))
            return kResultFalse;
        
        // Write parameter ID
        if (streamer.writeRaw(paramId.data(), static_cast<TSize>(paramId.length())) !=
            static_cast<TSize>(paramId.length()))
            return kResultFalse;

        // Write parameter value
        float value = paramManager->getParameterValue(paramId);
        if (!streamer.writeDouble(static_cast<double>(value)))
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
                    // Convert MIDI note to our format
                    // VST3 velocity is 0.0 to 1.0
                    audioEngine_->noteOn(event.noteOn.pitch, event.noteOn.velocity);
                    break;
                }

                case Event::kNoteOffEvent:
                {
                    audioEngine_->noteOff(event.noteOff.pitch);
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

    auto* paramManager = audioEngine_->getParameterManager();
    auto* eventQueue = audioEngine_->getParameterEventQueue();
    if (!paramManager || !eventQueue)
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
                const auto* spec = KickDrum::findKickParameterSpec(mapping->engineId);
                if (!spec)
                    continue;
                const std::string parameterId(spec->key);

                // Keep serialized state current and apply every automation point
                // at the host-provided position in the upcoming audio block.
                paramManager->setParameterValue(parameterId, value);
                eventQueue->addEvent(parameterId, value,
                                     static_cast<uint32_t>(std::max(sampleOffset, 0)));
            }
        }
    }
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
