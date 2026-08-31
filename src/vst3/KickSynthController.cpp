#include "KickSynthController.h"
#include "KickSynthCIDs.h"
#include "KiqMainView.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <cstring>
#include <string_view>

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
    if (!streamer.readInt32u(version) || !streamer.readInt32u(numParams)) {
        return kResultFalse;
    }
    for (uint32 index = 0; index < numParams; ++index) {
        uint32 idLength = 0;
        if (!streamer.readInt32u(idLength)) {
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
            setParamNormalized(mapping->vstId,
                               normalizeParameterValue(*mapping, value));
        }
    }
    return kResultOk;
}

IPlugView* PLUGIN_API KickSynthController::createView(FIDString name) {
    if (!name || !FIDStringsEqual(name, ViewType::kEditor)) {
        return nullptr;
    }
    auto* editor = new VSTGUI::AspectRatioVST3Editor(this, "KiqEditor", "kiq.uidesc");
    editor->setDelegate(this);
    editor->setMinZoomFactor(0.7);
    editor->setEditorSizeConstrains({770.0, 490.0}, {1650.0, 1050.0});
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
    ADD_PARAMETER("Start Phase", "cycles", kParamStartPhase, KickDrum::KickParameterId::StartPhase);
    ADD_PARAMETER("Click Level", "", kParamClickLevel, KickDrum::KickParameterId::ClickLevel);
    ADD_PARAMETER("Noise Level", "", kParamNoiseLevel, KickDrum::KickParameterId::NoiseLevel);
    ADD_PARAMETER("Noise Decay", "ms", kParamNoiseDecayMs, KickDrum::KickParameterId::NoiseDecayMs);
    ADD_PARAMETER("Noise Tone", "Hz", kParamNoiseToneHz, KickDrum::KickParameterId::NoiseToneHz);
    ADD_PARAMETER("Output Gain", "", kParamOutputGain, KickDrum::KickParameterId::OutputGain);

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

float KickSynthController::getOutputPeak() {
    return static_cast<float>(getParamNormalized(kParamOutputPeak));
}

bool KickSynthController::getOutputClip() {
    return getParamNormalized(kParamOutputClip) >= 0.5;
}

} // namespace Vst
} // namespace Steinberg
