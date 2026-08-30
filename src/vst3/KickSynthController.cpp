#include "KickSynthController.h"
#include "KickSynthCIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

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

IPlugView* PLUGIN_API KickSynthController::createView(FIDString) {
    return nullptr;
}

void KickSynthController::registerParameters() {
#define ADD_PARAMETER(title, unit, id)                                                  \
    parameters.addParameter(STR16(title), STR16(unit), 0,                              \
                            defaultNormalizedParameterValue(id),                       \
                            ParameterInfo::kCanAutomate, id)

    ADD_PARAMETER("Pitch 1", "Hz", kParamPitch0Hz);
    ADD_PARAMETER("Pitch 2 Time", "ms", kParamPitch1TimeMs);
    ADD_PARAMETER("Pitch 2", "Hz", kParamPitch1Hz);
    ADD_PARAMETER("Pitch 3 Time", "ms", kParamPitch2TimeMs);
    ADD_PARAMETER("Pitch 3", "Hz", kParamPitch2Hz);
    ADD_PARAMETER("Pitch 4 Time", "ms", kParamPitch3TimeMs);
    ADD_PARAMETER("Pitch 4", "Hz", kParamPitch3Hz);
    ADD_PARAMETER("Amplitude 1", "dB", kParamAmp0Db);
    ADD_PARAMETER("Amplitude 2 Time", "ms", kParamAmp1TimeMs);
    ADD_PARAMETER("Amplitude 2", "dB", kParamAmp1Db);
    ADD_PARAMETER("Amplitude 3 Time", "ms", kParamAmp2TimeMs);
    ADD_PARAMETER("Amplitude 3", "dB", kParamAmp2Db);
    ADD_PARAMETER("Amplitude 4 Time", "ms", kParamAmp3TimeMs);
    ADD_PARAMETER("Amplitude 4", "dB", kParamAmp3Db);
    ADD_PARAMETER("Pitch Curve 1", "", kParamPitchCurve1);
    ADD_PARAMETER("Pitch Curve 2", "", kParamPitchCurve2);
    ADD_PARAMETER("Pitch Curve 3", "", kParamPitchCurve3);
    ADD_PARAMETER("Amplitude Curve 1", "", kParamAmpCurve1);
    ADD_PARAMETER("Amplitude Curve 2", "", kParamAmpCurve2);
    ADD_PARAMETER("Amplitude Curve 3", "", kParamAmpCurve3);
    ADD_PARAMETER("Start Phase", "cycles", kParamStartPhase);
    ADD_PARAMETER("Click Level", "", kParamClickLevel);
    ADD_PARAMETER("Noise Level", "", kParamNoiseLevel);
    ADD_PARAMETER("Noise Decay", "ms", kParamNoiseDecayMs);
    ADD_PARAMETER("Noise Tone", "Hz", kParamNoiseToneHz);
    ADD_PARAMETER("Output Gain", "", kParamOutputGain);

#undef ADD_PARAMETER
}

} // namespace Vst
} // namespace Steinberg
