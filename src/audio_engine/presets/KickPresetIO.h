#pragma once

#include "../include/SampleLayerData.h"
#include "../parameters/KickParams.h"

#include <optional>
#include <string>

namespace KickDrum {

struct KickSamplePayload {
    std::string sourcePath;
    SampleLayerData audio;
};

struct KickPresetDocument {
    std::string name = "Untitled";
    KickParams params = kDefaultKickParams;
    std::optional<KickSamplePayload> sampleLayer;
};

/** Strict persistence for the current trajectory/membrane preset format. */
class KickPresetIO {
public:
    static constexpr const char* kFormatVersion = "kiq-kick-1";
    static constexpr const char* kFileExtension = ".kiqpreset";

    static std::string serialize(const KickPresetDocument& preset);

    /**
     * Parse a complete current-format preset. Legacy or partial parameter maps
     * are rejected; output is only changed after the whole document validates.
     */
    static bool deserialize(const std::string& json,
                            KickPresetDocument& output,
                            std::string* error = nullptr);

    static bool saveToFile(const std::string& path,
                           const KickPresetDocument& preset,
                           std::string* error = nullptr);
    static bool loadFromFile(const std::string& path,
                             KickPresetDocument& output,
                             std::string* error = nullptr);
};

} // namespace KickDrum
