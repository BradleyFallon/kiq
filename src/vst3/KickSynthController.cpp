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
        if (state->read((void*)paramId.data(), idLength) != idLength)
            return kResultFalse;

        // Read parameter value
        double value = 0.0;
        if (!streamer.readDouble(value))
            return kResultFalse;

        // TODO: Map string parameter ID to VST3 ParamID and set value
        // For now, we'll skip this as we need the mapping table
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
    parameters.addParameter(STR16("Base Pitch"), STR16("Hz"), 0, 0.5,
                           ParameterInfo::kCanAutomate, kParamBasePitch);
    
    parameters.addParameter(STR16("Sine Level"), STR16("%"), 0, 0.8,
                           ParameterInfo::kCanAutomate, kParamSineLevel);
    
    parameters.addParameter(STR16("Harmonic Ratio"), STR16("x"), 0, 0.2,
                           ParameterInfo::kCanAutomate, kParamHarmonicRatio);
    
    parameters.addParameter(STR16("Harmonic Level"), STR16("%"), 0, 0.3,
                           ParameterInfo::kCanAutomate, kParamHarmonicLevel);
    
    parameters.addParameter(STR16("Harmonic Mod Depth"), STR16("%"), 0, 0.5,
                           ParameterInfo::kCanAutomate, kParamHarmonicModDepth);
    
    parameters.addParameter(STR16("Noise Level"), STR16("%"), 0, 0.2,
                           ParameterInfo::kCanAutomate, kParamNoiseLevel);
    
    parameters.addParameter(STR16("Noise Mod Depth"), STR16("%"), 0, 0.7,
                           ParameterInfo::kCanAutomate, kParamNoiseModDepth);

    // Warm-Up Phase Parameters
    parameters.addParameter(STR16("Warm-Up Duration"), STR16("ms"), 0, 0.2,
                           ParameterInfo::kCanAutomate, kParamWarmUpDuration);
    
    parameters.addParameter(STR16("Warm-Up Start Freq"), STR16("Hz"), 0, 0.1,
                           ParameterInfo::kCanAutomate, kParamWarmUpStartFreq);
    
    parameters.addParameter(STR16("Warm-Up Amplitude"), STR16("%"), 0, 0.5,
                           ParameterInfo::kCanAutomate, kParamWarmUpAmplitude);

    // ADSR Envelope Parameters
    parameters.addParameter(STR16("Attack"), STR16("ms"), 0, 0.001,
                           ParameterInfo::kCanAutomate, kParamAttack);
    
    parameters.addParameter(STR16("Decay"), STR16("ms"), 0, 0.5,
                           ParameterInfo::kCanAutomate, kParamDecay);
    
    parameters.addParameter(STR16("Sustain"), STR16("%"), 0, 0.0,
                           ParameterInfo::kCanAutomate, kParamSustain);
    
    parameters.addParameter(STR16("Release"), STR16("ms"), 0, 0.1,
                           ParameterInfo::kCanAutomate, kParamRelease);

    // Pitch Envelope Parameters
    parameters.addParameter(STR16("Pitch Envelope Depth"), STR16("Hz"), 0, 0.25,
                           ParameterInfo::kCanAutomate, kParamPitchEnvelopeDepth);

    // Compressor Parameters
    parameters.addParameter(STR16("Compressor Threshold"), STR16("dB"), 0, 0.8,
                           ParameterInfo::kCanAutomate, kParamCompressorThreshold);
    
    parameters.addParameter(STR16("Compressor Ratio"), STR16(""), 0, 0.15,
                           ParameterInfo::kCanAutomate, kParamCompressorRatio);
    
    parameters.addParameter(STR16("Compressor Attack"), STR16("ms"), 0, 0.01,
                           ParameterInfo::kCanAutomate, kParamCompressorAttack);
    
    parameters.addParameter(STR16("Compressor Release"), STR16("ms"), 0, 0.1,
                           ParameterInfo::kCanAutomate, kParamCompressorRelease);
    
    parameters.addParameter(STR16("Compressor Mix"), STR16("%"), 0, 0.5,
                           ParameterInfo::kCanAutomate, kParamCompressorMix);
    
    parameters.addParameter(STR16("Compressor Bypass"), STR16(""), 0, 0.0,
                           ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass,
                           kParamCompressorBypass);

    // Reverb Parameters
    parameters.addParameter(STR16("Reverb Room Size"), STR16("%"), 0, 0.3,
                           ParameterInfo::kCanAutomate, kParamReverbRoomSize);
    
    parameters.addParameter(STR16("Reverb Decay Time"), STR16("s"), 0, 0.1,
                           ParameterInfo::kCanAutomate, kParamReverbDecayTime);
    
    parameters.addParameter(STR16("Reverb Damping"), STR16("%"), 0, 0.5,
                           ParameterInfo::kCanAutomate, kParamReverbDamping);
    
    parameters.addParameter(STR16("Reverb Mix"), STR16("%"), 0, 0.1,
                           ParameterInfo::kCanAutomate, kParamReverbMix);
    
    parameters.addParameter(STR16("Reverb Bypass"), STR16(""), 0, 0.0,
                           ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass,
                           kParamReverbBypass);

    // Master Parameters
    parameters.addParameter(STR16("Master Level"), STR16("%"), 0, 0.8,
                           ParameterInfo::kCanAutomate, kParamMasterLevel);
    
    parameters.addParameter(STR16("Pitch Tracking"), STR16(""), 0, 1.0,
                           ParameterInfo::kCanAutomate, kParamPitchTracking);
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
