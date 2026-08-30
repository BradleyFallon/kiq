#include "KickSynthController.h"
#include "KickSynthCIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// KickSynthController
//------------------------------------------------------------------------
KickSynthController::KickSynthController()
{
}

//------------------------------------------------------------------------
KickSynthController::~KickSynthController()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthController::initialize(FUnknown* context)
{
    // Always call parent initialize first
    tresult result = EditController::initialize(context);
    if (result != kResultOk)
        return result;

    // Register all parameters
    registerParameters();

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthController::terminate()
{
    return EditController::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API KickSynthController::setComponentState(IBStream* state)
{
    // This is called when the processor state is loaded
    // We need to sync our parameters with the processor state
    
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);
    
    // Read version
    uint32 version = 0;
    if (!streamer.readInt32u(version))
        return kResultFalse;

    // Read number of parameters
    uint32 numParams = 0;
    if (!streamer.readInt32u(numParams))
        return kResultFalse;

    // Read each parameter and update our controller
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

        if (const auto* mapping = findParameterMapping(paramId))
        {
            setParamNormalized(mapping->vstId, normalizeParameterValue(*mapping, value));
        }
    }

    return kResultOk;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API KickSynthController::createView(FIDString name)
{
    // Check if we're being asked for the editor view
    if (FIDStringsEqual(name, ViewType::kEditor))
    {
        // For now, return nullptr (no custom UI)
        // In the future, we can create a custom VSTGUI view here
        return nullptr;
    }
    return nullptr;
}

//------------------------------------------------------------------------
void KickSynthController::registerParameters()
{
    // Generator Parameters
    parameters.addParameter(STR16("Base Pitch"), STR16("Hz"), 0, defaultNormalizedParameterValue(kParamBasePitch),
                           ParameterInfo::kCanAutomate, kParamBasePitch);
    
    parameters.addParameter(STR16("Sine Level"), STR16("%"), 0, defaultNormalizedParameterValue(kParamSineLevel),
                           ParameterInfo::kCanAutomate, kParamSineLevel);
    
    parameters.addParameter(STR16("Harmonic Ratio"), STR16("x"), 0, defaultNormalizedParameterValue(kParamHarmonicRatio),
                           ParameterInfo::kCanAutomate, kParamHarmonicRatio);
    
    parameters.addParameter(STR16("Harmonic Level"), STR16("%"), 0, defaultNormalizedParameterValue(kParamHarmonicLevel),
                           ParameterInfo::kCanAutomate, kParamHarmonicLevel);
    
    parameters.addParameter(STR16("Harmonic Mod Depth"), STR16("%"), 0, defaultNormalizedParameterValue(kParamHarmonicModDepth),
                           ParameterInfo::kCanAutomate, kParamHarmonicModDepth);
    
    parameters.addParameter(STR16("Noise Level"), STR16("%"), 0, defaultNormalizedParameterValue(kParamNoiseLevel),
                           ParameterInfo::kCanAutomate, kParamNoiseLevel);
    
    parameters.addParameter(STR16("Noise Mod Depth"), STR16("%"), 0, defaultNormalizedParameterValue(kParamNoiseModDepth),
                           ParameterInfo::kCanAutomate, kParamNoiseModDepth);

    // Warm-Up Phase Parameters
    parameters.addParameter(STR16("Warm-Up Duration"), STR16("ms"), 0, defaultNormalizedParameterValue(kParamWarmUpDuration),
                           ParameterInfo::kCanAutomate, kParamWarmUpDuration);
    
    parameters.addParameter(STR16("Warm-Up Start Freq"), STR16("Hz"), 0, defaultNormalizedParameterValue(kParamWarmUpStartFreq),
                           ParameterInfo::kCanAutomate, kParamWarmUpStartFreq);
    
    parameters.addParameter(STR16("Warm-Up Amplitude"), STR16("%"), 0, defaultNormalizedParameterValue(kParamWarmUpAmplitude),
                           ParameterInfo::kCanAutomate, kParamWarmUpAmplitude);

    // ADSR Envelope Parameters
    parameters.addParameter(STR16("Attack"), STR16("ms"), 0, defaultNormalizedParameterValue(kParamAttack),
                           ParameterInfo::kCanAutomate, kParamAttack);
    
    parameters.addParameter(STR16("Decay"), STR16("ms"), 0, defaultNormalizedParameterValue(kParamDecay),
                           ParameterInfo::kCanAutomate, kParamDecay);
    
    parameters.addParameter(STR16("Sustain"), STR16("%"), 0, defaultNormalizedParameterValue(kParamSustain),
                           ParameterInfo::kCanAutomate, kParamSustain);
    
    parameters.addParameter(STR16("Release"), STR16("ms"), 0, defaultNormalizedParameterValue(kParamRelease),
                           ParameterInfo::kCanAutomate, kParamRelease);

    // Pitch Envelope Parameters
    parameters.addParameter(STR16("Pitch Envelope Depth"), STR16("Hz"), 0, defaultNormalizedParameterValue(kParamPitchEnvelopeDepth),
                           ParameterInfo::kCanAutomate, kParamPitchEnvelopeDepth);

    // Envelope Curve Parameters
    parameters.addParameter(STR16("Attack Curve"), STR16(""), 3, defaultNormalizedParameterValue(kParamAttackCurve),
                           ParameterInfo::kCanAutomate, kParamAttackCurve);

    parameters.addParameter(STR16("Decay Curve"), STR16(""), 3, defaultNormalizedParameterValue(kParamDecayCurve),
                           ParameterInfo::kCanAutomate, kParamDecayCurve);

    parameters.addParameter(STR16("Release Curve"), STR16(""), 3, defaultNormalizedParameterValue(kParamReleaseCurve),
                           ParameterInfo::kCanAutomate, kParamReleaseCurve);

    // Compressor Parameters
    parameters.addParameter(STR16("Compressor Threshold"), STR16("dB"), 0, defaultNormalizedParameterValue(kParamCompressorThreshold),
                           ParameterInfo::kCanAutomate, kParamCompressorThreshold);
    
    parameters.addParameter(STR16("Compressor Ratio"), STR16(""), 0, defaultNormalizedParameterValue(kParamCompressorRatio),
                           ParameterInfo::kCanAutomate, kParamCompressorRatio);
    
    parameters.addParameter(STR16("Compressor Attack"), STR16("ms"), 0, defaultNormalizedParameterValue(kParamCompressorAttack),
                           ParameterInfo::kCanAutomate, kParamCompressorAttack);
    
    parameters.addParameter(STR16("Compressor Release"), STR16("ms"), 0, defaultNormalizedParameterValue(kParamCompressorRelease),
                           ParameterInfo::kCanAutomate, kParamCompressorRelease);
    
    parameters.addParameter(STR16("Compressor Mix"), STR16("%"), 0, defaultNormalizedParameterValue(kParamCompressorMix),
                           ParameterInfo::kCanAutomate, kParamCompressorMix);

    // Reverb Parameters
    parameters.addParameter(STR16("Reverb Room Size"), STR16("%"), 0, defaultNormalizedParameterValue(kParamReverbRoomSize),
                           ParameterInfo::kCanAutomate, kParamReverbRoomSize);
    
    parameters.addParameter(STR16("Reverb Decay Time"), STR16("s"), 0, defaultNormalizedParameterValue(kParamReverbDecayTime),
                           ParameterInfo::kCanAutomate, kParamReverbDecayTime);
    
    parameters.addParameter(STR16("Reverb Damping"), STR16("%"), 0, defaultNormalizedParameterValue(kParamReverbDamping),
                           ParameterInfo::kCanAutomate, kParamReverbDamping);
    
    parameters.addParameter(STR16("Reverb Mix"), STR16("%"), 0, defaultNormalizedParameterValue(kParamReverbMix),
                           ParameterInfo::kCanAutomate, kParamReverbMix);

    // Master Parameters
    parameters.addParameter(STR16("Master Level"), STR16("%"), 0, defaultNormalizedParameterValue(kParamMasterLevel),
                           ParameterInfo::kCanAutomate, kParamMasterLevel);
    
    parameters.addParameter(STR16("Pitch Tracking"), STR16(""), 1, defaultNormalizedParameterValue(kParamPitchTracking),
                           ParameterInfo::kCanAutomate, kParamPitchTracking);
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
