#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace KickDrum {

constexpr std::size_t kTrajectoryPointCount = 4;

struct CurvePoint {
    float timeMs;
    float value;
    float curve;
};

struct TransientParams {
    float clickLevel;
    float noiseLevel;
    float noiseDecayMs;
    float noiseToneHz;
};

struct KickParams {
    std::array<CurvePoint, kTrajectoryPointCount> pitch;
    std::array<CurvePoint, kTrajectoryPointCount> amplitude;
    float startPhase;
    TransientParams transient;
    float outputGain;
};

inline constexpr KickParams kDefaultKickParams {
    // Bass-house body: absolute frequency in Hz.
    {{{0.0f, 220.0f, -0.35f},
      {18.0f, 105.0f, -0.20f},
      {58.0f, 52.0f, 0.0f},
      {200.0f, 52.0f, 0.0f}}},
    // Amplitude is stored in dB and converted to linear gain while rendering.
    {{{0.0f, -60.0f, -0.60f},
      {0.5f, 0.0f, 0.15f},
      {65.0f, -8.0f, 0.25f},
      {220.0f, -60.0f, 0.0f}}},
    0.25f,
    {0.18f, 0.12f, 7.0f, 6500.0f},
    0.8f,
};

enum class KickParameterId : std::uint32_t {
    Pitch0Hz,
    Pitch1TimeMs,
    Pitch1Hz,
    Pitch2TimeMs,
    Pitch2Hz,
    Pitch3TimeMs,
    Pitch3Hz,
    Amp0Db,
    Amp1TimeMs,
    Amp1Db,
    Amp2TimeMs,
    Amp2Db,
    Amp3TimeMs,
    Amp3Db,
    PitchCurve1,
    PitchCurve2,
    PitchCurve3,
    AmpCurve1,
    AmpCurve2,
    AmpCurve3,
    StartPhase,
    ClickLevel,
    NoiseLevel,
    NoiseDecayMs,
    NoiseToneHz,
    OutputGain,
    Count,
};

struct KickParameterSpec {
    KickParameterId id;
    std::string_view key;
    std::string_view name;
    float minimum;
    float maximum;
    std::string_view unit;
};

inline constexpr std::array<KickParameterSpec,
                            static_cast<std::size_t>(KickParameterId::Count)>
    kKickParameterSpecs {{
        {KickParameterId::Pitch0Hz, "pitch0Hz", "Pitch 1", 20.0f, 1000.0f, "Hz"},
        {KickParameterId::Pitch1TimeMs, "pitch1TimeMs", "Pitch 2 Time", 0.1f, 100.0f, "ms"},
        {KickParameterId::Pitch1Hz, "pitch1Hz", "Pitch 2", 20.0f, 1000.0f, "Hz"},
        {KickParameterId::Pitch2TimeMs, "pitch2TimeMs", "Pitch 3 Time", 1.0f, 300.0f, "ms"},
        {KickParameterId::Pitch2Hz, "pitch2Hz", "Pitch 3", 20.0f, 1000.0f, "Hz"},
        {KickParameterId::Pitch3TimeMs, "pitch3TimeMs", "Pitch 4 Time", 10.0f, 1000.0f, "ms"},
        {KickParameterId::Pitch3Hz, "pitch3Hz", "Pitch 4", 20.0f, 1000.0f, "Hz"},
        {KickParameterId::Amp0Db, "amp0Db", "Amplitude 1", -60.0f, 6.0f, "dB"},
        {KickParameterId::Amp1TimeMs, "amp1TimeMs", "Amplitude 2 Time", 0.1f, 20.0f, "ms"},
        {KickParameterId::Amp1Db, "amp1Db", "Amplitude 2", -60.0f, 6.0f, "dB"},
        {KickParameterId::Amp2TimeMs, "amp2TimeMs", "Amplitude 3 Time", 1.0f, 500.0f, "ms"},
        {KickParameterId::Amp2Db, "amp2Db", "Amplitude 3", -60.0f, 6.0f, "dB"},
        {KickParameterId::Amp3TimeMs, "amp3TimeMs", "Amplitude 4 Time", 10.0f, 2000.0f, "ms"},
        {KickParameterId::Amp3Db, "amp3Db", "Amplitude 4", -60.0f, 6.0f, "dB"},
        {KickParameterId::PitchCurve1, "pitchCurve1", "Pitch Curve 1", -1.0f, 1.0f, ""},
        {KickParameterId::PitchCurve2, "pitchCurve2", "Pitch Curve 2", -1.0f, 1.0f, ""},
        {KickParameterId::PitchCurve3, "pitchCurve3", "Pitch Curve 3", -1.0f, 1.0f, ""},
        {KickParameterId::AmpCurve1, "ampCurve1", "Amplitude Curve 1", -1.0f, 1.0f, ""},
        {KickParameterId::AmpCurve2, "ampCurve2", "Amplitude Curve 2", -1.0f, 1.0f, ""},
        {KickParameterId::AmpCurve3, "ampCurve3", "Amplitude Curve 3", -1.0f, 1.0f, ""},
        {KickParameterId::StartPhase, "startPhase", "Start Phase", 0.0f, 1.0f, "cycles"},
        {KickParameterId::ClickLevel, "clickLevel", "Click Level", 0.0f, 1.0f, ""},
        {KickParameterId::NoiseLevel, "noiseLevel", "Noise Level", 0.0f, 1.0f, ""},
        {KickParameterId::NoiseDecayMs, "noiseDecayMs", "Noise Decay", 1.0f, 50.0f, "ms"},
        {KickParameterId::NoiseToneHz, "noiseToneHz", "Noise Tone", 200.0f, 16000.0f, "Hz"},
        {KickParameterId::OutputGain, "outputGain", "Output Gain", 0.0f, 1.0f, ""},
    }};

const KickParameterSpec* findKickParameterSpec(KickParameterId id);
const KickParameterSpec* findKickParameterSpec(std::string_view key);
float getKickParameter(const KickParams& params, KickParameterId id);
void setKickParameter(KickParams& params, KickParameterId id, float value);
float getDefaultKickParameter(KickParameterId id);
KickParams sanitizeKickParams(const KickParams& params);

} // namespace KickDrum
