#include "KickWavExporter.h"

#include "../include/AudioEngine.h"
#include "../voice/Voice.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>

namespace KickDrum {
namespace {

constexpr std::uint32_t kBytesPerSample = 3;
constexpr std::uint32_t kMinimumRenderSampleRate = 8000;
constexpr std::uint32_t kMaximumRenderSampleRate = 384000;
constexpr double kMaximumRenderDurationMs = 10000.0;

void setError(std::string* error, const std::string& message) {
    if (error) {
        *error = message;
    }
}

void writeU16(std::ostream& stream, std::uint16_t value) {
    stream.put(static_cast<char>(value & 0xffu));
    stream.put(static_cast<char>((value >> 8u) & 0xffu));
}

void writeU32(std::ostream& stream, std::uint32_t value) {
    stream.put(static_cast<char>(value & 0xffu));
    stream.put(static_cast<char>((value >> 8u) & 0xffu));
    stream.put(static_cast<char>((value >> 16u) & 0xffu));
    stream.put(static_cast<char>((value >> 24u) & 0xffu));
}

void writePcm24Sample(std::ostream& stream, float sample) {
    const float finiteSample = std::isfinite(sample) ? sample : 0.0f;
    const float clamped = std::clamp(finiteSample, -1.0f, 1.0f);
    const std::int32_t encoded = clamped <= -1.0f
        ? -8388608
        : static_cast<std::int32_t>(std::lround(clamped * 8388607.0f));
    const std::uint32_t bits = static_cast<std::uint32_t>(encoded);
    stream.put(static_cast<char>(bits & 0xffu));
    stream.put(static_cast<char>((bits >> 8u) & 0xffu));
    stream.put(static_cast<char>((bits >> 16u) & 0xffu));
}

} // namespace

std::vector<float> KickWavExporter::renderMono(
    const KickParams& requestedParams,
    const KickRenderSettings& settings) {
    if (settings.sampleRate < kMinimumRenderSampleRate ||
        settings.sampleRate > kMaximumRenderSampleRate) {
        return {};
    }

    const KickParams params = sanitizeKickParams(requestedParams);
    const float velocity = std::isfinite(settings.velocity)
        ? std::clamp(settings.velocity, 0.0f, 1.0f)
        : 1.0f;
    if (velocity <= 0.0f) {
        return {};
    }

    constexpr float kMaximumContactMs = 2.5f;
    double durationMs = std::max(
        static_cast<double>(params.amplitude.back().timeMs + Voice::kTailFadeMs),
        static_cast<double>(std::max(params.transient.airDecayMs,
                                     kMaximumContactMs)));
    if (settings.sampleLayer && params.sampleLevel > 0.0f &&
        !settings.sampleLayer->samples.empty()) {
        const float sourceRate =
            std::isfinite(settings.sampleLayer->sourceSampleRate) &&
                    settings.sampleLayer->sourceSampleRate > 0.0f
                ? std::clamp(settings.sampleLayer->sourceSampleRate,
                             1000.0f, 768000.0f)
                : 48000.0f;
        durationMs = std::max(
            durationMs,
            static_cast<double>(settings.sampleLayer->samples.size()) * 1000.0 /
                static_cast<double>(sourceRate));
    }
    if (params.outputStage.eqLowDb != 0.0f ||
        params.outputStage.eqMidDb != 0.0f ||
        params.outputStage.eqHighDb != 0.0f) {
        // Let the two one-pole crossover states decay below audibility.
        durationMs += 50.0f;
    }

    if (!std::isfinite(durationMs) || durationMs <= 0.0 ||
        durationMs > kMaximumRenderDurationMs) {
        return {};
    }

    const double requestedSamples =
        std::ceil(durationMs * static_cast<double>(settings.sampleRate) / 1000.0);
    const double maximumSamples =
        kMaximumRenderDurationMs * static_cast<double>(settings.sampleRate) / 1000.0;
    if (!std::isfinite(requestedSamples) || requestedSamples <= 0.0 ||
        requestedSamples > maximumSamples) {
        return {};
    }
    const std::size_t sampleCount = static_cast<std::size_t>(requestedSamples);
    std::vector<float> samples(sampleCount, 0.0f);

    AudioEngine engine;
    engine.initialize(static_cast<float>(settings.sampleRate));
    engine.prepare(sampleCount);
    engine.setSoftClippingEnabled(settings.softClip);
    for (const auto& spec : kKickParameterSpecs) {
        engine.setParameter(std::string(spec.key), getKickParameter(params, spec.id));
    }
    if (settings.sampleLayer) {
        engine.setSampleLayer(std::make_shared<SampleLayerData>(
            *settings.sampleLayer));
    }
    engine.noteOn(36, velocity);
    engine.processBlock(samples.data(), samples.size(), 1);
    return samples;
}

bool KickWavExporter::writePcm24(const std::string& path,
                                 const std::vector<float>& samples,
                                 std::uint32_t sampleRate,
                                 std::string* error) {
    if (error) {
        error->clear();
    }
    if (path.empty()) {
        setError(error, "WAV path is empty");
        return false;
    }
    if (sampleRate == 0 ||
        sampleRate > std::numeric_limits<std::uint32_t>::max() / kBytesPerSample) {
        setError(error, "WAV sample rate is invalid");
        return false;
    }

    const std::uint64_t dataBytes64 =
        static_cast<std::uint64_t>(samples.size()) * kBytesPerSample;
    const std::uint32_t padding = static_cast<std::uint32_t>(dataBytes64 & 1u);
    if (dataBytes64 + padding >
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) - 36u) {
        setError(error, "WAV is too large for a RIFF file");
        return false;
    }
    const auto dataBytes = static_cast<std::uint32_t>(dataBytes64);
    const std::uint32_t riffBytes = 36u + dataBytes + padding;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(error, "Could not open WAV for writing");
        return false;
    }

    file.write("RIFF", 4);
    writeU32(file, riffBytes);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    writeU32(file, 16);
    writeU16(file, 1); // Linear PCM
    writeU16(file, 1); // Mono
    writeU32(file, sampleRate);
    writeU32(file, sampleRate * kBytesPerSample);
    writeU16(file, kBytesPerSample);
    writeU16(file, 24);
    file.write("data", 4);
    writeU32(file, dataBytes);
    for (const float sample : samples) {
        writePcm24Sample(file, sample);
    }
    if (padding != 0) {
        file.put('\0');
    }

    if (!file) {
        setError(error, "Could not write WAV");
        return false;
    }
    return true;
}

bool KickWavExporter::renderToFile(const std::string& path,
                                   const KickParams& params,
                                   const KickRenderSettings& settings,
                                   std::string* error) {
    const std::vector<float> samples = renderMono(params, settings);
    if (samples.empty()) {
        setError(error, "Render produced no samples");
        return false;
    }
    return writePcm24(path, samples, settings.sampleRate, error);
}

} // namespace KickDrum
