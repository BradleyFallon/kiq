#include "KickSynthController.h"
#include "KickSynthCIDs.h"
#include "KiqMainView.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Steinberg {
namespace Vst {

KickSynthController::KickSynthController() = default;
KickSynthController::~KickSynthController() = default;

tresult PLUGIN_API KickSynthController::initialize(FUnknown* context) {
    const tresult result = EditController::initialize(context);
    if (result != kResultOk) {
        return result;
    }
    registerParameters();
    return kResultOk;
}

tresult PLUGIN_API KickSynthController::terminate() {
    return EditController::terminate();
}

tresult PLUGIN_API KickSynthController::setComponentState(IBStream* state) {
    if (!state) {
        return kResultFalse;
    }
    IBStreamer streamer(state, kLittleEndian);
    uint32 version = 0;
    uint32 numParams = 0;
    if (!streamer.readInt32u(version) ||
        version == 0 || version > kStateFormatVersion ||
        !streamer.readInt32u(numParams) ||
        numParams > kMaximumStateParameterCount) {
        return kResultFalse;
    }
    KickDrum::KickParams loadedParams = KickDrum::kDefaultKickParams;
    for (uint32 index = 0; index < numParams; ++index) {
        uint32 idLength = 0;
        if (!streamer.readInt32u(idLength) || idLength == 0 ||
            idLength > kMaximumStateParameterIdBytes) {
            return kResultFalse;
        }
        std::string parameterId(idLength, '\0');
        if (streamer.readRaw(parameterId.data(), idLength) != idLength) {
            return kResultFalse;
        }
        double value = 0.0;
        if (!streamer.readDouble(value)) {
            return kResultFalse;
        }
        if (const auto* mapping = findParameterMapping(parameterId)) {
            KickDrum::setKickParameter(
                loadedParams, mapping->engineId, static_cast<float>(value));
        }
    }
    std::shared_ptr<const KickDrum::SampleLayerData> loadedSampleLayer;
    if (version >= 2) {
        uint32 hasSampleLayer = 0;
        if (!streamer.readInt32u(hasSampleLayer) || hasSampleLayer > 1) {
            return kResultFalse;
        }
        if (hasSampleLayer != 0) {
            double sourceSampleRate = 0.0;
            uint32 sampleCount = 0;
            if (!streamer.readDouble(sourceSampleRate) ||
                !streamer.readInt32u(sampleCount) ||
                sampleCount == 0 ||
                sampleCount > kMaximumSampleLayerSamples ||
                !std::isfinite(sourceSampleRate) ||
                sourceSampleRate < kMinimumSampleLayerRate ||
                sourceSampleRate > kMaximumSampleLayerRate) {
                return kResultFalse;
            }
            auto layer = std::make_shared<KickDrum::SampleLayerData>();
            layer->sourceSampleRate = static_cast<float>(sourceSampleRate);
            layer->samples.resize(sampleCount);
            const TSize byteCount = static_cast<TSize>(
                static_cast<std::size_t>(sampleCount) * sizeof(float));
            if (byteCount > 0 &&
                streamer.readRaw(layer->samples.data(), byteCount) != byteCount) {
                return kResultFalse;
            }
            for (float& sample : layer->samples) {
                sample = std::isfinite(sample)
                             ? std::clamp(sample, -1.0f, 1.0f)
                             : 0.0f;
            }
            loadedSampleLayer = std::move(layer);
        }
    }

    loadedParams = KickDrum::sanitizeKickParams(loadedParams);
    for (const auto& mapping : kKickSynthParameterMappings) {
        setParamNormalized(
            mapping.vstId,
            normalizeParameterValue(
                mapping,
                KickDrum::getKickParameter(loadedParams, mapping.engineId)));
    }
    // The processor receives and restores this same component state directly;
    // retain the layer for the editor without sending a duplicate copy back.
    sampleLayer_ = std::move(loadedSampleLayer);
    return kResultOk;
}

IPlugView* PLUGIN_API KickSynthController::createView(FIDString name) {
    if (!name || !FIDStringsEqual(name, ViewType::kEditor)) {
        return nullptr;
    }
    auto* editor = new VSTGUI::AspectRatioVST3Editor(this, "KiqEditor", "kiq.uidesc");
    editor->setDelegate(this);
    editor->setMinZoomFactor(0.7);
    editor->setEditorSizeConstrains({770.0, 602.0}, {1650.0, 1290.0});
    return editor;
}

VSTGUI::CView* KickSynthController::createCustomView(
    VSTGUI::UTF8StringPtr name, const VSTGUI::UIAttributes&,
    const VSTGUI::IUIDescription*, VSTGUI::VST3Editor*) {
    if (name && std::strcmp(name, "KiqMainView") == 0) {
        return new KickDrum::UI::KiqMainView(
            {0.0, 0.0, KickDrum::UI::KiqMainView::kDesignWidth,
             KickDrum::UI::KiqMainView::kDesignHeight},
            *this);
    }
    return nullptr;
}

void KickSynthController::registerParameters() {
#define ADD_PARAMETER(title, unit, id, engineId)                                      \
    do {                                                                               \
        const auto* spec = KickDrum::findKickParameterSpec(engineId);                  \
        parameters.addParameter(new RangeParameter(                                    \
            STR16(title), id, STR16(unit), spec->minimum, spec->maximum,               \
            KickDrum::getDefaultKickParameter(engineId), 0,                            \
            ParameterInfo::kCanAutomate));                                              \
    } while (false)

    ADD_PARAMETER("Pitch 1", "Hz", kParamPitch0Hz, KickDrum::KickParameterId::Pitch0Hz);
    ADD_PARAMETER("Pitch 2 Time", "ms", kParamPitch1TimeMs, KickDrum::KickParameterId::Pitch1TimeMs);
    ADD_PARAMETER("Pitch 2", "Hz", kParamPitch1Hz, KickDrum::KickParameterId::Pitch1Hz);
    ADD_PARAMETER("Pitch 3 Time", "ms", kParamPitch2TimeMs, KickDrum::KickParameterId::Pitch2TimeMs);
    ADD_PARAMETER("Pitch 3", "Hz", kParamPitch2Hz, KickDrum::KickParameterId::Pitch2Hz);
    ADD_PARAMETER("Pitch 4 Time", "ms", kParamPitch3TimeMs, KickDrum::KickParameterId::Pitch3TimeMs);
    ADD_PARAMETER("Pitch 4", "Hz", kParamPitch3Hz, KickDrum::KickParameterId::Pitch3Hz);
    ADD_PARAMETER("Amplitude 1", "dB", kParamAmp0Db, KickDrum::KickParameterId::Amp0Db);
    ADD_PARAMETER("Amplitude 2 Time", "ms", kParamAmp1TimeMs, KickDrum::KickParameterId::Amp1TimeMs);
    ADD_PARAMETER("Amplitude 2", "dB", kParamAmp1Db, KickDrum::KickParameterId::Amp1Db);
    ADD_PARAMETER("Amplitude 3 Time", "ms", kParamAmp2TimeMs, KickDrum::KickParameterId::Amp2TimeMs);
    ADD_PARAMETER("Amplitude 3", "dB", kParamAmp2Db, KickDrum::KickParameterId::Amp2Db);
    ADD_PARAMETER("Amplitude 4 Time", "ms", kParamAmp3TimeMs, KickDrum::KickParameterId::Amp3TimeMs);
    ADD_PARAMETER("Amplitude 4", "dB", kParamAmp3Db, KickDrum::KickParameterId::Amp3Db);
    ADD_PARAMETER("Pitch Curve 1", "", kParamPitchCurve1, KickDrum::KickParameterId::PitchCurve1);
    ADD_PARAMETER("Pitch Curve 2", "", kParamPitchCurve2, KickDrum::KickParameterId::PitchCurve2);
    ADD_PARAMETER("Pitch Curve 3", "", kParamPitchCurve3, KickDrum::KickParameterId::PitchCurve3);
    ADD_PARAMETER("Amplitude Curve 1", "", kParamAmpCurve1, KickDrum::KickParameterId::AmpCurve1);
    ADD_PARAMETER("Amplitude Curve 2", "", kParamAmpCurve2, KickDrum::KickParameterId::AmpCurve2);
    ADD_PARAMETER("Amplitude Curve 3", "", kParamAmpCurve3, KickDrum::KickParameterId::AmpCurve3);
    ADD_PARAMETER("Strike Position", "", kParamStrikePosition, KickDrum::KickParameterId::StrikePosition);
    ADD_PARAMETER("Impact Level", "", kParamImpactLevel, KickDrum::KickParameterId::ImpactLevel);
    ADD_PARAMETER("Air Level", "", kParamAirLevel, KickDrum::KickParameterId::AirLevel);
    ADD_PARAMETER("Air Decay", "ms", kParamAirDecayMs, KickDrum::KickParameterId::AirDecayMs);
    ADD_PARAMETER("Beater Hardness", "Hz", kParamBeaterHardnessHz, KickDrum::KickParameterId::BeaterHardnessHz);
    ADD_PARAMETER("Output Gain", "", kParamOutputGain, KickDrum::KickParameterId::OutputGain);
    ADD_PARAMETER("Membrane Level", "", kParamMembraneLevel, KickDrum::KickParameterId::MembraneLevel);
    ADD_PARAMETER("Sample Level", "", kParamSampleLevel, KickDrum::KickParameterId::SampleLevel);
    ADD_PARAMETER("Phase Rotation", "deg", kParamPhaseDegrees, KickDrum::KickParameterId::PhaseDegrees);
    ADD_PARAMETER("Phase Lock Time", "ms", kParamPhaseLockMs, KickDrum::KickParameterId::PhaseLockMs);
    ADD_PARAMETER("Low EQ", "dB", kParamEqLowDb, KickDrum::KickParameterId::EqLowDb);
    ADD_PARAMETER("Mid EQ", "dB", kParamEqMidDb, KickDrum::KickParameterId::EqMidDb);
    ADD_PARAMETER("High EQ", "dB", kParamEqHighDb, KickDrum::KickParameterId::EqHighDb);
    ADD_PARAMETER("Saturation", "", kParamSaturation, KickDrum::KickParameterId::Saturation);
    ADD_PARAMETER("Limiter Ceiling", "dB", kParamLimiterCeilingDb, KickDrum::KickParameterId::LimiterCeilingDb);

#undef ADD_PARAMETER

    parameters.addParameter(new RangeParameter(
        STR16("Output Peak"), kParamOutputPeak, STR16(""), 0.0, 1.0, 0.0, 0,
        ParameterInfo::kIsReadOnly));
    parameters.addParameter(new RangeParameter(
        STR16("Output Clip"), kParamOutputClip, STR16(""), 0.0, 1.0, 0.0, 1,
        ParameterInfo::kIsReadOnly));
}

float KickSynthController::getParameter(KickDrum::KickParameterId id) {
    for (const auto& mapping : kKickSynthParameterMappings) {
        if (mapping.engineId == id) {
            return static_cast<float>(denormalizeParameterValue(
                mapping, getParamNormalized(mapping.vstId)));
        }
    }
    return KickDrum::getDefaultKickParameter(id);
}

void KickSynthController::beginParameterEdit(KickDrum::KickParameterId id) {
    for (const auto& mapping : kKickSynthParameterMappings) {
        if (mapping.engineId == id) {
            beginEdit(mapping.vstId);
            return;
        }
    }
}

void KickSynthController::performParameterEdit(KickDrum::KickParameterId id,
                                                float plainValue) {
    for (const auto& mapping : kKickSynthParameterMappings) {
        if (mapping.engineId != id) {
            continue;
        }
        const ParamValue normalized = normalizeParameterValue(mapping, plainValue);
        if (setParamNormalized(mapping.vstId, normalized) == kResultTrue) {
            performEdit(mapping.vstId, getParamNormalized(mapping.vstId));
        }
        return;
    }
}

void KickSynthController::endParameterEdit(KickDrum::KickParameterId id) {
    for (const auto& mapping : kKickSynthParameterMappings) {
        if (mapping.engineId == id) {
            endEdit(mapping.vstId);
            return;
        }
    }
}

void KickSynthController::triggerAudition() {
    sendMessageID(kAuditionMessageId);
}

void KickSynthController::setAuditionLoop(bool enabled, float bpm) {
    if (auto message = owned(allocateMessage())) {
        message->setMessageID(kAuditionLoopMessageId);
        if (auto* attributes = message->getAttributes()) {
            attributes->setInt(kAuditionLoopEnabledAttribute, enabled ? 1 : 0);
            attributes->setFloat(kAuditionLoopBpmAttribute, bpm);
            sendMessage(message);
        }
    }
}

void KickSynthController::setSampleLayer(
    std::shared_ptr<const KickDrum::SampleLayerData> sampleLayer) {
    sampleLayer_ = std::move(sampleLayer);
    if (sampleLayer_ &&
        (sampleLayer_->samples.empty() ||
         sampleLayer_->samples.size() > kMaximumSampleLayerSamples ||
         !std::isfinite(sampleLayer_->sourceSampleRate) ||
         sampleLayer_->sourceSampleRate < kMinimumSampleLayerRate ||
         sampleLayer_->sourceSampleRate > kMaximumSampleLayerRate)) {
        sampleLayer_.reset();
    }
    auto message = owned(allocateMessage());
    if (!message) {
        return;
    }
    message->setMessageID(kSampleLayerMessageId);
    auto* attributes = message->getAttributes();
    if (!attributes) {
        return;
    }
    attributes->setInt(kSampleLayerEnabledAttribute, sampleLayer_ ? 1 : 0);
    if (sampleLayer_) {
        constexpr std::uint32_t formatVersion = 1;
        const std::uint32_t sampleCount =
            static_cast<std::uint32_t>(sampleLayer_->samples.size());
        const std::size_t headerBytes = sizeof(formatVersion) +
                                        sizeof(sampleLayer_->sourceSampleRate) +
                                        sizeof(sampleCount);
        std::vector<std::uint8_t> payload(
            headerBytes + sampleLayer_->samples.size() * sizeof(float));
        std::size_t offset = 0;
        auto append = [&payload, &offset](const void* source, std::size_t size) {
            std::memcpy(payload.data() + offset, source, size);
            offset += size;
        };
        append(&formatVersion, sizeof(formatVersion));
        append(&sampleLayer_->sourceSampleRate,
               sizeof(sampleLayer_->sourceSampleRate));
        append(&sampleCount, sizeof(sampleCount));
        if (!sampleLayer_->samples.empty()) {
            append(sampleLayer_->samples.data(),
                   sampleLayer_->samples.size() * sizeof(float));
        }
        attributes->setBinary(kSampleLayerDataAttribute, payload.data(),
                              static_cast<std::uint32_t>(payload.size()));
    }
    sendMessage(message);
    setDirty(true);
}

std::shared_ptr<const KickDrum::SampleLayerData>
KickSynthController::getSampleLayer() const {
    return sampleLayer_;
}

float KickSynthController::getOutputPeak() {
    return static_cast<float>(getParamNormalized(kParamOutputPeak));
}

bool KickSynthController::getOutputClip() {
    return getParamNormalized(kParamOutputClip) >= 0.5;
}

} // namespace Vst
} // namespace Steinberg
