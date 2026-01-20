#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// Kick Drum Synthesizer Plugin UIDs
//------------------------------------------------------------------------

// Processor (Audio Component) UID
// {A1B2C3D4-E5F6-4A5B-8C9D-0E1F2A3B4C5D}
static const FUID kKickSynthProcessorUID(0xA1B2C3D4, 0xE5F64A5B, 0x8C9D0E1F, 0x2A3B4C5D);

// Controller (UI Component) UID
// {B2C3D4E5-F6A7-4B5C-9D0E-1F2A3B4C5D6E}
static const FUID kKickSynthControllerUID(0xB2C3D4E5, 0xF6A74B5C, 0x9D0E1F2A, 0x3B4C5D6E);

//------------------------------------------------------------------------
// Parameter IDs
//------------------------------------------------------------------------
enum KickSynthParams : Vst::ParamID
{
    // Generator Parameters
    kParamBasePitch = 100,
    kParamSineLevel = 101,
    kParamHarmonicRatio = 102,
    kParamHarmonicLevel = 103,
    kParamHarmonicModDepth = 104,
    kParamNoiseLevel = 105,
    kParamNoiseModDepth = 106,

    // Warm-Up Phase Parameters
    kParamWarmUpDuration = 200,
    kParamWarmUpStartFreq = 201,
    kParamWarmUpAmplitude = 202,

    // ADSR Envelope Parameters
    kParamAttack = 300,
    kParamDecay = 301,
    kParamSustain = 302,
    kParamRelease = 303,

    // Pitch Envelope Parameters
    kParamPitchEnvelopeDepth = 400,

    // Curve Parameters
    kParamAttackCurve = 500,
    kParamDecayCurve = 501,
    kParamReleaseCurve = 502,

    // Compressor Parameters
    kParamCompressorThreshold = 600,
    kParamCompressorRatio = 601,
    kParamCompressorAttack = 602,
    kParamCompressorRelease = 603,
    kParamCompressorMix = 604,
    kParamCompressorBypass = 605,

    // Reverb Parameters
    kParamReverbRoomSize = 700,
    kParamReverbDecayTime = 701,
    kParamReverbDamping = 702,
    kParamReverbMix = 703,
    kParamReverbBypass = 704,

    // Master Parameters
    kParamMasterLevel = 800,
    kParamPitchTracking = 801
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
