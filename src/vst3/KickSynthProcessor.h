#pragma once

#include "KickParams.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace KickDrum {
    class AudioEngine;
}

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Kick Drum Synthesizer VST3 Processor
 * 
 * This is the audio processing component of the VST3 plugin.
 * It handles:
 * - Audio buffer processing
 * - MIDI event handling
 * - Parameter automation
 * - State serialization/deserialization
 */
class KickSynthProcessor : public AudioEffect
{
public:
    KickSynthProcessor();
    ~KickSynthProcessor() override;

    // Create function used by factory
    static FUnknown* createInstance(void* /*context*/)
    {
        return (IAudioProcessor*)new KickSynthProcessor;
    }

    //--- AudioEffect overrides --------
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
    tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
    tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
    tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
    tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup) SMTG_OVERRIDE;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
    tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

protected:
    // Process MIDI events from the event list
    void processMIDIEvents(IEventList* events);
    
    // Process parameter changes
    void processParameterChanges(IParameterChanges* changes);

    /** Publish one coherent audio-thread parameter snapshot for getState(). */
    void publishProcessedState() noexcept;

    /** Read a coherent processed parameter snapshot without blocking audio. */
    KickDrum::KickParams readProcessedState() const noexcept;

private:
    std::unique_ptr<KickDrum::AudioEngine> audioEngine_;
    std::vector<float> interleavedBuffer_;
    std::array<std::atomic<float>,
               static_cast<std::size_t>(KickDrum::KickParameterId::Count)>
        stateParameterValues_;
    std::atomic<std::uint64_t> processedStateEpoch_ {0};
    std::atomic<std::uint64_t> requestedStateSerial_ {0};
    std::atomic<std::uint64_t> processedStateSerial_ {0};
    KickDrum::KickParams requestedState_ = KickDrum::kDefaultKickParams;
    mutable std::mutex stateMutex_;
    std::atomic<bool> auditionPending_ {false};
    float meterPeak_ = 0.0f;
    float previousOutputPeak_ = -1.0f;
    int64 clipHoldSamplesRemaining_ = 0;
    bool previousOutputClip_ = false;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
