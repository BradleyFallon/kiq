#pragma once

#include "../parameters/KickParams.h"

#include <cstdint>
#include <string>
#include <vector>

namespace KickDrum {

struct SampleLayerData;

struct KickRenderSettings {
    std::uint32_t sampleRate = 48000;
    float velocity = 1.0f;
    bool softClip = true;
    /** Optional immutable layer mixed by the same Voice path as realtime. */
    const SampleLayerData* sampleLayer = nullptr;
};

/** Offline deterministic rendering and mono 24-bit PCM WAV persistence. */
class KickWavExporter {
public:
    static std::vector<float> renderMono(
        const KickParams& params,
        const KickRenderSettings& settings = KickRenderSettings {});

    static bool writePcm24(const std::string& path,
                           const std::vector<float>& samples,
                           std::uint32_t sampleRate,
                           std::string* error = nullptr);

    static bool renderToFile(
        const std::string& path,
        const KickParams& params,
        const KickRenderSettings& settings = KickRenderSettings {},
        std::string* error = nullptr);
};

} // namespace KickDrum
