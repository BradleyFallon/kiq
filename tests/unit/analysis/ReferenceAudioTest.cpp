#include "audio_engine/analysis/ReferenceAudio.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace KickDrum {
namespace {

constexpr double kPi = 3.14159265358979323846;

void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
}

void appendU24(std::vector<std::uint8_t>& bytes, std::int32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16u));
    bytes.push_back(static_cast<std::uint8_t>(value >> 24u));
}

void patchU32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8u);
    bytes[offset + 2] = static_cast<std::uint8_t>(value >> 16u);
    bytes[offset + 3] = static_cast<std::uint8_t>(value >> 24u);
}

std::vector<std::uint8_t> makeWav(std::uint16_t format,
                                  std::uint16_t bits,
                                  std::uint16_t channels,
                                  std::uint32_t sampleRate,
                                  const std::vector<float>& interleaved) {
    std::vector<std::uint8_t> bytes {'R', 'I', 'F', 'F', 0, 0, 0, 0,
                                     'W', 'A', 'V', 'E'};
    bytes.insert(bytes.end(), {'f', 'm', 't', ' '});
    appendU32(bytes, 16);
    appendU16(bytes, format);
    appendU16(bytes, channels);
    appendU32(bytes, sampleRate);
    const std::uint16_t blockAlignment = channels * bits / 8;
    appendU32(bytes, sampleRate * blockAlignment);
    appendU16(bytes, blockAlignment);
    appendU16(bytes, bits);
    bytes.insert(bytes.end(), {'d', 'a', 't', 'a'});
    const std::size_t dataSizeOffset = bytes.size();
    appendU32(bytes, 0);
    const std::size_t dataStart = bytes.size();

    for (float sample : interleaved) {
        sample = std::clamp(sample, -1.0f, 1.0f);
        if (format == 3) {
            std::uint32_t raw = 0;
            std::memcpy(&raw, &sample, sizeof(raw));
            appendU32(bytes, raw);
        } else if (bits == 16) {
            const auto integer = static_cast<std::int16_t>(
                std::lrint(sample * (sample < 0.0f ? 32768.0f : 32767.0f)));
            appendU16(bytes, static_cast<std::uint16_t>(integer));
        } else if (bits == 24) {
            const auto integer = static_cast<std::int32_t>(
                std::llround(sample * (sample < 0.0f ? 8388608.0 : 8388607.0)));
            appendU24(bytes, integer);
        } else {
            const auto integer = static_cast<std::int32_t>(
                std::llround(sample * (sample < 0.0f ? 2147483648.0 : 2147483647.0)));
            appendU32(bytes, static_cast<std::uint32_t>(integer));
        }
    }
    const std::size_t dataSize = bytes.size() - dataStart;
    patchU32(bytes, dataSizeOffset, static_cast<std::uint32_t>(dataSize));
    if ((dataSize & 1u) != 0) {
        bytes.push_back(0);
    }
    patchU32(bytes, 4, static_cast<std::uint32_t>(bytes.size() - 8));
    return bytes;
}

std::vector<float> makeKick(std::uint32_t sampleRate,
                            float durationSeconds,
                            float gain = 1.0f) {
    const std::size_t count = static_cast<std::size_t>(sampleRate * durationSeconds);
    std::vector<float> samples(count, 0.0f);
    double phase = 0.0;
    std::uint32_t noise = 0x12345678u;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const float time = static_cast<float>(index) / sampleRate;
        const float frequency = 56.0f + 175.0f * std::exp(-time / 0.026f);
        phase += 2.0 * kPi * frequency / sampleRate;
        const float envelope = (1.0f - std::exp(-time / 0.0007f)) *
                               std::exp(-time / 0.105f);
        const float body = envelope *
            (0.72f * std::sin(phase + 0.42) +
             0.14f * std::exp(-time / 0.034f) * std::sin(phase * 1.59334 + 0.2) +
             0.06f * std::exp(-time / 0.016f) * std::sin(phase * 2.13555 - 0.3));
        noise = noise * 1664525u + 1013904223u;
        const float white = static_cast<float>(static_cast<std::int32_t>(noise)) /
                            static_cast<float>(std::numeric_limits<std::int32_t>::max());
        const float transient = 0.24f * white * std::exp(-time / 0.0032f);
        samples[index] = gain * (body + transient);
    }
    return samples;
}

TEST(ReferenceAudioWavTest, DecodesPcm16StereoToNormalizedMono) {
    const auto bytes = makeWav(1, 16, 2, 48000,
                               {1.0f, -1.0f, 0.5f, 0.5f, -0.25f, 0.75f});
    const auto result = decodeWav(bytes);
    ASSERT_TRUE(result) << result.message;
    EXPECT_EQ(result.audio.sampleRate, 48000u);
    EXPECT_EQ(result.audio.sourceChannelCount, 2u);
    ASSERT_EQ(result.audio.monoSamples.size(), 3u);
    EXPECT_NEAR(result.audio.monoSamples[0], 0.0f, 2.0e-5f);
    EXPECT_NEAR(result.audio.monoSamples[1], 0.5f, 2.0e-5f);
    EXPECT_NEAR(result.audio.monoSamples[2], 0.25f, 2.0e-5f);
    EXPECT_NEAR(result.audio.durationSeconds(), 3.0 / 48000.0, 1.0e-10);
}

TEST(ReferenceAudioWavTest, DecodesPcm24AndPcm32) {
    for (std::uint16_t bits : {24u, 32u}) {
        const auto result = decodeWav(makeWav(1, bits, 1, 44100,
                                               {-1.0f, -0.125f, 0.0f, 0.625f, 1.0f}));
        ASSERT_TRUE(result) << bits << ": " << result.message;
        ASSERT_EQ(result.audio.monoSamples.size(), 5u);
        EXPECT_NEAR(result.audio.monoSamples[0], -1.0f, 2.0e-6f);
        EXPECT_NEAR(result.audio.monoSamples[1], -0.125f, 2.0e-6f);
        EXPECT_NEAR(result.audio.monoSamples[3], 0.625f, 2.0e-6f);
        EXPECT_NEAR(result.audio.monoSamples[4], 1.0f, 2.0e-6f);
    }
}

TEST(ReferenceAudioWavTest, DecodesFloat32) {
    const auto result = decodeWav(makeWav(3, 32, 1, 96000,
                                           {-1.0f, -0.125f, 0.0f, 0.625f, 1.0f}));
    ASSERT_TRUE(result) << result.message;
    EXPECT_EQ(result.audio.sourceEncoding, WavSampleEncoding::IeeeFloat);
    ASSERT_EQ(result.audio.monoSamples.size(), 5u);
    EXPECT_FLOAT_EQ(result.audio.monoSamples[1], -0.125f);
    EXPECT_FLOAT_EQ(result.audio.monoSamples[3], 0.625f);
}

TEST(ReferenceAudioWavTest, RejectsMalformedAndResourceLimitViolations) {
    EXPECT_EQ(decodeWav(std::vector<std::uint8_t> {'n', 'o', 'p', 'e'}).error,
              WavDecodeError::NotRiffWave);

    auto truncated = makeWav(1, 16, 1, 48000, {0.1f, 0.2f});
    truncated.pop_back();
    EXPECT_EQ(decodeWav(truncated).error, WavDecodeError::TruncatedChunk);

    const auto valid = makeWav(1, 16, 1, 48000, {0.1f, 0.2f});
    for (std::size_t byteCount = 0; byteCount < valid.size(); ++byteCount) {
        EXPECT_FALSE(decodeWav(valid.data(), byteCount));
    }
    WavDecodeLimits limits;
    limits.maximumFrames = 1;
    EXPECT_EQ(decodeWav(valid, limits).error, WavDecodeError::TooManySamples);
}

TEST(ReferenceAudioAnalysisTest, ProducesOverlayTrajectoriesSeparationAndPhysicalFit) {
    ReferenceAudio audio;
    audio.sampleRate = 12000;
    audio.sourceChannelCount = 1;
    audio.sourceBitsPerSample = 32;
    audio.sourceEncoding = WavSampleEncoding::IeeeFloat;
    audio.monoSamples = makeKick(audio.sampleRate, 0.34f, 0.9f);

    ReferenceAnalysisOptions options;
    options.waveformPointCount = 128;
    const auto result = analyzeReferenceKick(
        audio, KickRegion::samples(0, audio.monoSamples.size()), options);
    ASSERT_TRUE(result) << result.message;
    const auto& analysis = result.analysis;
    EXPECT_EQ(analysis.waveform.size(), 128u);
    EXPECT_GT(analysis.pitch.size(), 8u);
    EXPECT_GT(analysis.amplitude.size(), 8u);
    EXPECT_EQ(analysis.components.transient.size(), analysis.analyzedMonoSamples.size());
    EXPECT_EQ(analysis.components.body.size(), analysis.analyzedMonoSamples.size());
    EXPECT_GT(analysis.components.transientEndSample, 0u);
    EXPECT_GT(analysis.components.transientEnergyFraction, 0.0f);
    EXPECT_GT(analysis.components.bodyEnergyFraction, 0.0f);

    for (std::size_t index = 0; index < analysis.analyzedMonoSamples.size(); index += 37) {
        EXPECT_NEAR(analysis.components.transient[index] + analysis.components.body[index],
                    analysis.analyzedMonoSamples[index],
                    1.0e-6f);
    }
    EXPECT_GT(analysis.fit.pitchHz.front().value, analysis.fit.pitchHz.back().value);
    EXPECT_NEAR(analysis.fit.pitchHz.back().value, 56.0f, 22.0f);
    EXPECT_GE(analysis.fit.strikePosition, 0.0f);
    EXPECT_LE(analysis.fit.strikePosition, 1.0f);
    EXPECT_GE(analysis.fit.phaseAtOnsetDegrees, 0.0f);
    EXPECT_LT(analysis.fit.phaseAtOnsetDegrees, 360.0f);
    EXPECT_TRUE(std::isfinite(analysis.fit.fundamentalSinProjection));
    EXPECT_TRUE(std::isfinite(analysis.fit.fundamentalCosProjection));
    EXPECT_GT(analysis.fit.phaseConfidence, 0.02f);
    EXPECT_GT(analysis.fit.fitConfidence, 0.1f);

    ASSERT_TRUE(analysis.transientSample.has_value());
    ASSERT_FALSE(analysis.transientSample->monoSamples.empty());
    EXPECT_LE(*std::max_element(analysis.transientSample->monoSamples.begin(),
                                analysis.transientSample->monoSamples.end()),
              0.981f);
    EXPECT_NEAR(analysis.transientSample->monoSamples.back(), 0.0f, 1.0e-7f);
}

TEST(ReferenceAudioAnalysisTest, AutoSelectsTheStrongestKickFromAFullTrack) {
    ReferenceAudio audio;
    audio.sampleRate = 12000;
    audio.sourceChannelCount = 1;
    audio.monoSamples.resize(audio.sampleRate * 2, 0.0f);
    const auto quietKick = makeKick(audio.sampleRate, 0.25f, 0.22f);
    const auto loudKick = makeKick(audio.sampleRate, 0.25f, 0.88f);
    const std::size_t quietStart = static_cast<std::size_t>(0.30f * audio.sampleRate);
    const std::size_t loudStart = static_cast<std::size_t>(1.20f * audio.sampleRate);
    std::copy(quietKick.begin(), quietKick.end(), audio.monoSamples.begin() + quietStart);
    std::copy(loudKick.begin(), loudKick.end(), audio.monoSamples.begin() + loudStart);

    const auto selected = selectStrongestKickRegion(audio);
    EXPECT_NEAR(static_cast<double>(selected.startSample),
                static_cast<double>(loudStart),
                audio.sampleRate * 0.04);
    EXPECT_GT(selected.confidence, 0.1f);

    const auto result = analyzeReferenceKick(audio);
    ASSERT_TRUE(result) << result.message;
    EXPECT_NEAR(static_cast<double>(result.analysis.sourceRegion.startSample),
                static_cast<double>(loudStart),
                audio.sampleRate * 0.04);
}

TEST(ReferenceAudioAnalysisTest, RejectsSilentAndOutOfRangeSelections) {
    ReferenceAudio audio;
    audio.sampleRate = 48000;
    audio.monoSamples.resize(1024, 0.0f);
    EXPECT_EQ(analyzeReferenceKick(audio, KickRegion::samples(0)).error,
              KickAnalysisError::SilentRegion);
    EXPECT_EQ(analyzeReferenceKick(audio, KickRegion::samples(2048)).error,
              KickAnalysisError::InvalidRegion);
}

} // namespace
} // namespace KickDrum
