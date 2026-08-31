#include "KiqFactoryPresets.h"

#include "KickParams.h"

namespace KickDrum::UI {
namespace {

KickPresetDocument makePreset(const char* name,
                              std::initializer_list<std::pair<KickParameterId, float>> edits) {
    KickPresetDocument preset;
    preset.name = name;
    for (const auto& edit : edits) {
        setKickParameter(preset.params, edit.first, edit.second);
    }
    preset.params = sanitizeKickParams(preset.params);
    return preset;
}

} // namespace

const std::vector<KickPresetDocument>& factoryPresets() {
    static const std::vector<KickPresetDocument> presets {
        makePreset("Init — Bass House", {}),
        makePreset("Deep 808", {
            {KickParameterId::Pitch0Hz, 145.0f},
            {KickParameterId::Pitch1Hz, 72.0f},
            {KickParameterId::Pitch2Hz, 43.0f},
            {KickParameterId::Pitch3Hz, 43.0f},
            {KickParameterId::Pitch3TimeMs, 650.0f},
            {KickParameterId::Amp2TimeMs, 180.0f},
            {KickParameterId::Amp2Db, -5.0f},
            {KickParameterId::Amp3TimeMs, 720.0f},
            {KickParameterId::ImpactLevel, 0.08f},
            {KickParameterId::AirLevel, 0.04f},
            {KickParameterId::Saturation, 0.14f},
            {KickParameterId::LimiterCeilingDb, -1.0f},
        }),
        makePreset("Tight House", {
            {KickParameterId::Pitch0Hz, 265.0f},
            {KickParameterId::Pitch1Hz, 122.0f},
            {KickParameterId::Pitch2Hz, 58.0f},
            {KickParameterId::Pitch3Hz, 58.0f},
            {KickParameterId::Pitch2TimeMs, 45.0f},
            {KickParameterId::Pitch3TimeMs, 145.0f},
            {KickParameterId::Amp3TimeMs, 175.0f},
            {KickParameterId::ImpactLevel, 0.28f},
            {KickParameterId::AirLevel, 0.16f},
            {KickParameterId::BeaterHardnessHz, 9000.0f},
            {KickParameterId::Saturation, 0.12f},
        }),
        makePreset("Techno Punch", {
            {KickParameterId::Pitch0Hz, 192.0f},
            {KickParameterId::Pitch1Hz, 96.0f},
            {KickParameterId::Pitch2Hz, 48.0f},
            {KickParameterId::Pitch3Hz, 48.0f},
            {KickParameterId::Amp3TimeMs, 330.0f},
            {KickParameterId::StrikePosition, 0.50f},
            {KickParameterId::ImpactLevel, 0.34f},
            {KickParameterId::AirLevel, 0.20f},
            {KickParameterId::EqMidDb, 2.0f},
            {KickParameterId::Saturation, 0.25f},
            {KickParameterId::LimiterCeilingDb, -0.7f},
        }),
        makePreset("Soft Drum", {
            {KickParameterId::Pitch0Hz, 155.0f},
            {KickParameterId::Pitch1Hz, 91.0f},
            {KickParameterId::Pitch2Hz, 57.0f},
            {KickParameterId::Pitch3Hz, 54.0f},
            {KickParameterId::StrikePosition, 0.14f},
            {KickParameterId::ImpactLevel, 0.05f},
            {KickParameterId::AirLevel, 0.04f},
            {KickParameterId::BeaterHardnessHz, 2100.0f},
            {KickParameterId::EqHighDb, -3.0f},
        }),
        makePreset("Rim Knock", {
            {KickParameterId::StrikePosition, 0.72f},
            {KickParameterId::ImpactLevel, 0.46f},
            {KickParameterId::AirLevel, 0.13f},
            {KickParameterId::AirDecayMs, 4.0f},
            {KickParameterId::BeaterHardnessHz, 11800.0f},
            {KickParameterId::EqMidDb, 3.5f},
            {KickParameterId::EqHighDb, -1.5f},
            {KickParameterId::MembraneLevel, 0.72f},
        }),
        makePreset("Long Sub", {
            {KickParameterId::Pitch0Hz, 122.0f},
            {KickParameterId::Pitch1Hz, 68.0f},
            {KickParameterId::Pitch2Hz, 45.0f},
            {KickParameterId::Pitch3Hz, 45.0f},
            {KickParameterId::Pitch3TimeMs, 820.0f},
            {KickParameterId::Amp2TimeMs, 250.0f},
            {KickParameterId::Amp2Db, -4.0f},
            {KickParameterId::Amp3TimeMs, 900.0f},
            {KickParameterId::ImpactLevel, 0.10f},
            {KickParameterId::AirLevel, 0.02f},
            {KickParameterId::EqLowDb, 2.0f},
            {KickParameterId::LimiterCeilingDb, -1.0f},
        }),
    };
    return presets;
}

} // namespace KickDrum::UI
