#include "AudioEngine.h"

#include "../parameters/KickParams.h"
#include "../parameters/ParameterEventQueue.h"
#include "../parameters/ParameterManager.h"
#include "../utils/DSPUtils.h"
#include "../voice/VoiceAllocator.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace KickDrum {
namespace {

std::shared_ptr<const SampleLayerData> sanitizeSampleLayer(
    const std::shared_ptr<const SampleLayerData>& sampleLayer) {
    constexpr std::size_t maximumSampleCount = 10000000;
    if (!sampleLayer || sampleLayer->samples.empty() ||
        sampleLayer->samples.size() > maximumSampleCount) {
        return {};
    }

    auto sanitized = std::make_shared<SampleLayerData>();
    sanitized->sourceSampleRate =
        std::isfinite(sampleLayer->sourceSampleRate) &&
                sampleLayer->sourceSampleRate > 0.0f
            ? std::clamp(sampleLayer->sourceSampleRate, 1000.0f, 768000.0f)
            : 48000.0f;
    sanitized->samples.resize(sampleLayer->samples.size());
    std::transform(sampleLayer->samples.begin(), sampleLayer->samples.end(),
                   sanitized->samples.begin(), [](float sample) {
                       return std::isfinite(sample)
                                  ? std::clamp(sample, -1.0f, 1.0f)
                                  : 0.0f;
                   });
    return sanitized;
}

} // namespace

class AudioEngine::Impl {
public:
    struct InstalledSampleLayer {
        std::uint64_t revision = 0;
        std::shared_ptr<const SampleLayerData> data;
    };

    struct RealtimeParameterEvent {
        KickParameterId id = KickParameterId::Pitch0Hz;
        float value = 0.0f;
        std::uint32_t sampleOffset = 0;
        std::uint32_t order = 0;
    };

    struct RealtimeNoteEvent {
        enum class Type : std::uint8_t { On, Off };
        Type type = Type::On;
        int note = 0;
        float velocity = 0.0f;
        std::uint32_t sampleOffset = 0;
        std::uint32_t order = 0;
    };

    float sampleRate = 48000.0f;
    KickParams params = kDefaultKickParams;
    std::array<std::string,
               static_cast<std::size_t>(KickParameterId::Count)> parameterKeys;
    std::unique_ptr<VoiceAllocator> voiceAllocator;
    std::unique_ptr<ParameterManager> parameterManager;
    std::unique_ptr<ParameterEventQueue> parameterEventQueue;
    std::atomic<bool> enableSoftClipping {true};
    std::atomic<bool> enableNaNDetection {true};
    std::vector<float> monoBuffer;
    std::vector<ParameterEvent> currentEvents;
    std::atomic<std::uint32_t> pendingNoteOn {0};
    std::atomic<bool> requestedAuditionLoopEnabled {false};
    std::atomic<float> requestedAuditionLoopBpm {120.0f};
    std::atomic<std::uint32_t> auditionLoopRevision {0};
    std::atomic<float> outputPeak {0.0f};
    std::atomic<bool> outputClip {false};
    std::atomic<const InstalledSampleLayer*> requestedSampleLayer {nullptr};
    std::atomic<const InstalledSampleLayer*> sampleLayerHazard {nullptr};
    std::atomic<std::uint64_t> observedSampleLayerRevision {0};
    std::array<std::atomic<std::uint64_t>, VoiceAllocator::kMaxVoices>
        protectedSampleLayerRevisions;
    std::vector<std::shared_ptr<const InstalledSampleLayer>> installedSampleLayers;
    std::mutex installedSampleLayersMutex;
    std::mutex controlStateMutex;
    std::optional<KickParams> pendingStateParams;
    std::uint64_t pendingStateRevision = 0;
    std::uint64_t nextStateRevision = 0;
    std::atomic<std::uint64_t> appliedStateRevision {0};
    std::uint64_t nextSampleLayerRevision = 0;
    std::uint32_t appliedAuditionLoopRevision = 0;
    const InstalledSampleLayer* appliedSampleLayer = nullptr;
    std::uint64_t samplesUntilAuditionHit = 0;
    std::uint64_t auditionIntervalSamples = 24000;
    bool auditionLoopStateInitialized = false;
    bool auditionLoopEnabled = false;
    bool sampleLayerStateInitialized = false;
    float eqLowState = 0.0f;
    float eqWideState = 0.0f;
    float eqLowCoefficient = 0.0f;
    float eqWideCoefficient = 0.0f;
    float eqLowGain = 1.0f;
    float eqMidGain = 1.0f;
    float eqHighGain = 1.0f;
    float saturationAmount = 0.0f;
    float limiterCeilingLinear = 1.0f;
    float eqLowGainTarget = 1.0f;
    float eqMidGainTarget = 1.0f;
    float eqHighGainTarget = 1.0f;
    float saturationAmountTarget = 0.0f;
    float limiterCeilingTarget = 1.0f;
    float outputSmoothingCoefficient = 1.0f;
    std::array<RealtimeParameterEvent,
               AudioEngine::kMaxRealtimeParameterEvents> realtimeParameterEvents;
    std::array<RealtimeNoteEvent,
               AudioEngine::kMaxRealtimeNoteEvents> realtimeNoteEvents;
    std::array<float,
               static_cast<std::size_t>(KickParameterId::Count)>
        finalParameterValues {};
    std::array<std::uint32_t,
               static_cast<std::size_t>(KickParameterId::Count)>
        finalParameterOffsets {};
    std::array<std::uint32_t,
               static_cast<std::size_t>(KickParameterId::Count)>
        finalParameterOrders {};
    std::array<bool,
               static_cast<std::size_t>(KickParameterId::Count)>
        finalParameterValid {};
    std::size_t realtimeParameterEventCount = 0;
    std::size_t realtimeNoteEventCount = 0;
    std::uint32_t realtimeParameterOrder = 0;
    std::uint32_t realtimeNoteOrder = 0;

    Impl() {
        for (auto& revision : protectedSampleLayerRevisions) {
            revision.store(0, std::memory_order_relaxed);
        }
        for (const auto& spec : kKickParameterSpecs) {
            parameterKeys[static_cast<std::size_t>(spec.id)] =
                std::string(spec.key);
        }
    }

    bool commitParams(const KickParams& candidate) {
        const KickParams previousParams = params;
        params = sanitizeKickParams(candidate);

        bool anyChanged = false;
        bool voiceParamsChanged = false;
        bool outputStageChanged = false;
        for (const auto& spec : kKickParameterSpecs) {
            if (getKickParameter(params, spec.id) ==
                getKickParameter(previousParams, spec.id)) {
                continue;
            }
            anyChanged = true;
            const bool isOutputStage =
                spec.id == KickParameterId::EqLowDb ||
                spec.id == KickParameterId::EqMidDb ||
                spec.id == KickParameterId::EqHighDb ||
                spec.id == KickParameterId::Saturation ||
                spec.id == KickParameterId::LimiterCeilingDb;
            outputStageChanged = outputStageChanged || isOutputStage;
            voiceParamsChanged = voiceParamsChanged || !isOutputStage;
        }
        if (!anyChanged) {
            return true;
        }

        if (voiceParamsChanged) {
            voiceAllocator->setParams(params);
        } else {
            voiceAllocator->setOutputStageParams(params.outputStage);
        }
        if (outputStageChanged) {
            const bool canApplyOutputImmediately =
                voiceAllocator->getNumActiveVoices() == 0;
            updateOutputConfiguration(canApplyOutputImmediately);
        }

        if (parameterManager) {
            for (const auto& spec : kKickParameterSpecs) {
                const float current = getKickParameter(params, spec.id);
                if (current != getKickParameter(previousParams, spec.id)) {
                    parameterManager->setParameterValue(
                        parameterKeys[static_cast<std::size_t>(spec.id)], current);
                }
            }
        }
        return true;
    }

    bool applyParameter(const std::string& parameterId, float value) {
        const auto* spec = findKickParameterSpec(parameterId);
        if (!spec) {
            return false;
        }

        return applyParameter(spec->id, value);
    }

    bool applyParameter(KickParameterId id, float value) {
        if (!findKickParameterSpec(id)) {
            return false;
        }

        KickParams candidate = params;
        setKickParameter(candidate, id, value);
        return commitParams(candidate);
    }

    void sortRealtimeEvents() noexcept {
        // Shell sort has deterministic bounded storage and performs no hidden
        // allocation. Sequence numbers preserve arrival order at equal sample
        // offsets even though host queues are grouped by parameter.
        const auto sort = [](auto& events, std::size_t count) {
            for (std::size_t gap = count / 2; gap > 0; gap /= 2) {
                for (std::size_t index = gap; index < count; ++index) {
                    const auto event = events[index];
                    std::size_t cursor = index;
                    const auto comesBefore = [&event](const auto& other) {
                        return event.sampleOffset < other.sampleOffset ||
                               (event.sampleOffset == other.sampleOffset &&
                                event.order < other.order);
                    };
                    while (cursor >= gap && comesBefore(events[cursor - gap])) {
                        events[cursor] = events[cursor - gap];
                        cursor -= gap;
                    }
                    events[cursor] = event;
                }
            }
        };
        sort(realtimeParameterEvents, realtimeParameterEventCount);
        sort(realtimeNoteEvents, realtimeNoteEventCount);
    }

    void clearRealtimeEvents() noexcept {
        realtimeParameterEventCount = 0;
        realtimeNoteEventCount = 0;
        realtimeParameterOrder = 0;
        realtimeNoteOrder = 0;
        finalParameterValid.fill(false);
    }

    void applyNoteEvent(const RealtimeNoteEvent& event) {
        if (event.type == RealtimeNoteEvent::Type::On) {
            voiceAllocator->allocateVoice(event.note, event.velocity);
        } else {
            voiceAllocator->releaseVoice(event.note);
        }
    }

    static float dbToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }

    void updateOutputConfiguration(bool immediate) {
        eqLowGainTarget = dbToLinear(params.outputStage.eqLowDb);
        eqMidGainTarget = dbToLinear(params.outputStage.eqMidDb);
        eqHighGainTarget = dbToLinear(params.outputStage.eqHighDb);
        saturationAmountTarget = params.outputStage.saturation;
        limiterCeilingTarget = dbToLinear(params.outputStage.limiterCeilingDb);
        if (immediate) {
            eqLowGain = eqLowGainTarget;
            eqMidGain = eqMidGainTarget;
            eqHighGain = eqHighGainTarget;
            saturationAmount = saturationAmountTarget;
            limiterCeilingLinear = limiterCeilingTarget;
        }
    }

    void advanceOutputConfiguration() {
        const auto smooth = [this](float current, float target) {
            const float next = current +
                               outputSmoothingCoefficient * (target - current);
            return std::abs(next - target) < 1.0e-6f ? target : next;
        };
        eqLowGain = smooth(eqLowGain, eqLowGainTarget);
        eqMidGain = smooth(eqMidGain, eqMidGainTarget);
        eqHighGain = smooth(eqHighGain, eqHighGainTarget);
        saturationAmount = smooth(saturationAmount, saturationAmountTarget);
        limiterCeilingLinear = smooth(limiterCeilingLinear, limiterCeilingTarget);
    }

    void resetOutputStage() {
        eqLowState = 0.0f;
        eqWideState = 0.0f;
        constexpr float twoPi = 6.28318530717958647692f;
        eqLowCoefficient = 1.0f - std::exp(-twoPi * 120.0f / sampleRate);
        eqWideCoefficient = 1.0f - std::exp(-twoPi * 3000.0f / sampleRate);
        outputSmoothingCoefficient =
            1.0f - std::exp(-1.0f / (sampleRate * 0.005f));
        updateOutputConfiguration(true);
    }

    float processOutputSample(float input, bool& limiterOver) {
        advanceOutputConfiguration();
        if (!std::isfinite(input)) {
            eqLowState = 0.0f;
            eqWideState = 0.0f;
            return 0.0f;
        }

        eqLowState += eqLowCoefficient * (input - eqLowState);
        eqWideState += eqWideCoefficient * (input - eqWideState);
        if (std::abs(eqLowState) < 1.0e-20f) {
            eqLowState = 0.0f;
        }
        if (std::abs(eqWideState) < 1.0e-20f) {
            eqWideState = 0.0f;
        }

        float processed = input;
        if (eqLowGain != 1.0f || eqMidGain != 1.0f || eqHighGain != 1.0f) {
            const float low = eqLowState;
            const float mid = eqWideState - eqLowState;
            const float high = input - eqWideState;
            processed = low * eqLowGain + mid * eqMidGain + high * eqHighGain;
        }

        if (saturationAmount > 0.0f) {
            const float drive = 1.0f + saturationAmount * 5.0f;
            const float shaped = std::tanh(processed * drive) / std::tanh(drive);
            processed += (shaped - processed) * saturationAmount;
        }
        if (!std::isfinite(processed)) {
            return 0.0f;
        }

        limiterOver = limiterOver || std::abs(processed) >= limiterCeilingLinear;
        if (enableSoftClipping.load(std::memory_order_relaxed)) {
            processed = limiterCeilingLinear *
                        DSPUtils::softClip(processed / limiterCeilingLinear);
        }
        return std::isfinite(processed) ? processed : 0.0f;
    }

    void processOutputRange(float* buffer, std::size_t numSamples,
                            bool& limiterOver) {
        for (std::size_t sample = 0; sample < numSamples; ++sample) {
            buffer[sample] = processOutputSample(buffer[sample], limiterOver);
        }
    }

    std::uint64_t auditionIntervalForBpm(float bpm) const {
        const double interval = static_cast<double>(sampleRate) * 60.0 /
                                static_cast<double>(bpm);
        return std::max<std::uint64_t>(
            1, static_cast<std::uint64_t>(std::llround(interval)));
    }

    void synchronizeAuditionLoop() {
        std::uint32_t revision = auditionLoopRevision.load(std::memory_order_acquire);
        if (auditionLoopStateInitialized &&
            revision == appliedAuditionLoopRevision) {
            return;
        }

        bool enabled = false;
        float bpm = 120.0f;
        std::uint32_t stableRevision = revision;
        do {
            revision = stableRevision;
            enabled = requestedAuditionLoopEnabled.load(std::memory_order_relaxed);
            bpm = requestedAuditionLoopBpm.load(std::memory_order_relaxed);
            stableRevision = auditionLoopRevision.load(std::memory_order_acquire);
        } while (stableRevision != revision);

        const auto newInterval = auditionIntervalForBpm(bpm);
        if (enabled && !auditionLoopEnabled) {
            samplesUntilAuditionHit = 0;
        } else if (enabled && auditionLoopEnabled &&
                   newInterval != auditionIntervalSamples &&
                   samplesUntilAuditionHit > 0) {
            const double phaseRemaining =
                static_cast<double>(samplesUntilAuditionHit) /
                static_cast<double>(auditionIntervalSamples);
            samplesUntilAuditionHit = std::max<std::uint64_t>(
                1, static_cast<std::uint64_t>(
                       std::llround(phaseRemaining *
                                    static_cast<double>(newInterval))));
        }

        auditionLoopEnabled = enabled;
        auditionIntervalSamples = newInterval;
        if (!enabled) {
            samplesUntilAuditionHit = 0;
        }
        appliedAuditionLoopRevision = stableRevision;
        auditionLoopStateInitialized = true;
    }

    void publishRequestedSampleLayer(
        std::shared_ptr<const SampleLayerData> sampleLayer) {
        auto installed = std::make_shared<InstalledSampleLayer>();
        installed->data = std::move(sampleLayer);
        std::lock_guard<std::mutex> lock(installedSampleLayersMutex);
        installed->revision = ++nextSampleLayerRevision;
        const auto* requested = installed.get();
        installedSampleLayers.push_back(std::move(installed));
        requestedSampleLayer.store(requested, std::memory_order_seq_cst);
        pruneRetiredSampleLayers();
    }

    void synchronizeSampleLayer() {
        // Publish a hazard before dereferencing the control-owned registry
        // entry, then validate that it is still the requested entry. This lets
        // the control thread reclaim skipped revisions even before audio has
        // processed once, without racing a callback that just loaded one.
        const InstalledSampleLayer* installed = nullptr;
        do {
            installed = requestedSampleLayer.load(std::memory_order_seq_cst);
            sampleLayerHazard.store(installed, std::memory_order_seq_cst);
        } while (installed !=
                 requestedSampleLayer.load(std::memory_order_seq_cst));
        if (sampleLayerStateInitialized && installed == appliedSampleLayer) {
            sampleLayerHazard.store(nullptr, std::memory_order_seq_cst);
            return;
        }
        const auto* sampleLayer =
            installed && installed->data ? installed->data.get() : nullptr;
        voiceAllocator->setSampleLayer(
            sampleLayer, installed ? installed->revision : 0);
        appliedSampleLayer = installed;
        sampleLayerStateInitialized = true;
        publishSampleLayerUsage();
        sampleLayerHazard.store(nullptr, std::memory_order_seq_cst);
    }

    bool synchronizeControlState() {
        std::unique_lock<std::mutex> lock(controlStateMutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false;
        }
        std::uint64_t completedStateRevision = 0;
        if (pendingStateParams) {
            commitParams(*pendingStateParams);
            pendingStateParams.reset();
            completedStateRevision = pendingStateRevision;
        }
        synchronizeSampleLayer();
        if (completedStateRevision != 0) {
            // A revision represents the complete parameter + sample pair.
            appliedStateRevision.store(completedStateRevision,
                                       std::memory_order_release);
        }
        return true;
    }

    void publishSampleLayerUsage() {
        const auto activeRevisions =
            voiceAllocator->activeSampleLayerRevisions();
        for (std::size_t index = 0; index < activeRevisions.size(); ++index) {
            protectedSampleLayerRevisions[index].store(
                activeRevisions[index], std::memory_order_release);
        }
        // Publishing the observed revision last makes the exact active set
        // visible before control-side reclamation advances to that revision.
        observedSampleLayerRevision.store(
            appliedSampleLayer ? appliedSampleLayer->revision : 0,
            std::memory_order_release);
    }

    void pruneRetiredSampleLayers() {
        // Read the transient hazard before the stable observed revision. If
        // this sees the audio thread's clear, the following acquire must also
        // see the applied-revision publication that preceded that clear.
        const auto* hazard = sampleLayerHazard.load(std::memory_order_seq_cst);
        const auto observedRevision =
            observedSampleLayerRevision.load(std::memory_order_acquire);
        const auto* requested =
            requestedSampleLayer.load(std::memory_order_seq_cst);
        std::array<std::uint64_t, VoiceAllocator::kMaxVoices>
            protectedRevisions {};
        for (std::size_t index = 0; index < protectedRevisions.size(); ++index) {
            protectedRevisions[index] =
                protectedSampleLayerRevisions[index].load(
                    std::memory_order_acquire);
        }
        installedSampleLayers.erase(
            std::remove_if(
                installedSampleLayers.begin(), installedSampleLayers.end(),
                [observedRevision, requested, hazard,
                 &protectedRevisions](const auto& installed) {
                    // The requested, currently inspected, and most recently
                    // applied entries are control/audio handoff hazards. Exact
                    // active revisions protect older buffers used by live hits.
                    if (installed.get() == requested ||
                        installed.get() == hazard ||
                        (observedRevision != 0 &&
                         installed->revision == observedRevision)) {
                        return false;
                    }
                    return std::find(protectedRevisions.begin(),
                                     protectedRevisions.end(),
                                     installed->revision) ==
                           protectedRevisions.end();
                }),
            installedSampleLayers.end());
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
    pImpl->sampleRate = std::isfinite(sampleRate) && sampleRate > 0.0f
                            ? sampleRate
                            : 48000.0f;
    pImpl->voiceAllocator->initialize(pImpl->sampleRate);
    pImpl->voiceAllocator->setParams(pImpl->params);
    for (const auto& spec : kKickParameterSpecs) {
        pImpl->parameterManager->setParameterValue(
            std::string(spec.key), getKickParameter(pImpl->params, spec.id));
    }
    // Producer-side control/state events intentionally survive sample-rate
    // reinitialization. VST hosts commonly restore component state before
    // setupProcessing()/setActive(), and those lifecycle calls must not erase
    // the pending block-boundary snapshot.
    pImpl->clearRealtimeEvents();
    pImpl->pendingNoteOn.store(0, std::memory_order_release);
    pImpl->auditionLoopEnabled = false;
    pImpl->auditionLoopStateInitialized = false;
    pImpl->sampleLayerStateInitialized = false;
    pImpl->samplesUntilAuditionHit = 0;
    pImpl->outputPeak.store(0.0f, std::memory_order_release);
    pImpl->outputClip.store(false, std::memory_order_release);
    pImpl->resetOutputStage();
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

    // If a control thread is currently publishing a complete restore or a new
    // sample, keep rendering the last complete snapshot. The next callback
    // adopts the update; the realtime thread never blocks or drops a buffer.
    (void)pImpl->synchronizeControlState();

    if (pImpl->monoBuffer.size() < numSamples) {
        pImpl->monoBuffer.resize(numSamples);
    }
    std::fill_n(pImpl->monoBuffer.data(), numSamples, 0.0f);

    pImpl->parameterEventQueue->getEventsForBuffer(pImpl->currentEvents);
    pImpl->sortRealtimeEvents();
    std::size_t uiParameterIndex = 0;
    std::size_t realtimeParameterIndex = 0;
    std::size_t realtimeNoteIndex = 0;

    // Preserve the raw final values independently of the bounded automation
    // timeline. Intermediate trajectory states may need sanitizing for safe
    // rendering, but that must not destroy a companion point's final value.
    KickParams finalBlockParams = pImpl->params;
    bool hasFinalParameterState = false;
    std::array<std::uint32_t,
               static_cast<std::size_t>(KickParameterId::Count)>
        uiFinalParameterOffsets {};
    std::array<bool,
               static_cast<std::size_t>(KickParameterId::Count)>
        uiFinalParameterValid {};
    for (const auto& event : pImpl->currentEvents) {
        if (const auto* spec = findKickParameterSpec(event.parameterId)) {
            setKickParameter(finalBlockParams, spec->id, event.value);
            const auto index = static_cast<std::size_t>(spec->id);
            uiFinalParameterOffsets[index] = event.sampleOffset;
            uiFinalParameterValid[index] = true;
            hasFinalParameterState = true;
        }
    }
    for (std::size_t index = 0;
        index < pImpl->finalParameterValid.size(); ++index) {
        if (pImpl->finalParameterValid[index]) {
            const auto hostOffset = std::min<std::size_t>(
                pImpl->finalParameterOffsets[index], numSamples);
            const auto uiOffset = std::min<std::size_t>(
                uiFinalParameterOffsets[index], numSamples);
            // UI producer events are applied before host events at an equal
            // sample boundary; otherwise the later timeline point wins.
            if (uiFinalParameterValid[index] && hostOffset < uiOffset) {
                continue;
            }
            setKickParameter(finalBlockParams,
                             static_cast<KickParameterId>(index),
                             pImpl->finalParameterValues[index]);
            hasFinalParameterState = true;
        }
    }

    const std::uint32_t packedNote =
        pImpl->pendingNoteOn.exchange(0, std::memory_order_acquire);
    bool pendingNoteAtBlockStart = packedNote != 0;

    pImpl->synchronizeAuditionLoop();

    const auto effectiveOffset = [numSamples](std::uint32_t offset) {
        return std::min<std::size_t>(offset, numSamples);
    };
    const auto applyParametersThrough = [&](std::size_t offset) {
        KickParams candidate = pImpl->params;
        bool hasParameterChange = false;
        while (uiParameterIndex < pImpl->currentEvents.size() &&
               effectiveOffset(
                   pImpl->currentEvents[uiParameterIndex].sampleOffset) <= offset) {
            const auto& event = pImpl->currentEvents[uiParameterIndex++];
            if (const auto* spec = findKickParameterSpec(event.parameterId)) {
                setKickParameter(candidate, spec->id, event.value);
                hasParameterChange = true;
            }
        }
        while (realtimeParameterIndex < pImpl->realtimeParameterEventCount &&
               effectiveOffset(pImpl->realtimeParameterEvents[
                                   realtimeParameterIndex].sampleOffset) <= offset) {
            const auto& event =
                pImpl->realtimeParameterEvents[realtimeParameterIndex++];
            setKickParameter(candidate, event.id, event.value);
            hasParameterChange = true;
        }
        if (hasParameterChange) {
            // Host queues are grouped by parameter. Commit one coherent
            // trajectory at each boundary so simultaneous point moves do not
            // depend on queue iteration order.
            pImpl->commitParams(candidate);
        }
    };
    const auto applyNotesThrough = [&](std::size_t offset) {
        while (realtimeNoteIndex < pImpl->realtimeNoteEventCount &&
               effectiveOffset(
                   pImpl->realtimeNoteEvents[realtimeNoteIndex].sampleOffset) <=
                   offset) {
            pImpl->applyNoteEvent(
                pImpl->realtimeNoteEvents[realtimeNoteIndex++]);
        }
    };

    bool limiterOver = false;
    std::size_t currentSample = 0;
    while (currentSample < numSamples) {
        // Parameters from both producer paths always precede note events at a
        // shared sample offset, so a note snapshots the exact host state for
        // that instant.
        applyParametersThrough(currentSample);
        applyNotesThrough(currentSample);

        if (pendingNoteAtBlockStart) {
            const int note = static_cast<int>((packedNote & 0xffu) - 1u);
            const float velocity =
                static_cast<float>((packedNote >> 8u) & 0xffffu) / 65535.0f;
            pImpl->voiceAllocator->allocateVoice(note, velocity);
            pendingNoteAtBlockStart = false;
        }

        if (pImpl->auditionLoopEnabled &&
            pImpl->samplesUntilAuditionHit == 0) {
            pImpl->voiceAllocator->allocateVoice(36, 1.0f);
            pImpl->samplesUntilAuditionHit = pImpl->auditionIntervalSamples;
        }

        std::size_t nextEventSample = numSamples;
        if (uiParameterIndex < pImpl->currentEvents.size()) {
            nextEventSample = std::min(
                nextEventSample,
                effectiveOffset(
                    pImpl->currentEvents[uiParameterIndex].sampleOffset));
        }
        if (realtimeParameterIndex < pImpl->realtimeParameterEventCount) {
            nextEventSample = std::min(
                nextEventSample,
                effectiveOffset(pImpl->realtimeParameterEvents[
                                    realtimeParameterIndex].sampleOffset));
        }
        if (realtimeNoteIndex < pImpl->realtimeNoteEventCount) {
            nextEventSample = std::min(
                nextEventSample,
                effectiveOffset(
                    pImpl->realtimeNoteEvents[realtimeNoteIndex].sampleOffset));
        }
        std::size_t samplesToRender = nextEventSample - currentSample;
        if (pImpl->auditionLoopEnabled) {
            samplesToRender = std::min<std::size_t>(
                samplesToRender,
                static_cast<std::size_t>(std::min<std::uint64_t>(
                    pImpl->samplesUntilAuditionHit,
                    static_cast<std::uint64_t>(numSamples - currentSample))));
        }
        if (samplesToRender > 0) {
            pImpl->voiceAllocator->renderBuffer(
                pImpl->monoBuffer.data() + currentSample,
                static_cast<int>(samplesToRender));
            pImpl->processOutputRange(
                pImpl->monoBuffer.data() + currentSample,
                samplesToRender, limiterOver);
            currentSample += samplesToRender;
            if (pImpl->auditionLoopEnabled) {
                pImpl->samplesUntilAuditionHit -= samplesToRender;
            }
        }
    }

    // Events at or beyond the block end establish state for the next block.
    // Keep the same parameter-before-note rule at that boundary.
    applyParametersThrough(numSamples);
    // Recommit the coherent raw final snapshot once. This also preserves the
    // latest state if a pathological block exceeded fixed timeline storage.
    if (hasFinalParameterState) {
        pImpl->commitParams(finalBlockParams);
    }
    applyNotesThrough(numSamples);
    pImpl->clearRealtimeEvents();

    if (pImpl->enableNaNDetection.load(std::memory_order_relaxed)) {
        if (!DSPUtils::isBufferValid(pImpl->monoBuffer.data(), numSamples)) {
            DSPUtils::sanitizeBuffer(pImpl->monoBuffer.data(), numSamples);
            pImpl->voiceAllocator->releaseAll();
        }
    }
    pImpl->publishSampleLayerUsage();

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
    if (limiterOver) {
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
    if (!pImpl->synchronizeControlState()) {
        enqueueNoteOn(note, velocity);
        return;
    }
    pImpl->voiceAllocator->allocateVoice(note, velocity);
    pImpl->publishSampleLayerUsage();
}

void AudioEngine::enqueueNoteOn(int note, float velocity) {
    const std::uint32_t encodedNote =
        static_cast<std::uint32_t>(std::clamp(note, 0, 127) + 1);
    const float safeVelocity = std::isfinite(velocity)
                                   ? std::clamp(velocity, 0.0f, 1.0f)
                                   : 0.0f;
    const std::uint32_t encodedVelocity = static_cast<std::uint32_t>(
        std::lround(safeVelocity * 65535.0f));
    pImpl->pendingNoteOn.store(encodedNote | (encodedVelocity << 8u),
                               std::memory_order_release);
}

bool AudioEngine::scheduleParameterEvent(
    KickParameterId id, float value, std::uint32_t sampleOffset) noexcept {
    const auto parameterIndex = static_cast<std::size_t>(id);
    if (parameterIndex >= static_cast<std::size_t>(KickParameterId::Count)) {
        return false;
    }
    const std::uint32_t order = pImpl->realtimeParameterOrder++;
    if (!pImpl->finalParameterValid[parameterIndex] ||
        sampleOffset > pImpl->finalParameterOffsets[parameterIndex] ||
        (sampleOffset == pImpl->finalParameterOffsets[parameterIndex] &&
         order > pImpl->finalParameterOrders[parameterIndex])) {
        pImpl->finalParameterValues[parameterIndex] = value;
        pImpl->finalParameterOffsets[parameterIndex] = sampleOffset;
        pImpl->finalParameterOrders[parameterIndex] = order;
        pImpl->finalParameterValid[parameterIndex] = true;
    }
    if (pImpl->realtimeParameterEventCount >= kMaxRealtimeParameterEvents) {
        return false;
    }

    auto& event = pImpl->realtimeParameterEvents[
        pImpl->realtimeParameterEventCount++];
    event.id = id;
    event.value = value;
    event.sampleOffset = sampleOffset;
    event.order = order;
    return true;
}

bool AudioEngine::scheduleNoteOnEvent(
    int note, float velocity, std::uint32_t sampleOffset) noexcept {
    if (pImpl->realtimeNoteEventCount >= kMaxRealtimeNoteEvents) {
        return false;
    }

    auto& event = pImpl->realtimeNoteEvents[pImpl->realtimeNoteEventCount++];
    event.type = Impl::RealtimeNoteEvent::Type::On;
    event.note = std::clamp(note, 0, 127);
    event.velocity = std::isfinite(velocity)
                         ? std::clamp(velocity, 0.0f, 1.0f)
                         : 0.0f;
    event.sampleOffset = sampleOffset;
    event.order = pImpl->realtimeNoteOrder++;
    return true;
}

bool AudioEngine::scheduleNoteOffEvent(
    int note, std::uint32_t sampleOffset) noexcept {
    if (pImpl->realtimeNoteEventCount >= kMaxRealtimeNoteEvents) {
        return false;
    }

    auto& event = pImpl->realtimeNoteEvents[pImpl->realtimeNoteEventCount++];
    event.type = Impl::RealtimeNoteEvent::Type::Off;
    event.note = std::clamp(note, 0, 127);
    event.velocity = 0.0f;
    event.sampleOffset = sampleOffset;
    event.order = pImpl->realtimeNoteOrder++;
    return true;
}

void AudioEngine::clearScheduledEvents() noexcept {
    pImpl->clearRealtimeEvents();
}

void AudioEngine::flushScheduledEvents() {
    if (!pImpl->synchronizeControlState()) {
        return;
    }
    pImpl->parameterEventQueue->getEventsForBuffer(pImpl->currentEvents);
    pImpl->sortRealtimeEvents();
    // With no rendered samples every event lands on the same block boundary.
    // Apply all parameters first so notes snapshot the final boundary state.
    KickParams candidate = pImpl->params;
    bool hasParameterChange = false;
    for (const auto& event : pImpl->currentEvents) {
        if (const auto* spec = findKickParameterSpec(event.parameterId)) {
            setKickParameter(candidate, spec->id, event.value);
            hasParameterChange = true;
        }
    }
    for (std::size_t index = 0;
         index < pImpl->finalParameterValid.size(); ++index) {
        if (pImpl->finalParameterValid[index]) {
            setKickParameter(candidate, static_cast<KickParameterId>(index),
                             pImpl->finalParameterValues[index]);
            hasParameterChange = true;
        }
    }
    if (hasParameterChange) {
        pImpl->commitParams(candidate);
    }
    for (std::size_t index = 0; index < pImpl->realtimeNoteEventCount; ++index) {
        pImpl->applyNoteEvent(pImpl->realtimeNoteEvents[index]);
    }
    pImpl->publishSampleLayerUsage();
    pImpl->clearRealtimeEvents();
}

void AudioEngine::setAuditionLoop(bool enabled, float bpm) {
    const float safeBpm = std::isfinite(bpm) ? bpm : 120.0f;
    pImpl->requestedAuditionLoopBpm.store(
        std::clamp(safeBpm, 40.0f, 240.0f), std::memory_order_relaxed);
    pImpl->requestedAuditionLoopEnabled.store(enabled, std::memory_order_relaxed);
    pImpl->auditionLoopRevision.fetch_add(1, std::memory_order_release);
}

void AudioEngine::setSampleLayer(
    std::shared_ptr<const SampleLayerData> sampleLayer) {
    auto sanitized = sanitizeSampleLayer(sampleLayer);
    std::lock_guard<std::mutex> lock(pImpl->controlStateMutex);
    pImpl->publishRequestedSampleLayer(std::move(sanitized));
}

std::uint64_t AudioEngine::setStateSnapshot(
    const KickParams& params,
    std::shared_ptr<const SampleLayerData> sampleLayer) {
    const KickParams sanitizedParams = sanitizeKickParams(params);
    auto sanitizedSample = sanitizeSampleLayer(sampleLayer);
    std::lock_guard<std::mutex> lock(pImpl->controlStateMutex);
    pImpl->publishRequestedSampleLayer(std::move(sanitizedSample));
    pImpl->pendingStateParams = sanitizedParams;
    pImpl->pendingStateRevision = ++pImpl->nextStateRevision;
    return pImpl->pendingStateRevision;
}

std::uint64_t AudioEngine::getAppliedStateRevision() const noexcept {
    return pImpl->appliedStateRevision.load(std::memory_order_acquire);
}

std::shared_ptr<const SampleLayerData> AudioEngine::getSampleLayer() const {
    std::lock_guard<std::mutex> lock(pImpl->installedSampleLayersMutex);
    pImpl->pruneRetiredSampleLayers();
    const auto* requested =
        pImpl->requestedSampleLayer.load(std::memory_order_seq_cst);
    if (!requested) {
        return {};
    }
    const auto found = std::find_if(
        pImpl->installedSampleLayers.begin(),
        pImpl->installedSampleLayers.end(),
        [requested](const auto& installed) {
            return installed.get() == requested;
        });
    return found != pImpl->installedSampleLayers.end() ? (*found)->data : nullptr;
}

void AudioEngine::clearSampleLayer() {
    std::lock_guard<std::mutex> lock(pImpl->controlStateMutex);
    pImpl->publishRequestedSampleLayer(nullptr);
}

KickParams AudioEngine::getParams() const {
    return pImpl->params;
}

void AudioEngine::noteOff(int note) {
    pImpl->voiceAllocator->releaseVoice(note);
}

void AudioEngine::allNotesOff() {
    pImpl->voiceAllocator->releaseAll();
    pImpl->publishSampleLayerUsage();
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
    pImpl->enableSoftClipping.store(enable, std::memory_order_relaxed);
}

bool AudioEngine::isSoftClippingEnabled() const {
    return pImpl->enableSoftClipping.load(std::memory_order_relaxed);
}

void AudioEngine::setNaNDetectionEnabled(bool enable) {
    pImpl->enableNaNDetection.store(enable, std::memory_order_relaxed);
}

bool AudioEngine::isNaNDetectionEnabled() const {
    return pImpl->enableNaNDetection.load(std::memory_order_relaxed);
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
