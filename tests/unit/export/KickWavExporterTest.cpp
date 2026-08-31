#include <gtest/gtest.h>

#include "audio_engine/export/KickWavExporter.h"
#include "audio_engine/include/SampleLayerData.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace KickDrum;

namespace {

std::uint16_t readU16(const std::vector<unsigned char>& bytes,
                      std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1] << 8u);
}

std::uint32_t readU32(const std::vector<unsigned char>& bytes,
                      std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

std::filesystem::path temporaryWavPath() {
    return std::filesystem::temp_directory_path() /
           "kiq_kick_wav_exporter_test.wav";
}

} // namespace

TEST(KickWavExporterTest, OfflineRenderIsDeterministicAndBounded) {
    const auto first = KickWavExporter::renderMono(kDefaultKickParams);
    const auto second = KickWavExporter::renderMono(kDefaultKickParams);

    ASSERT_FALSE(first.empty());
    EXPECT_EQ(first, second);
    EXPECT_EQ(first.size(), 10800u); // 220 ms body + 5 ms tail at 48 kHz.
    for (const float sample : first) {
        EXPECT_TRUE(std::isfinite(sample));
        EXPECT_LE(std::abs(sample), 1.0f);
    }
}

TEST(KickWavExporterTest, VelocityScalesUnclippedRender) {
    KickRenderSettings full;
    full.softClip = false;
    KickRenderSettings half = full;
    half.velocity = 0.5f;
    const auto fullSamples = KickWavExporter::renderMono(kDefaultKickParams, full);
    const auto halfSamples = KickWavExporter::renderMono(kDefaultKickParams, half);
    ASSERT_EQ(fullSamples.size(), halfSamples.size());
    for (std::size_t index = 0; index < fullSamples.size(); ++index) {
        EXPECT_FLOAT_EQ(halfSamples[index], fullSamples[index] * 0.5f);
    }
}

TEST(KickWavExporterTest, IncludesOptionalRealtimeSampleLayer) {
    KickParams params = kDefaultKickParams;
    params.membraneLevel = 0.0f;
    params.transient.impactLevel = 0.0f;
    params.transient.airLevel = 0.0f;
    params.sampleLevel = 1.0f;
    params.outputGain = 1.0f;
    SampleLayerData sampleLayer;
    sampleLayer.sourceSampleRate = 24000.0f;
    sampleLayer.samples.assign(480, 0.25f);

    KickRenderSettings withoutLayer;
    withoutLayer.softClip = false;
    KickRenderSettings withLayer = withoutLayer;
    withLayer.sampleLayer = &sampleLayer;
    const auto silent = KickWavExporter::renderMono(params, withoutLayer);
    const auto layered = KickWavExporter::renderMono(params, withLayer);

    ASSERT_EQ(silent.size(), layered.size());
    EXPECT_TRUE(std::all_of(silent.begin(), silent.end(), [](float sample) {
        return sample == 0.0f;
    }));
    EXPECT_FLOAT_EQ(layered.front(), 0.25f);
    EXPECT_GT(*std::max_element(layered.begin(), layered.end()), 0.0f);
}

TEST(KickWavExporterTest, UsesEngineSampleRateBoundsAndRejectsHugeRenders) {
    KickParams params = kDefaultKickParams;
    params.sampleLevel = 1.0f;
    SampleLayerData sampleLayer;
    sampleLayer.sourceSampleRate = 1536000.0f;
    sampleLayer.samples.assign(384000, 0.1f);

    KickRenderSettings settings;
    settings.sampleLayer = &sampleLayer;
    const auto clampedRate = KickWavExporter::renderMono(params, settings);
    EXPECT_EQ(clampedRate.size(), 24000u); // 0.5 seconds at the 768 kHz source cap.

    sampleLayer.sourceSampleRate = 1000.0f;
    sampleLayer.samples.assign(10001, 0.1f);
    EXPECT_TRUE(KickWavExporter::renderMono(params, settings).empty());
}

TEST(KickWavExporterTest, UsesTheRealtimeEqSaturationAndLimiterStage) {
    KickParams params = kDefaultKickParams;
    params.outputStage.eqLowDb = 6.0f;
    params.outputStage.eqMidDb = -3.0f;
    params.outputStage.eqHighDb = 4.0f;
    params.outputStage.saturation = 1.0f;
    params.outputStage.limiterCeilingDb = -6.0f;

    const auto rendered = KickWavExporter::renderMono(params);
    EXPECT_EQ(rendered.size(), 13200u); // Voice plus 50 ms EQ-state tail.
    const float peak = *std::max_element(
        rendered.begin(), rendered.end(), [](float left, float right) {
            return std::abs(left) < std::abs(right);
        });
    EXPECT_LE(std::abs(peak), std::pow(10.0f, -6.0f / 20.0f) + 1.0e-6f);
}

TEST(KickWavExporterTest, WritesCanonicalMono24BitPcmHeaderAndSamples) {
    const auto path = temporaryWavPath();
    std::filesystem::remove(path);
    const std::vector<float> samples {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    std::string error;
    ASSERT_TRUE(KickWavExporter::writePcm24(
        path.string(), samples, 48000, &error)) << error;

    std::ifstream file(path, std::ios::binary);
    const std::vector<unsigned char> bytes {
        std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    ASSERT_EQ(bytes.size(), 60u); // 44-byte header + 15 data bytes + pad.
    EXPECT_EQ(std::string(bytes.begin(), bytes.begin() + 4), "RIFF");
    EXPECT_EQ(readU32(bytes, 4), 52u);
    EXPECT_EQ(std::string(bytes.begin() + 8, bytes.begin() + 12), "WAVE");
    EXPECT_EQ(readU16(bytes, 20), 1u);
    EXPECT_EQ(readU16(bytes, 22), 1u);
    EXPECT_EQ(readU32(bytes, 24), 48000u);
    EXPECT_EQ(readU32(bytes, 28), 144000u);
    EXPECT_EQ(readU16(bytes, 32), 3u);
    EXPECT_EQ(readU16(bytes, 34), 24u);
    EXPECT_EQ(readU32(bytes, 40), 15u);
    EXPECT_EQ(bytes[44], 0x00u);
    EXPECT_EQ(bytes[45], 0x00u);
    EXPECT_EQ(bytes[46], 0x80u);
    EXPECT_EQ(bytes[50], 0x00u);
    EXPECT_EQ(bytes[51], 0x00u);
    EXPECT_EQ(bytes[52], 0x00u);
    EXPECT_EQ(bytes[56], 0xffu);
    EXPECT_EQ(bytes[57], 0xffu);
    EXPECT_EQ(bytes[58], 0x7fu);
    EXPECT_EQ(bytes[59], 0x00u);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST(KickWavExporterTest, RenderToFileProducesAUsableWav) {
    const auto path = temporaryWavPath();
    std::filesystem::remove(path);
    std::string error;
    ASSERT_TRUE(KickWavExporter::renderToFile(
        path.string(), kDefaultKickParams, {}, &error)) << error;
    EXPECT_GT(std::filesystem::file_size(path), 44u);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST(KickWavExporterTest, RejectsInvalidRenderAndFileSettings) {
    KickRenderSettings invalid;
    invalid.sampleRate = 0;
    EXPECT_TRUE(KickWavExporter::renderMono(kDefaultKickParams, invalid).empty());
    invalid.sampleRate = 1000000;
    EXPECT_TRUE(KickWavExporter::renderMono(kDefaultKickParams, invalid).empty());

    std::string error;
    EXPECT_FALSE(KickWavExporter::writePcm24(
        temporaryWavPath().string(), {}, 0, &error));
    EXPECT_FALSE(error.empty());
}
