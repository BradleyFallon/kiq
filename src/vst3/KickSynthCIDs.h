#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

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

    // Reverb Parameters
    kParamReverbRoomSize = 700,
    kParamReverbDecayTime = 701,
    kParamReverbDamping = 702,
    kParamReverbMix = 703,

    // Master Parameters
    kParamMasterLevel = 800,
    kParamPitchTracking = 801
};

struct KickSynthParameterMapping
{
    ParamID vstId;
    std::string_view engineId;
    double minimum;
    double maximum;
    double defaultValue;
    int32 stepCount;
};

inline constexpr std::array<KickSynthParameterMapping, 29> kKickSynthParameterMappings {{
    {kParamBasePitch, "basePitch", 20.0, 200.0, 50.0, 0},
    {kParamSineLevel, "sineLevel", 0.0, 100.0, 80.0, 0},
    {kParamHarmonicRatio, "harmonicRatio", 0.5, 8.0, 2.0, 0},
    {kParamHarmonicLevel, "harmonicLevel", 0.0, 100.0, 30.0, 0},
    {kParamHarmonicModDepth, "harmonicModDepth", 0.0, 100.0, 50.0, 0},
    {kParamNoiseLevel, "noiseLevel", 0.0, 100.0, 20.0, 0},
    {kParamNoiseModDepth, "noiseModDepth", 0.0, 100.0, 70.0, 0},
    {kParamWarmUpDuration, "warmUpDuration", 0.0, 100.0, 20.0, 0},
    {kParamWarmUpStartFreq, "warmUpStartFreq", 5.0, 50.0, 10.0, 0},
    {kParamWarmUpAmplitude, "warmUpAmplitude", 0.0, 100.0, 50.0, 0},
    {kParamAttack, "attack", 0.0, 1000.0, 1.0, 0},
    {kParamDecay, "decay", 0.0, 5000.0, 500.0, 0},
    {kParamSustain, "sustain", 0.0, 100.0, 0.0, 0},
    {kParamRelease, "release", 0.0, 5000.0, 100.0, 0},
    {kParamPitchEnvelopeDepth, "pitchEnvelopeDepth", 0.0, 2000.0, 500.0, 0},
    {kParamAttackCurve, "attackCurve", 0.0, 3.0, 0.0, 3},
    {kParamDecayCurve, "decayCurve", 0.0, 3.0, 1.0, 3},
    {kParamReleaseCurve, "releaseCurve", 0.0, 3.0, 1.0, 3},
    {kParamCompressorThreshold, "compressorThreshold", -60.0, 0.0, -12.0, 0},
    {kParamCompressorRatio, "compressorRatio", 1.0, 20.0, 4.0, 0},
    {kParamCompressorAttack, "compressorAttack", 0.1, 100.0, 1.0, 0},
    {kParamCompressorRelease, "compressorRelease", 10.0, 1000.0, 100.0, 0},
    {kParamCompressorMix, "compressorMix", 0.0, 100.0, 50.0, 0},
    {kParamReverbRoomSize, "reverbRoomSize", 0.0, 100.0, 30.0, 0},
    {kParamReverbDecayTime, "reverbDecayTime", 0.1, 10.0, 1.0, 0},
    {kParamReverbDamping, "reverbDamping", 0.0, 100.0, 50.0, 0},
    {kParamReverbMix, "reverbMix", 0.0, 100.0, 10.0, 0},
    {kParamMasterLevel, "masterLevel", 0.0, 100.0, 80.0, 0},
    {kParamPitchTracking, "pitchTracking", 0.0, 1.0, 1.0, 1},
}};

inline const KickSynthParameterMapping* findParameterMapping(ParamID vstId)
{
    for (const auto& mapping : kKickSynthParameterMappings)
    {
        if (mapping.vstId == vstId)
            return &mapping;
    }
    return nullptr;
}

inline const KickSynthParameterMapping* findParameterMapping(std::string_view engineId)
{
    for (const auto& mapping : kKickSynthParameterMappings)
    {
        if (mapping.engineId == engineId)
            return &mapping;
    }
    return nullptr;
}

inline ParamValue normalizeParameterValue(const KickSynthParameterMapping& mapping,
                                           double value)
{
    if (mapping.maximum == mapping.minimum)
        return 0.0;

    return std::clamp((value - mapping.minimum) / (mapping.maximum - mapping.minimum),
                      0.0, 1.0);
}

inline double denormalizeParameterValue(const KickSynthParameterMapping& mapping,
                                        ParamValue normalizedValue)
{
    normalizedValue = std::clamp(normalizedValue, 0.0, 1.0);
    if (mapping.stepCount > 0)
        normalizedValue = std::round(normalizedValue * mapping.stepCount) / mapping.stepCount;

    return mapping.minimum + normalizedValue * (mapping.maximum - mapping.minimum);
}

inline ParamValue defaultNormalizedParameterValue(ParamID vstId)
{
    const auto* mapping = findParameterMapping(vstId);
    return mapping ? normalizeParameterValue(*mapping, mapping->defaultValue) : 0.0;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
