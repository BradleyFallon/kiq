#pragma once

#include "KickParams.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace Steinberg {
namespace Vst {

static const FUID kKickSynthProcessorUID(0xA1B2C3D4, 0xE5F64A5B, 0x8C9D0E1F,
                                         0x2A3B4C5D);
static const FUID kKickSynthControllerUID(0xB2C3D4E5, 0xF6A74B5C, 0x9D0E1F2A,
                                          0x3B4C5D6E);

enum KickSynthParams : ParamID {
    // New ID range intentionally prevents old host automation from being
    // reinterpreted as the trajectory parameter model.
    kParamPitch0Hz = 1000,
    kParamPitch1TimeMs,
    kParamPitch1Hz,
    kParamPitch2TimeMs,
    kParamPitch2Hz,
    kParamPitch3TimeMs,
    kParamPitch3Hz,
    kParamAmp0Db,
    kParamAmp1TimeMs,
    kParamAmp1Db,
    kParamAmp2TimeMs,
    kParamAmp2Db,
    kParamAmp3TimeMs,
    kParamAmp3Db,
    kParamPitchCurve1,
    kParamPitchCurve2,
    kParamPitchCurve3,
    kParamAmpCurve1,
    kParamAmpCurve2,
    kParamAmpCurve3,
    kParamStrikePosition,
    kParamImpactLevel,
    kParamAirLevel,
    kParamAirDecayMs,
    kParamBeaterHardnessHz,
    kParamOutputGain,
    kParamMembraneLevel,
    kParamSampleLevel,
    kParamPhaseDegrees,
    kParamPhaseLockMs,
    kParamEqLowDb,
    kParamEqMidDb,
    kParamEqHighDb,
    kParamSaturation,
    kParamLimiterCeilingDb,

    // Read-only UI telemetry. These are intentionally outside the engine
    // parameter range and are never serialized as synthesis state.
    kParamOutputPeak = 2000,
    kParamOutputClip,
};

inline constexpr const char* kAuditionMessageId = "Kiq.Audition";
inline constexpr const char* kAuditionLoopMessageId = "Kiq.AuditionLoop";
inline constexpr const char* kAuditionLoopEnabledAttribute = "Enabled";
inline constexpr const char* kAuditionLoopBpmAttribute = "BPM";
inline constexpr const char* kSampleLayerMessageId = "Kiq.SampleLayer";
inline constexpr const char* kSampleLayerEnabledAttribute = "Enabled";
inline constexpr const char* kSampleLayerDataAttribute = "AudioData";

inline constexpr std::uint32_t kStateFormatVersion = 2;
inline constexpr std::uint32_t kMaximumStateParameterCount = 1024;
inline constexpr std::uint32_t kMaximumStateParameterIdBytes = 256;
inline constexpr std::uint32_t kMaximumSampleLayerSamples = 10000000;
inline constexpr float kMinimumSampleLayerRate = 1000.0f;
inline constexpr float kMaximumSampleLayerRate = 768000.0f;

struct KickSynthParameterMapping {
    ParamID vstId;
    KickDrum::KickParameterId engineId;
};

inline constexpr std::array<KickSynthParameterMapping,
                            static_cast<std::size_t>(KickDrum::KickParameterId::Count)>
    kKickSynthParameterMappings {{
        {kParamPitch0Hz, KickDrum::KickParameterId::Pitch0Hz},
        {kParamPitch1TimeMs, KickDrum::KickParameterId::Pitch1TimeMs},
        {kParamPitch1Hz, KickDrum::KickParameterId::Pitch1Hz},
        {kParamPitch2TimeMs, KickDrum::KickParameterId::Pitch2TimeMs},
        {kParamPitch2Hz, KickDrum::KickParameterId::Pitch2Hz},
        {kParamPitch3TimeMs, KickDrum::KickParameterId::Pitch3TimeMs},
        {kParamPitch3Hz, KickDrum::KickParameterId::Pitch3Hz},
        {kParamAmp0Db, KickDrum::KickParameterId::Amp0Db},
        {kParamAmp1TimeMs, KickDrum::KickParameterId::Amp1TimeMs},
        {kParamAmp1Db, KickDrum::KickParameterId::Amp1Db},
        {kParamAmp2TimeMs, KickDrum::KickParameterId::Amp2TimeMs},
        {kParamAmp2Db, KickDrum::KickParameterId::Amp2Db},
        {kParamAmp3TimeMs, KickDrum::KickParameterId::Amp3TimeMs},
        {kParamAmp3Db, KickDrum::KickParameterId::Amp3Db},
        {kParamPitchCurve1, KickDrum::KickParameterId::PitchCurve1},
        {kParamPitchCurve2, KickDrum::KickParameterId::PitchCurve2},
        {kParamPitchCurve3, KickDrum::KickParameterId::PitchCurve3},
        {kParamAmpCurve1, KickDrum::KickParameterId::AmpCurve1},
        {kParamAmpCurve2, KickDrum::KickParameterId::AmpCurve2},
        {kParamAmpCurve3, KickDrum::KickParameterId::AmpCurve3},
        {kParamStrikePosition, KickDrum::KickParameterId::StrikePosition},
        {kParamImpactLevel, KickDrum::KickParameterId::ImpactLevel},
        {kParamAirLevel, KickDrum::KickParameterId::AirLevel},
        {kParamAirDecayMs, KickDrum::KickParameterId::AirDecayMs},
        {kParamBeaterHardnessHz, KickDrum::KickParameterId::BeaterHardnessHz},
        {kParamOutputGain, KickDrum::KickParameterId::OutputGain},
        {kParamMembraneLevel, KickDrum::KickParameterId::MembraneLevel},
        {kParamSampleLevel, KickDrum::KickParameterId::SampleLevel},
        {kParamPhaseDegrees, KickDrum::KickParameterId::PhaseDegrees},
        {kParamPhaseLockMs, KickDrum::KickParameterId::PhaseLockMs},
        {kParamEqLowDb, KickDrum::KickParameterId::EqLowDb},
        {kParamEqMidDb, KickDrum::KickParameterId::EqMidDb},
        {kParamEqHighDb, KickDrum::KickParameterId::EqHighDb},
        {kParamSaturation, KickDrum::KickParameterId::Saturation},
        {kParamLimiterCeilingDb, KickDrum::KickParameterId::LimiterCeilingDb},
    }};

inline const KickSynthParameterMapping* findParameterMapping(ParamID vstId) {
    for (const auto& mapping : kKickSynthParameterMappings) {
        if (mapping.vstId == vstId) {
            return &mapping;
        }
    }
    return nullptr;
}

inline const KickSynthParameterMapping* findParameterMapping(std::string_view engineId) {
    const auto* spec = KickDrum::findKickParameterSpec(engineId);
    if (!spec) {
        return nullptr;
    }
    for (const auto& mapping : kKickSynthParameterMappings) {
        if (mapping.engineId == spec->id) {
            return &mapping;
        }
    }
    return nullptr;
}

inline ParamValue normalizeParameterValue(const KickSynthParameterMapping& mapping,
                                           double value) {
    const auto* spec = KickDrum::findKickParameterSpec(mapping.engineId);
    if (!spec || spec->maximum == spec->minimum) {
        return 0.0;
    }
    if (!std::isfinite(value)) {
        value = KickDrum::getDefaultKickParameter(mapping.engineId);
    }
    return std::clamp((value - spec->minimum) / (spec->maximum - spec->minimum),
                      0.0, 1.0);
}

inline double denormalizeParameterValue(const KickSynthParameterMapping& mapping,
                                        ParamValue normalizedValue) {
    const auto* spec = KickDrum::findKickParameterSpec(mapping.engineId);
    if (!spec) {
        return 0.0;
    }
    if (!std::isfinite(normalizedValue)) {
        return KickDrum::getDefaultKickParameter(mapping.engineId);
    }
    normalizedValue = std::clamp(normalizedValue, 0.0, 1.0);
    return spec->minimum + normalizedValue * (spec->maximum - spec->minimum);
}

inline ParamValue defaultNormalizedParameterValue(ParamID vstId) {
    const auto* mapping = findParameterMapping(vstId);
    return mapping ? normalizeParameterValue(
                         *mapping, KickDrum::getDefaultKickParameter(mapping->engineId))
                   : 0.0;
}

} // namespace Vst
} // namespace Steinberg
