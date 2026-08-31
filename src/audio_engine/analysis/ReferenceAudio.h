#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace KickDrum {

enum class WavSampleEncoding {
    PcmInteger,
    IeeeFloat,
};

/** Decoded, interleaved-source audio reduced to bounded normalized mono. */
struct ReferenceAudio {
    std::uint32_t sampleRate = 0;
    std::uint16_t sourceChannelCount = 0;
    std::uint16_t sourceBitsPerSample = 0;
    WavSampleEncoding sourceEncoding = WavSampleEncoding::PcmInteger;
    std::vector<float> monoSamples;

    double durationSeconds() const;
};

enum class WavDecodeError {
    None,
    IoError,
    FileTooLarge,
    NotRiffWave,
    TruncatedChunk,
    MissingFormatChunk,
    MissingDataChunk,
    InvalidFormat,
    UnsupportedEncoding,
    TooManySamples,
};

struct WavDecodeLimits {
    std::size_t maximumFileBytes = 512u * 1024u * 1024u;
    std::size_t maximumFrames = 100u * 1024u * 1024u;
};

struct WavDecodeResult {
    ReferenceAudio audio;
    WavDecodeError error = WavDecodeError::None;
    std::string message;

    bool ok() const { return error == WavDecodeError::None; }
    explicit operator bool() const { return ok(); }
};

WavDecodeResult decodeWav(const std::uint8_t* bytes,
                          std::size_t byteCount,
                          const WavDecodeLimits& limits = {});
WavDecodeResult decodeWav(const std::vector<std::uint8_t>& bytes,
                          const WavDecodeLimits& limits = {});
WavDecodeResult loadWavFile(const std::string& path,
                            const WavDecodeLimits& limits = {});

enum class KickRegionMode {
    AutoStrongestKick,
    ExplicitSamples,
};

/** A zero sampleCount in an explicit region means "through end of file". */
struct KickRegion {
    KickRegionMode mode = KickRegionMode::AutoStrongestKick;
    std::size_t startSample = 0;
    std::size_t sampleCount = 0;

    static KickRegion strongestKick();
    static KickRegion samples(std::size_t startSample, std::size_t sampleCount = 0);
};

struct SelectedKickRegion {
    std::size_t startSample = 0;
    std::size_t sampleCount = 0;
    float confidence = 0.0f;
};

struct ReferenceAnalysisOptions {
    std::size_t waveformPointCount = 512;
    float minimumPitchHz = 30.0f;
    float maximumPitchHz = 1000.0f;
    float pitchHopMs = 5.0f;
    float pitchWindowMs = 42.0f;
    float maximumDurationMs = 2000.0f;
    bool extractTransientSample = true;
};

/** One peak-preserving point intended for a thin reference-waveform overlay. */
struct ReferenceWaveformPoint {
    float timeMs = 0.0f;
    float sample = 0.0f;
    float minimum = 0.0f;
    float maximum = 0.0f;
    float rms = 0.0f;
};

struct ReferencePitchPoint {
    float timeMs = 0.0f;
    float hertz = 0.0f;
    float confidence = 0.0f;
};

struct ReferenceAmplitudePoint {
    float timeMs = 0.0f;
    float linear = 0.0f;
    float decibels = -60.0f;
};

struct SeparatedKickComponents {
    // Both arrays are aligned to the analyzed region and sum back to it.
    std::vector<float> transient;
    std::vector<float> body;
    std::size_t transientEndSample = 0;
    float transientEnergyFraction = 0.0f;
    float bodyEnergyFraction = 0.0f;
    float transientSpectralCentroidHz = 0.0f;
};

/** A short, click-focused, click-free-at-the-end sample for a sample layer. */
struct ExtractedTransientSample {
    std::uint32_t sampleRate = 0;
    std::vector<float> monoSamples;
    float sourcePeak = 0.0f;
    float suggestedGain = 1.0f;
};

struct FittedTrajectoryPoint {
    float timeMs = 0.0f;
    float value = 0.0f;
    float curve = 0.0f;
};

/**
 * Mirrors the current KickParams model without depending on its storage type.
 * The UI/controller can copy these fields directly and sanitize through
 * sanitizeKickParams().
 */
struct FittedPhysicalKick {
    std::array<FittedTrajectoryPoint, 4> pitchHz {};
    std::array<FittedTrajectoryPoint, 4> amplitudeDb {};
    float strikePosition = 0.25f;
    float impactLevel = 0.0f;
    float airLevel = 0.0f;
    float airDecayMs = 7.0f;
    float beaterHardnessHz = 6500.0f;
    float outputGain = 0.8f;

    // Phase is expressed for x = A*sin(integratedPhase + phaseAtOnset).
    float phaseAtOnsetDegrees = 0.0f;
    float phaseReferenceTimeMs = 0.0f;
    float fundamentalSinProjection = 0.0f;
    float fundamentalCosProjection = 0.0f;
    float phaseConfidence = 0.0f;
    float fitConfidence = 0.0f;
};

enum class KickAnalysisError {
    None,
    EmptyAudio,
    InvalidSampleRate,
    InvalidRegion,
    InvalidOptions,
    SilentRegion,
    AnalysisFailed,
};

struct KickReferenceAnalysis {
    SelectedKickRegion sourceRegion;
    std::vector<float> analyzedMonoSamples;
    std::vector<ReferenceWaveformPoint> waveform;
    std::vector<ReferencePitchPoint> pitch;
    std::vector<ReferenceAmplitudePoint> amplitude;
    SeparatedKickComponents components;
    FittedPhysicalKick fit;
    std::optional<ExtractedTransientSample> transientSample;
    float sourcePeak = 0.0f;
    float activeDurationMs = 0.0f;
};

struct KickAnalysisResult {
    KickReferenceAnalysis analysis;
    KickAnalysisError error = KickAnalysisError::None;
    std::string message;

    bool ok() const { return error == KickAnalysisError::None; }
    explicit operator bool() const { return ok(); }
};

/** Finds a likely low-frequency percussive onset in full-track material. */
SelectedKickRegion selectStrongestKickRegion(
    const ReferenceAudio& audio,
    const ReferenceAnalysisOptions& options = {});

/**
 * Analyzes a selected kick or, by default, auto-selects the strongest likely
 * kick from a full-track WAV. All trajectory times are relative to onset.
 */
KickAnalysisResult analyzeReferenceKick(
    const ReferenceAudio& audio,
    KickRegion region = KickRegion::strongestKick(),
    const ReferenceAnalysisOptions& options = {});

} // namespace KickDrum
