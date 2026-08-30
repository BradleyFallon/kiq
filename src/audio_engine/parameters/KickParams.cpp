#include "KickParams.h"

#include <algorithm>

namespace KickDrum {

const KickParameterSpec* findKickParameterSpec(KickParameterId id) {
    const auto index = static_cast<std::size_t>(id);
    return index < kKickParameterSpecs.size() ? &kKickParameterSpecs[index] : nullptr;
}

const KickParameterSpec* findKickParameterSpec(std::string_view key) {
    for (const auto& spec : kKickParameterSpecs) {
        if (spec.key == key) {
            return &spec;
        }
    }
    return nullptr;
}

float getKickParameter(const KickParams& params, KickParameterId id) {
    switch (id) {
        case KickParameterId::Pitch0Hz: return params.pitch[0].value;
        case KickParameterId::Pitch1TimeMs: return params.pitch[1].timeMs;
        case KickParameterId::Pitch1Hz: return params.pitch[1].value;
        case KickParameterId::Pitch2TimeMs: return params.pitch[2].timeMs;
        case KickParameterId::Pitch2Hz: return params.pitch[2].value;
        case KickParameterId::Pitch3TimeMs: return params.pitch[3].timeMs;
        case KickParameterId::Pitch3Hz: return params.pitch[3].value;
        case KickParameterId::Amp0Db: return params.amplitude[0].value;
        case KickParameterId::Amp1TimeMs: return params.amplitude[1].timeMs;
        case KickParameterId::Amp1Db: return params.amplitude[1].value;
        case KickParameterId::Amp2TimeMs: return params.amplitude[2].timeMs;
        case KickParameterId::Amp2Db: return params.amplitude[2].value;
        case KickParameterId::Amp3TimeMs: return params.amplitude[3].timeMs;
        case KickParameterId::Amp3Db: return params.amplitude[3].value;
        case KickParameterId::PitchCurve1: return params.pitch[0].curve;
        case KickParameterId::PitchCurve2: return params.pitch[1].curve;
        case KickParameterId::PitchCurve3: return params.pitch[2].curve;
        case KickParameterId::AmpCurve1: return params.amplitude[0].curve;
        case KickParameterId::AmpCurve2: return params.amplitude[1].curve;
        case KickParameterId::AmpCurve3: return params.amplitude[2].curve;
        case KickParameterId::StartPhase: return params.startPhase;
        case KickParameterId::ClickLevel: return params.transient.clickLevel;
        case KickParameterId::NoiseLevel: return params.transient.noiseLevel;
        case KickParameterId::NoiseDecayMs: return params.transient.noiseDecayMs;
        case KickParameterId::NoiseToneHz: return params.transient.noiseToneHz;
        case KickParameterId::OutputGain: return params.outputGain;
        case KickParameterId::Count: break;
    }
    return 0.0f;
}

void setKickParameter(KickParams& params, KickParameterId id, float value) {
    const auto* spec = findKickParameterSpec(id);
    if (!spec) {
        return;
    }
    value = std::clamp(value, spec->minimum, spec->maximum);

    switch (id) {
        case KickParameterId::Pitch0Hz: params.pitch[0].value = value; break;
        case KickParameterId::Pitch1TimeMs: params.pitch[1].timeMs = value; break;
        case KickParameterId::Pitch1Hz: params.pitch[1].value = value; break;
        case KickParameterId::Pitch2TimeMs: params.pitch[2].timeMs = value; break;
        case KickParameterId::Pitch2Hz: params.pitch[2].value = value; break;
        case KickParameterId::Pitch3TimeMs: params.pitch[3].timeMs = value; break;
        case KickParameterId::Pitch3Hz: params.pitch[3].value = value; break;
        case KickParameterId::Amp0Db: params.amplitude[0].value = value; break;
        case KickParameterId::Amp1TimeMs: params.amplitude[1].timeMs = value; break;
        case KickParameterId::Amp1Db: params.amplitude[1].value = value; break;
        case KickParameterId::Amp2TimeMs: params.amplitude[2].timeMs = value; break;
        case KickParameterId::Amp2Db: params.amplitude[2].value = value; break;
        case KickParameterId::Amp3TimeMs: params.amplitude[3].timeMs = value; break;
        case KickParameterId::Amp3Db: params.amplitude[3].value = value; break;
        case KickParameterId::PitchCurve1: params.pitch[0].curve = value; break;
        case KickParameterId::PitchCurve2: params.pitch[1].curve = value; break;
        case KickParameterId::PitchCurve3: params.pitch[2].curve = value; break;
        case KickParameterId::AmpCurve1: params.amplitude[0].curve = value; break;
        case KickParameterId::AmpCurve2: params.amplitude[1].curve = value; break;
        case KickParameterId::AmpCurve3: params.amplitude[2].curve = value; break;
        case KickParameterId::StartPhase: params.startPhase = value; break;
        case KickParameterId::ClickLevel: params.transient.clickLevel = value; break;
        case KickParameterId::NoiseLevel: params.transient.noiseLevel = value; break;
        case KickParameterId::NoiseDecayMs: params.transient.noiseDecayMs = value; break;
        case KickParameterId::NoiseToneHz: params.transient.noiseToneHz = value; break;
        case KickParameterId::OutputGain: params.outputGain = value; break;
        case KickParameterId::Count: break;
    }
}

float getDefaultKickParameter(KickParameterId id) {
    return getKickParameter(kDefaultKickParams, id);
}

KickParams sanitizeKickParams(const KickParams& params) {
    KickParams sanitized = params;
    for (const auto& spec : kKickParameterSpecs) {
        setKickParameter(sanitized, spec.id, getKickParameter(params, spec.id));
    }

    sanitized.pitch[0].timeMs = 0.0f;
    sanitized.amplitude[0].timeMs = 0.0f;
    for (std::size_t index = 1; index < kTrajectoryPointCount; ++index) {
        sanitized.pitch[index].timeMs =
            std::max(sanitized.pitch[index].timeMs, sanitized.pitch[index - 1].timeMs + 0.01f);
        sanitized.amplitude[index].timeMs =
            std::max(sanitized.amplitude[index].timeMs,
                     sanitized.amplitude[index - 1].timeMs + 0.01f);
    }
    return sanitized;
}

} // namespace KickDrum
