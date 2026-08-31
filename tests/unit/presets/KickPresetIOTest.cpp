#include <gtest/gtest.h>

#include "audio_engine/presets/KickPresetIO.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using namespace KickDrum;

namespace {

void expectSameParams(const KickParams& left, const KickParams& right) {
    for (const auto& spec : kKickParameterSpecs) {
        EXPECT_FLOAT_EQ(getKickParameter(left, spec.id),
                        getKickParameter(right, spec.id))
            << spec.key;
    }
}

std::filesystem::path temporaryPresetPath() {
    return std::filesystem::temp_directory_path() /
           "kiq_kick_preset_io_test.kiqpreset";
}

} // namespace

TEST(KickPresetIOTest, RoundTripsEveryCurrentParameterAtFloatPrecision) {
    KickPresetDocument original;
    original.name = "Studio \"A\"\nKick";
    original.params.pitch[1].value = 123.456787109375f;
    original.params.pitch[2].curve = -0.37123456597328186f;
    original.params.amplitude[2].timeMs = 87.654319763183594f;
    original.params.transient.beaterHardnessHz = 9123.4560546875f;

    const std::string json = KickPresetIO::serialize(original);
    EXPECT_NE(json.find("\"version\": \"kiq-kick-1\""), std::string::npos);
    EXPECT_NE(json.find("\"strikePosition\""), std::string::npos);
    EXPECT_EQ(json.find("basePitch"), std::string::npos);

    KickPresetDocument decoded;
    std::string error;
    ASSERT_TRUE(KickPresetIO::deserialize(json, decoded, &error)) << error;
    EXPECT_EQ(decoded.name, original.name);
    expectSameParams(decoded.params, sanitizeKickParams(original.params));
}

TEST(KickPresetIOTest, RejectsLegacyOrPartialParameterMaps) {
    const std::string legacy = R"({
        "name": "Old Kick",
        "version": "1.0.0",
        "parameters": {"basePitch": 52.0}
    })";
    KickPresetDocument output;
    output.name = "Unchanged";
    std::string error;
    EXPECT_FALSE(KickPresetIO::deserialize(legacy, output, &error));
    EXPECT_EQ(output.name, "Unchanged");
    EXPECT_FALSE(error.empty());
}

TEST(KickPresetIOTest, RejectsTrailingOrMalformedJson) {
    KickPresetDocument preset;
    const std::string valid = KickPresetIO::serialize(preset);
    KickPresetDocument output;
    output.name = "Unchanged";
    std::string error;

    EXPECT_FALSE(KickPresetIO::deserialize(valid + " trailing", output, &error));
    EXPECT_EQ(output.name, "Unchanged");

    std::string invalidPrimitive = valid;
    const std::string version = "\"version\": \"kiq-kick-1\"";
    const auto versionPosition = invalidPrimitive.find(version);
    ASSERT_NE(versionPosition, std::string::npos);
    invalidPrimitive.replace(versionPosition, version.size(),
                             "\"version\": unsupported");
    EXPECT_FALSE(KickPresetIO::deserialize(
        invalidPrimitive, output, &error));

    std::string missingComma = valid;
    const std::string separator = "\",\n  \"version\"";
    const auto separatorPosition = missingComma.find(separator);
    ASSERT_NE(separatorPosition, std::string::npos);
    missingComma.replace(separatorPosition, separator.size(),
                         "\"\n  \"version\"");
    EXPECT_FALSE(KickPresetIO::deserialize(missingComma, output, &error));
}

TEST(KickPresetIOTest, RejectsInvalidTrajectoryOrdering) {
    KickPresetDocument preset;
    std::string json = KickPresetIO::serialize(preset);
    const std::string oldValue = "\"pitch2TimeMs\": 58";
    const auto position = json.find(oldValue);
    ASSERT_NE(position, std::string::npos);
    json.replace(position, oldValue.size(), "\"pitch2TimeMs\": 2");

    KickPresetDocument output;
    std::string error;
    EXPECT_FALSE(KickPresetIO::deserialize(json, output, &error));
    EXPECT_NE(error.find("strictly increasing"), std::string::npos);
}

TEST(KickPresetIOTest, SavesAndLoadsCurrentFormatFile) {
    const auto path = temporaryPresetPath();
    std::filesystem::remove(path);
    KickPresetDocument original;
    original.name = "Saved Kick";
    original.params.outputGain = 0.61234569549560547f;

    std::string error;
    ASSERT_TRUE(KickPresetIO::saveToFile(path.string(), original, &error))
        << error;
    KickPresetDocument loaded;
    ASSERT_TRUE(KickPresetIO::loadFromFile(path.string(), loaded, &error))
        << error;
    EXPECT_EQ(loaded.name, original.name);
    expectSameParams(loaded.params, original.params);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST(KickPresetIOTest, RoundTripsOptionalEmbeddedMonoSampleLayer) {
    KickPresetDocument original;
    original.name = "Layered Kick";
    original.sampleLayer = KickSamplePayload {
        "/Samples/{favorite}/click.wav",
        {{-1.0f, -0.125f, 0.0f, 0.3333333432674408f, 1.0f}, 44100.0f},
    };

    const std::string json = KickPresetIO::serialize(original);
    EXPECT_NE(json.find("\"sampleLayer\""), std::string::npos);
    EXPECT_NE(json.find("float32-le-base64"), std::string::npos);

    KickPresetDocument decoded;
    std::string error;
    ASSERT_TRUE(KickPresetIO::deserialize(json, decoded, &error)) << error;
    ASSERT_TRUE(decoded.sampleLayer.has_value());
    EXPECT_EQ(decoded.sampleLayer->sourcePath,
              original.sampleLayer->sourcePath);
    EXPECT_FLOAT_EQ(decoded.sampleLayer->audio.sourceSampleRate, 44100.0f);
    EXPECT_EQ(decoded.sampleLayer->audio.samples,
              original.sampleLayer->audio.samples);
}

TEST(KickPresetIOTest, RoundTripsSourceOnlySampleLayerMetadata) {
    KickPresetDocument original;
    original.sampleLayer = KickSamplePayload {
        "/Samples/reference kick.wav", {{}, 48000.0f}};

    KickPresetDocument decoded;
    std::string error;
    ASSERT_TRUE(KickPresetIO::deserialize(
        KickPresetIO::serialize(original), decoded, &error)) << error;
    ASSERT_TRUE(decoded.sampleLayer);
    EXPECT_EQ(decoded.sampleLayer->sourcePath,
              original.sampleLayer->sourcePath);
    EXPECT_FLOAT_EQ(decoded.sampleLayer->audio.sourceSampleRate, 48000.0f);
    EXPECT_TRUE(decoded.sampleLayer->audio.samples.empty());
}

TEST(KickPresetIOTest, RejectsEmbeddedAudioWithoutASampleRateOnSave) {
    KickPresetDocument preset;
    preset.sampleLayer = KickSamplePayload {"", {{0.25f}, 0.0f}};
    std::string error;
    EXPECT_FALSE(KickPresetIO::saveToFile(
        temporaryPresetPath().string(), preset, &error));
    EXPECT_NE(error.find("sample rate"), std::string::npos);
}

TEST(KickPresetIOTest, RejectsEmbeddedAudioOutsideSupportedSampleRates) {
    KickPresetDocument preset;
    preset.sampleLayer = KickSamplePayload {"", {{0.25f}, 999.0f}};
    std::string error;
    EXPECT_FALSE(KickPresetIO::saveToFile(
        temporaryPresetPath().string(), preset, &error));
    EXPECT_NE(error.find("sample rate"), std::string::npos);

    preset.sampleLayer->audio.sourceSampleRate = 768001.0f;
    error.clear();
    EXPECT_FALSE(KickPresetIO::saveToFile(
        temporaryPresetPath().string(), preset, &error));
    EXPECT_NE(error.find("sample rate"), std::string::npos);
}

TEST(KickPresetIOTest, ReportsFileErrorsWithoutChangingOutput) {
    KickPresetDocument output;
    output.name = "Keep Me";
    std::string error;
    EXPECT_FALSE(KickPresetIO::loadFromFile(
        "/path/that/does/not/exist/missing.kiqpreset", output, &error));
    EXPECT_EQ(output.name, "Keep Me");
    EXPECT_FALSE(error.empty());
}
