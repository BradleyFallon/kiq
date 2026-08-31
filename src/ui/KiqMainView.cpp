#include "KiqMainView.h"

#include "KickPresetIO.h"
#include "KickWavExporter.h"
#include "KiqFactoryPresets.h"
#include "Trajectory.h"

#include "vstgui/lib/cdropsource.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <limits>
#include <string>
#include <thread>

namespace KickDrum::UI {
namespace {

using namespace VSTGUI;

constexpr CColor kWindow {10, 12, 13, 255};
constexpr CColor kPanel {16, 19, 20, 255};
constexpr CColor kPanelEdge {48, 51, 51, 255};
constexpr CColor kPanelInner {4, 6, 7, 255};
constexpr CColor kText {231, 227, 211, 255};
constexpr CColor kMutedText {151, 154, 148, 255};
constexpr CColor kGrid {52, 56, 56, 115};
constexpr CColor kPitch {249, 158, 13, 255};
constexpr CColor kPitchFill {105, 65, 8, 100};
constexpr CColor kAmplitude {35, 214, 224, 255};
constexpr CColor kAmplitudeFill {5, 92, 99, 105};
constexpr CColor kReference {179, 130, 224, 255};
constexpr CColor kRed {229, 62, 55, 255};
constexpr CColor kKnobFace {24, 27, 28, 255};
constexpr CColor kKnobEdge {76, 80, 79, 255};

constexpr double kPi = 3.14159265358979323846;

std::size_t parameterIndex(KickParameterId id) {
    return static_cast<std::size_t>(id);
}

float clampPlain(KickParameterId id, float plainValue) {
    const auto* spec = findKickParameterSpec(id);
    if (!spec) {
        return plainValue;
    }
    if (!std::isfinite(plainValue)) {
        return getDefaultKickParameter(id);
    }
    return std::clamp(plainValue, spec->minimum, spec->maximum);
}

float normalizePlain(KickParameterId id, float plainValue) {
    const auto* spec = findKickParameterSpec(id);
    if (!spec || spec->maximum <= spec->minimum || !std::isfinite(plainValue)) {
        return 0.0f;
    }
    if (id == KickParameterId::BeaterHardnessHz || id == KickParameterId::AirDecayMs) {
        const float clamped = std::clamp(plainValue, spec->minimum, spec->maximum);
        return (std::log(clamped) - std::log(spec->minimum)) /
               (std::log(spec->maximum) - std::log(spec->minimum));
    }
    if (id == KickParameterId::OutputGain) {
        if (plainValue <= 0.0f) {
            return 0.0f;
        }
        constexpr float minimumDb = -60.0f;
        const float db = std::clamp(20.0f * std::log10(plainValue), minimumDb, 0.0f);
        return (db - minimumDb) / -minimumDb;
    }
    return std::clamp((plainValue - spec->minimum) / (spec->maximum - spec->minimum),
                      0.0f, 1.0f);
}

float denormalizePlain(KickParameterId id, float normalizedValue) {
    const auto* spec = findKickParameterSpec(id);
    if (!spec) {
        return normalizedValue;
    }
    const float normalized = std::clamp(normalizedValue, 0.0f, 1.0f);
    if (id == KickParameterId::BeaterHardnessHz || id == KickParameterId::AirDecayMs) {
        return std::exp(std::log(spec->minimum) +
                        normalized * (std::log(spec->maximum) -
                                      std::log(spec->minimum)));
    }
    if (id == KickParameterId::OutputGain) {
        if (normalized <= 0.0f) {
            return 0.0f;
        }
        constexpr float minimumDb = -60.0f;
        return std::pow(10.0f, (minimumDb + normalized * -minimumDb) / 20.0f);
    }
    return spec->minimum + normalized * (spec->maximum - spec->minimum);
}

float distanceSquared(const CPoint& a, const CPoint& b) {
    const float dx = static_cast<float>(a.x - b.x);
    const float dy = static_cast<float>(a.y - b.y);
    return dx * dx + dy * dy;
}

void fillRoundedRect(CDrawContext& context, const CRect& rect, double radius,
                     const CColor& fill, const CColor& stroke, double strokeWidth = 1.0) {
    auto path = owned(context.createRoundRectGraphicsPath(rect, radius));
    if (!path) {
        return;
    }
    context.setFillColor(fill);
    context.drawGraphicsPath(path, CDrawContext::kPathFilled);
    context.setFrameColor(stroke);
    context.setLineWidth(strokeWidth);
    context.drawGraphicsPath(path, CDrawContext::kPathStroked);
}

void drawText(CDrawContext& context, const CFontRef& font, double size,
              const CColor& color, const char* text, const CRect& rect,
              CHoriTxtAlign alignment = kLeftText, int32_t style = kNormalFace) {
    context.setFont(font, size, style);
    context.setFontColor(color);
    context.drawString(text, rect, alignment, true);
}

const char* formatTime(float milliseconds, char (&buffer)[32]) {
    if (milliseconds < 10.0f) {
        std::snprintf(buffer, sizeof(buffer), "%.1f ms", milliseconds);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f ms", milliseconds);
    }
    return buffer;
}

const char* formatFrequency(float hz, char (&buffer)[32]) {
    if (hz >= 1000.0f) {
        std::snprintf(buffer, sizeof(buffer), "%.1f kHz", hz / 1000.0f);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.0f Hz", hz);
    }
    return buffer;
}

const char* formatParameter(KickParameterId id, float plainValue, char (&buffer)[32]) {
    switch (id) {
        case KickParameterId::StrikePosition:
        case KickParameterId::MembraneLevel:
        case KickParameterId::ImpactLevel:
        case KickParameterId::AirLevel:
        case KickParameterId::SampleLevel:
        case KickParameterId::Saturation:
            std::snprintf(buffer, sizeof(buffer), "%.0f %%", plainValue * 100.0f);
            break;
        case KickParameterId::AirDecayMs:
            std::snprintf(buffer, sizeof(buffer), "%.1f ms", plainValue);
            break;
        case KickParameterId::BeaterHardnessHz:
            formatFrequency(plainValue, buffer);
            break;
        case KickParameterId::OutputGain:
            if (plainValue <= 0.00001f) {
                std::snprintf(buffer, sizeof(buffer), "-inf dB");
            } else {
                std::snprintf(buffer, sizeof(buffer), "%.1f dB",
                              20.0f * std::log10(plainValue));
            }
            break;
        case KickParameterId::PhaseDegrees:
            std::snprintf(buffer, sizeof(buffer), "%+.0f deg", plainValue);
            break;
        case KickParameterId::PhaseLockMs:
            if (plainValue < 0.0f) {
                std::snprintf(buffer, sizeof(buffer), "OFF");
            } else {
                std::snprintf(buffer, sizeof(buffer), "%.1f ms", plainValue);
            }
            break;
        case KickParameterId::EqLowDb:
        case KickParameterId::EqMidDb:
        case KickParameterId::EqHighDb:
        case KickParameterId::LimiterCeilingDb:
            std::snprintf(buffer, sizeof(buffer), "%+.1f dB", plainValue);
            break;
        default:
            std::snprintf(buffer, sizeof(buffer), "%.2f", plainValue);
            break;
    }
    return buffer;
}

CRect presetButtonRect() { return {22.0, 20.0, 232.0, 49.0}; }
CRect undoButtonRect() { return {242.0, 20.0, 278.0, 49.0}; }
CRect redoButtonRect() { return {284.0, 20.0, 320.0, 49.0}; }
CRect importButtonRect() { return {700.0, 20.0, 782.0, 49.0}; }
CRect fitButtonRect() { return {790.0, 20.0, 844.0, 49.0}; }
CRect alignButtonRect() { return {852.0, 20.0, 920.0, 49.0}; }
CRect exportButtonRect() { return {928.0, 20.0, 1078.0, 49.0}; }
CRect modelTabRect() { return {28.0, 695.0, 91.0, 716.0}; }
CRect outputTabRect() { return {96.0, 695.0, 168.0, 716.0}; }
CRect phaseLockButtonRect() { return {939.0, 87.0, 1027.0, 109.0}; }

std::string withExtension(std::string path, const char* extension) {
    if (!path.empty() && std::filesystem::path(path).extension().empty()) {
        path += extension;
    }
    return path;
}

std::string safeSaveStem(const std::string& name) {
    std::string stem;
    stem.reserve(name.size());
    for (const unsigned char character : name) {
        const bool accepted = (character >= 'a' && character <= 'z') ||
                              (character >= 'A' && character <= 'Z') ||
                              (character >= '0' && character <= '9') ||
                              character == '-' || character == '_';
        if (accepted) {
            stem.push_back(static_cast<char>(character));
        } else if (!stem.empty() && stem.back() != '-') {
            stem.push_back('-');
        }
    }
    while (!stem.empty() && stem.back() == '-') {
        stem.pop_back();
    }
    return stem.empty() ? "Kiq-Kick" : stem;
}

void cleanupOldTemporaryExports() {
    constexpr const char* prefix = "Kiq-Kick-";
    constexpr const char* extension = ".wav";
    std::error_code error;
    const auto directory = std::filesystem::temp_directory_path(error);
    if (error) {
        return;
    }
    const auto now = std::filesystem::file_time_type::clock::now();
    for (std::filesystem::directory_iterator iterator(directory, error), end;
         !error && iterator != end; iterator.increment(error)) {
        const auto filename = iterator->path().filename().string();
        if (filename.rfind(prefix, 0) != 0 ||
            iterator->path().extension() != extension) {
            continue;
        }
        const auto modified = iterator->last_write_time(error);
        if (!error && now - modified > std::chrono::hours(24 * 30)) {
            std::filesystem::remove(iterator->path(), error);
        }
        error.clear();
    }
}

} // namespace

KiqMainView::KiqMainView(const CRect& size, KiqUIBridge& bridge)
    : CView(size)
    , bridge_(bridge)
    , callbackState_(std::make_shared<AsyncCallbackState>())
    , titleFont_(makeOwned<CFontDesc>("Helvetica Neue", 40.0, kBoldFace))
    , sectionFont_(makeOwned<CFontDesc>("Helvetica Neue", 17.0, kBoldFace))
    , labelFont_(makeOwned<CFontDesc>("Helvetica Neue", 11.0, kBoldFace))
    , valueFont_(makeOwned<CFontDesc>("Menlo", 10.5, kNormalFace)) {
    callbackState_->view.store(this, std::memory_order_release);
    setTransparency(false);
    setMouseEnabled(true);
    setWantsFocus(true);
    syncFromBridge();
    sampleLayer_ = bridge_.getSampleLayer();
    bool isDefaultSound = !sampleLayer_;
    for (const auto& spec : kKickParameterSpecs) {
        isDefaultSound = isDefaultSound &&
            std::abs(value(spec.id) - getDefaultKickParameter(spec.id)) <= 1.0e-5f;
    }
    presetName_ = isDefaultSound ? "Init — Bass House" : "Host State";
    history_.reset(currentParams());
    rebuildWaveformPreview();
    timer_ = makeOwned<CVSTGUITimer>([this](CVSTGUITimer*) { timerTick(); }, 33);
}

KiqMainView::~KiqMainView() noexcept {
    callbackState_->view.store(nullptr, std::memory_order_release);
    if (timer_) {
        timer_->stop();
    }
    // The worker executes analysis code from this module, so it must finish
    // before a host can unload the editor bundle.
    if (referenceWorker_.joinable()) {
        referenceWorker_.join();
    }
    bridge_.setAuditionLoop(false, loopBpm_);
}

float KiqMainView::value(KickParameterId id) const {
    return values_[parameterIndex(id)];
}

void KiqMainView::setValue(KickParameterId id, float plainValue) {
    const float clamped = clampPlain(id, plainValue);
    if (std::abs(value(id) - clamped) <= 1.0e-6f) {
        return;
    }
    values_[parameterIndex(id)] = clamped;
    presetName_ = "Edited";
    waveformDirty_ = true;
    bridge_.performParameterEdit(id, clamped);
    invalid();
}

KickParams KiqMainView::currentParams() const {
    KickParams params = kDefaultKickParams;
    for (const auto& spec : kKickParameterSpecs) {
        setKickParameter(params, spec.id, value(spec.id));
    }
    return sanitizeKickParams(params);
}

void KiqMainView::applyParams(const KickParams& requestedParams,
                              bool recordHistory) {
    const KickParams params = sanitizeKickParams(requestedParams);
    bool changed = false;
    for (const auto& spec : kKickParameterSpecs) {
        const float next = getKickParameter(params, spec.id);
        if (std::abs(value(spec.id) - next) <= 1.0e-6f) {
            continue;
        }
        bridge_.beginParameterEdit(spec.id);
        values_[parameterIndex(spec.id)] = next;
        bridge_.performParameterEdit(spec.id, next);
        bridge_.endParameterEdit(spec.id);
        changed = true;
    }
    if (!changed) {
        return;
    }
    waveformDirty_ = true;
    if (recordHistory) {
        history_.record(params);
    }
    invalid();
}

void KiqMainView::recordCurrentState() {
    history_.record(currentParams());
}

void KiqMainView::undo() {
    KickParams restored;
    if (history_.undo(restored)) {
        applyParams(restored, false);
        presetName_ = "Edited";
        setStatus("Undo");
    }
}

void KiqMainView::redo() {
    KickParams restored;
    if (history_.redo(restored)) {
        applyParams(restored, false);
        presetName_ = "Edited";
        setStatus("Redo");
    }
}

void KiqMainView::syncFromBridge() {
    bool changed = false;
    bool parametersChanged = false;
    for (const auto& spec : kKickParameterSpecs) {
        const auto index = parameterIndex(spec.id);
        const float next = clampPlain(spec.id, bridge_.getParameter(spec.id));
        if (std::abs(values_[index] - next) > 1.0e-5f) {
            values_[index] = next;
            changed = true;
            parametersChanged = true;
        }
    }

    if (parametersChanged) {
        presetName_ = "Host State";
        waveformDirty_ = true;
        if (drag_.kind == DragKind::None) {
            const KickParams snapshot = currentParams();
            bool matchesHistory = true;
            for (const auto& spec : kKickParameterSpecs) {
                if (std::abs(getKickParameter(snapshot, spec.id) -
                             getKickParameter(history_.current(), spec.id)) > 1.0e-5f) {
                    matchesHistory = false;
                    break;
                }
            }
            if (!matchesHistory) {
                history_.reset(snapshot);
            }
        }
    }

    const auto bridgeSampleLayer = bridge_.getSampleLayer();
    if (bridgeSampleLayer.get() != sampleLayer_.get()) {
        const bool sameAudio = bridgeSampleLayer && sampleLayer_ &&
            bridgeSampleLayer->sourceSampleRate == sampleLayer_->sourceSampleRate &&
            bridgeSampleLayer->samples == sampleLayer_->samples;
        sampleLayer_ = bridgeSampleLayer;
        if (!sameAudio) {
            sampleSourcePath_.clear();
            referenceAnalysis_.reset();
            presetName_ = "Host State";
        }
        // Samples are not part of KickParamsHistory. A bridge-side sample
        // replacement therefore establishes a new undo baseline even when
        // its bytes happen to match the previous allocation.
        history_.reset(currentParams());
        waveformDirty_ = true;
        changed = true;
    }

    const float targetPeak = std::clamp(bridge_.getOutputPeak(), 0.0f, 1.0f);
    const float nextPeak = targetPeak >= displayedPeak_
                               ? targetPeak
                               : std::max(targetPeak, displayedPeak_ * 0.88f - 0.004f);
    if (std::abs(nextPeak - displayedPeak_) > 1.0e-4f) {
        displayedPeak_ = nextPeak;
        changed = true;
    }

    if (bridge_.getOutputClip()) {
        clipHoldFrames_ = 45;
        changed = true;
    } else if (clipHoldFrames_ > 0) {
        --clipHoldFrames_;
        changed = true;
    }

    if (changed) {
        invalid();
    }
}

void KiqMainView::timerTick() {
    syncFromBridge();
    consumeReferenceImport();
    if (statusFrames_ > 0 && --statusFrames_ == 0) {
        statusMessage_.clear();
        invalid();
    }
    // Long (up to two-second) previews are intentionally rebuilt after a drag
    // settles, keeping trajectory and knob interaction responsive.
    if (waveformDirty_ && drag_.kind == DragKind::None) {
        rebuildWaveformPreview();
        invalid();
    }
}

void KiqMainView::rebuildWaveformPreview() {
    constexpr std::uint32_t previewSampleRate = 48000;
    const KickParams params = currentParams();
    KickRenderSettings settings;
    settings.sampleRate = previewSampleRate;
    settings.sampleLayer = sampleLayer_.get();
    const std::vector<float> rendered =
        KickWavExporter::renderMono(params, settings);
    const std::size_t sampleCount = std::max<std::size_t>(2, rendered.size());
    waveformDurationMs_ = static_cast<float>(sampleCount) * 1000.0f /
                          static_cast<float>(previewSampleRate);
    waveformBinsUsed_ = std::min(kWaveformBinCount, sampleCount);
    std::fill(waveformSamples_.begin(), waveformSamples_.end(), 0.0f);
    std::fill(tuningSamplesHz_.begin(), tuningSamplesHz_.end(), 0.0f);
    waveformPeak_ = 0.0f;

    for (std::size_t sample = 0; sample < sampleCount; ++sample) {
        const float renderedSample = sample < rendered.size() ? rendered[sample] : 0.0f;
        waveformPeak_ = std::max(waveformPeak_, std::abs(renderedSample));
        const std::size_t bin = std::min(
            waveformBinsUsed_ - 1, sample * waveformBinsUsed_ / sampleCount);
        // Keep the strongest sample in each pixel-width time bucket. This
        // preserves the short impact and body peaks while still drawing a
        // single, unfilled waveform line.
        if (std::abs(renderedSample) > std::abs(waveformSamples_[bin])) {
            waveformSamples_[bin] = renderedSample;
        }
    }

    Trajectory tuningTrajectory(Trajectory::Scale::Logarithmic);
    tuningTrajectory.setPoints(params.pitch);
    for (std::size_t bin = 0; bin < waveformBinsUsed_; ++bin) {
        const float timeMs = waveformDurationMs_ * static_cast<float>(bin) /
                             static_cast<float>(waveformBinsUsed_ - 1);
        tuningSamplesHz_[bin] = tuningTrajectory.valueAt(timeMs);
    }
    waveformDirty_ = false;
}

void KiqMainView::setLoopBpm(float bpm) {
    const float next = std::clamp(bpm, 40.0f, 240.0f);
    if (std::abs(loopBpm_ - next) < 0.01f) {
        return;
    }
    loopBpm_ = next;
    bridge_.setAuditionLoop(loopEnabled_, loopBpm_);
    invalid();
}

void KiqMainView::setLoopEnabled(bool enabled) {
    if (loopEnabled_ == enabled) {
        return;
    }
    loopEnabled_ = enabled;
    bridge_.setAuditionLoop(loopEnabled_, loopBpm_);
    invalid();
}

void KiqMainView::draw(CDrawContext* context) {
    if (!context) {
        return;
    }
    context->setDrawMode(kAntiAliasing);
    drawBackground(*context);
    drawHeader(*context);
    drawTrajectory(*context, TrajectoryKind::Pitch);
    drawTrajectory(*context, TrajectoryKind::Amplitude);
    drawWaveformPreview(*context);
    drawControls(*context);
    setDirty(false);
}

void KiqMainView::drawBackground(CDrawContext& context) {
    const CRect bounds = getViewSize();
    context.setFillColor(kWindow);
    context.drawRect(bounds, kDrawFilled);

    CRect inset = bounds;
    inset.inset(3.0, 3.0);
    fillRoundedRect(context, inset, 18.0, kWindow, CColor(89, 92, 89, 255), 1.5);

    context.setFrameColor(CColor(255, 255, 255, 10));
    context.setLineWidth(1.0);
    for (int y = 6; y < static_cast<int>(kDesignHeight); y += 4) {
        context.drawLine(CPoint(8.0, y), CPoint(kDesignWidth - 8.0, y));
    }
    if (dropHover_) {
        CRect target = bounds;
        target.inset(9.0, 9.0);
        fillRoundedRect(context, target, 15.0, CColor(20, 45, 47, 45),
                        kAmplitude, 2.0);
    }
}

void KiqMainView::drawHeader(CDrawContext& context) {
    drawText(context, titleFont_, 32.0, kText, "K I Q",
             CRect(330.0, 4.0, 690.0, 42.0), kCenterText, kBoldFace);
    drawText(context, labelFont_, 9.5, kText, "P H Y S I C A L   K I C K   D E S I G N E R",
             CRect(320.0, 39.0, 700.0, 55.0), kCenterText, kBoldFace);

    std::string presetLabel = "PRESET  " + presetName_;
    if (presetLabel.size() > 27) {
        presetLabel.resize(26);
        presetLabel += "…";
    }
    drawWorkflowButton(context, presetButtonRect(), presetLabel.c_str());
    drawWorkflowButton(context, undoButtonRect(), "<", history_.canUndo());
    drawWorkflowButton(context, redoButtonRect(), ">", history_.canRedo());
    drawWorkflowButton(context, importButtonRect(), "IMPORT");
    drawWorkflowButton(context, fitButtonRect(), "FIT", referenceAnalysis_.has_value());
    drawWorkflowButton(context, alignButtonRect(), "ALIGN", referenceAnalysis_.has_value());
    drawWorkflowButton(context, exportButtonRect(), "EXPORT / DRAG", true, true);

    const char* status = statusMessage_.empty()
                             ? "DROP A WAV ANYWHERE TO MATCH IT"
                             : statusMessage_.c_str();
    drawText(context, valueFont_, 8.5,
             statusMessage_.empty() ? kMutedText : kAmplitude,
             status, CRect(300.0, 57.0, 800.0, 70.0), kCenterText);
}

void KiqMainView::drawWorkflowButton(CDrawContext& context,
                                     const CRect& rect, const char* label,
                                     bool active, bool accent) {
    const CColor edge = !active ? CColor(45, 48, 48, 255)
                                : (accent ? kAmplitude : CColor(78, 82, 81, 255));
    const CColor text = !active ? CColor(82, 85, 83, 255)
                                : (accent ? kAmplitude : kText);
    fillRoundedRect(context, rect, 5.0, CColor(14, 17, 18, 255), edge,
                    accent ? 1.6 : 1.0);
    drawText(context, labelFont_, 8.6, text, label, rect, kCenterText, kBoldFace);
}

CRect KiqMainView::trajectoryPanel(TrajectoryKind kind) const {
    return kind == TrajectoryKind::Pitch
               ? CRect(14.0, 78.0, kDesignWidth - 14.0, 302.0)
               : CRect(14.0, 310.0, kDesignWidth - 14.0, 520.0);
}

CRect KiqMainView::trajectoryGraph(TrajectoryKind kind) const {
    return kind == TrajectoryKind::Pitch
               ? CRect(78.0, 119.0, kDesignWidth - 38.0, 258.0)
               : CRect(78.0, 351.0, kDesignWidth - 38.0, 480.0);
}

float KiqMainView::trajectoryTimeMax(TrajectoryKind kind) const {
    const float lastTime = kind == TrajectoryKind::Pitch
                               ? value(KickParameterId::Pitch3TimeMs)
                               : value(KickParameterId::Amp3TimeMs);
    const float minimum = kind == TrajectoryKind::Pitch ? 240.0f : 280.0f;
    return std::max(minimum, lastTime * 1.18f);
}

CPoint KiqMainView::trajectoryPoint(TrajectoryKind kind, std::size_t index,
                                    float timeMax) const {
    static constexpr std::array<KickParameterId, 4> pitchValueIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1Hz,
        KickParameterId::Pitch2Hz, KickParameterId::Pitch3Hz,
    };
    static constexpr std::array<KickParameterId, 4> pitchTimeIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1TimeMs,
        KickParameterId::Pitch2TimeMs, KickParameterId::Pitch3TimeMs,
    };
    static constexpr std::array<KickParameterId, 4> ampValueIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1Db,
        KickParameterId::Amp2Db, KickParameterId::Amp3Db,
    };
    static constexpr std::array<KickParameterId, 4> ampTimeIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1TimeMs,
        KickParameterId::Amp2TimeMs, KickParameterId::Amp3TimeMs,
    };

    const CRect graph = trajectoryGraph(kind);
    const float maximumTime = timeMax > 0.0f ? timeMax : trajectoryTimeMax(kind);
    const float time = index == 0
                           ? 0.0f
                           : value(kind == TrajectoryKind::Pitch ? pitchTimeIds[index]
                                                                  : ampTimeIds[index]);
    const float plain = value(kind == TrajectoryKind::Pitch ? pitchValueIds[index]
                                                             : ampValueIds[index]);
    const float x = static_cast<float>(graph.left + graph.getWidth() *
                                                      std::clamp(time / maximumTime, 0.0f, 1.0f));

    float normalizedY = 0.0f;
    if (kind == TrajectoryKind::Pitch) {
        constexpr float minimumHz = 20.0f;
        constexpr float maximumHz = 1000.0f;
        normalizedY = (std::log(std::max(plain, minimumHz)) - std::log(minimumHz)) /
                      (std::log(maximumHz) - std::log(minimumHz));
    } else {
        normalizedY = (plain + 60.0f) / 66.0f;
    }
    const float y = static_cast<float>(graph.bottom -
                                       graph.getHeight() * std::clamp(normalizedY, 0.0f, 1.0f));
    return CPoint(x, y);
}

CPoint KiqMainView::curveHandlePoint(TrajectoryKind kind, std::size_t segment,
                                     float timeMax) const {
    static constexpr std::array<KickParameterId, 3> pitchCurveIds {
        KickParameterId::PitchCurve1, KickParameterId::PitchCurve2,
        KickParameterId::PitchCurve3,
    };
    static constexpr std::array<KickParameterId, 3> ampCurveIds {
        KickParameterId::AmpCurve1, KickParameterId::AmpCurve2,
        KickParameterId::AmpCurve3,
    };

    const CPoint start = trajectoryPoint(kind, segment, timeMax);
    const CPoint end = trajectoryPoint(kind, segment + 1, timeMax);
    const float curve = value(kind == TrajectoryKind::Pitch ? pitchCurveIds[segment]
                                                             : ampCurveIds[segment]);
    const float shaped = Trajectory::shape(0.5f, curve);
    const double x = (start.x + end.x) * 0.5;
    double y = 0.0;
    if (kind == TrajectoryKind::Pitch) {
        const auto pitchId = [segment](bool ending) {
            static constexpr std::array<KickParameterId, 4> ids {
                KickParameterId::Pitch0Hz, KickParameterId::Pitch1Hz,
                KickParameterId::Pitch2Hz, KickParameterId::Pitch3Hz,
            };
            return ids[segment + (ending ? 1 : 0)];
        };
        const float a = std::max(value(pitchId(false)), 0.001f);
        const float b = std::max(value(pitchId(true)), 0.001f);
        const float midpointValue = std::exp(std::log(a) +
                                             (std::log(b) - std::log(a)) * shaped);
        constexpr float minHz = 20.0f;
        constexpr float maxHz = 1000.0f;
        const float normalized = (std::log(midpointValue) - std::log(minHz)) /
                                 (std::log(maxHz) - std::log(minHz));
        const CRect graph = trajectoryGraph(kind);
        y = graph.bottom - graph.getHeight() * std::clamp(normalized, 0.0f, 1.0f);
    } else {
        static constexpr std::array<KickParameterId, 4> ids {
            KickParameterId::Amp0Db, KickParameterId::Amp1Db,
            KickParameterId::Amp2Db, KickParameterId::Amp3Db,
        };
        const float db = value(ids[segment]) +
                         (value(ids[segment + 1]) - value(ids[segment])) * shaped;
        const CRect graph = trajectoryGraph(kind);
        y = graph.bottom - graph.getHeight() * std::clamp((db + 60.0f) / 66.0f, 0.0f, 1.0f);
    }
    return CPoint(x, y);
}

void KiqMainView::drawTrajectory(CDrawContext& context, TrajectoryKind kind) {
    const bool pitch = kind == TrajectoryKind::Pitch;
    const CColor accent = pitch ? kPitch : kAmplitude;
    const CColor fill = pitch ? kPitchFill : kAmplitudeFill;
    const CRect panel = trajectoryPanel(kind);
    const CRect graph = trajectoryGraph(kind);

    fillRoundedRect(context, panel, 12.0, kPanel, kPanelEdge, 1.2);
    CRect inner = panel;
    inner.inset(4.0, 4.0);
    fillRoundedRect(context, inner, 9.0, CColor(13, 16, 17, 255), kPanelInner, 1.0);

    drawText(context, sectionFont_, 17.0, accent,
             pitch ? "MEMBRANE TENSION" : "ENERGY DECAY",
             CRect(42.0, panel.top + 10.0, 290.0, panel.top + 34.0),
             kLeftText, kBoldFace);
    if (pitch) {
        const bool locked = value(KickParameterId::PhaseLockMs) >= 0.0f;
        drawWorkflowButton(context, phaseLockButtonRect(),
                           locked ? "PHASE LOCK  ON" : "PHASE LOCK  OFF",
                           true, locked);
    }

    context.setFrameColor(kGrid);
    context.setLineWidth(1.0);
    for (int line = 0; line <= 12; ++line) {
        const double x = graph.left + graph.getWidth() * line / 12.0;
        context.drawLine(CPoint(x, graph.top), CPoint(x, graph.bottom));
    }
    for (int line = 0; line <= 6; ++line) {
        const double y = graph.top + graph.getHeight() * line / 6.0;
        context.drawLine(CPoint(graph.left, y), CPoint(graph.right, y));
    }

    if (pitch && value(KickParameterId::PhaseLockMs) >= 0.0f) {
        const float lockMs = value(KickParameterId::PhaseLockMs);
        const double x = graph.left + graph.getWidth() *
            std::clamp(lockMs / trajectoryTimeMax(kind), 0.0f, 1.0f);
        context.setFrameColor(kReference);
        context.setLineWidth(1.5);
        context.drawLine(CPoint(x, graph.top), CPoint(x, graph.bottom));
        context.setFillColor(kReference);
        context.drawEllipse(CRect(x - 4.0, graph.top - 4.0,
                                  x + 4.0, graph.top + 4.0), kDrawFilled);
        char lockBuffer[32] {};
        formatTime(lockMs, lockBuffer);
        drawText(context, valueFont_, 8.2, kReference, lockBuffer,
                 CRect(x - 36.0, graph.top + 3.0, x + 36.0, graph.top + 17.0),
                 kCenterText);
    }

    char scaleBuffer[32] {};
    if (pitch) {
        drawText(context, valueFont_, 10.0, accent, "1 kHz",
                 CRect(25.0, graph.top - 7.0, graph.left - 8.0, graph.top + 11.0), kRightText);
        drawText(context, valueFont_, 10.0, accent, "20 Hz",
                 CRect(25.0, graph.bottom - 10.0, graph.left - 8.0, graph.bottom + 8.0), kRightText);
    } else {
        drawText(context, valueFont_, 10.0, accent, "+6 dB",
                 CRect(25.0, graph.top - 7.0, graph.left - 8.0, graph.top + 11.0), kRightText);
        drawText(context, valueFont_, 10.0, accent, "-60 dB",
                 CRect(22.0, graph.bottom - 10.0, graph.left - 8.0, graph.bottom + 8.0), kRightText);
    }
    const float timeMax = trajectoryTimeMax(kind);
    formatTime(timeMax, scaleBuffer);
    drawText(context, valueFont_, 9.5, kMutedText, "0 ms",
             CRect(graph.left, graph.bottom + 5.0, graph.left + 60.0, graph.bottom + 23.0), kLeftText);
    drawText(context, valueFont_, 9.5, kMutedText, scaleBuffer,
             CRect(graph.right - 90.0, graph.bottom + 5.0, graph.right, graph.bottom + 23.0), kRightText);

    auto linePath = owned(context.createGraphicsPath());
    auto fillPath = owned(context.createGraphicsPath());
    if (linePath && fillPath) {
        const CPoint first = trajectoryPoint(kind, 0, timeMax);
        linePath->beginSubpath(first);
        fillPath->beginSubpath(CPoint(first.x, graph.bottom));
        fillPath->addLine(first);

        for (std::size_t segment = 0; segment < 3; ++segment) {
            const CPoint start = trajectoryPoint(kind, segment, timeMax);
            const CPoint end = trajectoryPoint(kind, segment + 1, timeMax);
            const KickParameterId curveId = pitch
                                                 ? static_cast<KickParameterId>(
                                                       parameterIndex(KickParameterId::PitchCurve1) + segment)
                                                 : static_cast<KickParameterId>(
                                                       parameterIndex(KickParameterId::AmpCurve1) + segment);
            const float curve = value(curveId);
            for (int sample = 1; sample <= 28; ++sample) {
                const float u = static_cast<float>(sample) / 28.0f;
                const float shaped = Trajectory::shape(u, curve);
                const CPoint point(start.x + (end.x - start.x) * u,
                                   start.y + (end.y - start.y) * shaped);
                linePath->addLine(point);
                fillPath->addLine(point);
            }
        }
        const CPoint last = trajectoryPoint(kind, 3, timeMax);
        fillPath->addLine(CPoint(last.x, graph.bottom));
        fillPath->closeSubpath();
        context.setFillColor(fill);
        context.drawGraphicsPath(fillPath, CDrawContext::kPathFilled);
        context.setFrameColor(accent);
        context.setLineWidth(2.3);
        context.drawGraphicsPath(linePath, CDrawContext::kPathStroked);
    }

    static constexpr std::array<KickParameterId, 4> pitchValueIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1Hz,
        KickParameterId::Pitch2Hz, KickParameterId::Pitch3Hz,
    };
    static constexpr std::array<KickParameterId, 4> pitchTimeIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1TimeMs,
        KickParameterId::Pitch2TimeMs, KickParameterId::Pitch3TimeMs,
    };
    static constexpr std::array<KickParameterId, 4> ampValueIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1Db,
        KickParameterId::Amp2Db, KickParameterId::Amp3Db,
    };
    static constexpr std::array<KickParameterId, 4> ampTimeIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1TimeMs,
        KickParameterId::Amp2TimeMs, KickParameterId::Amp3TimeMs,
    };

    for (std::size_t index = 0; index < 4; ++index) {
        const CPoint point = trajectoryPoint(kind, index, timeMax);
        CRect marker(point.x - 7.0, point.y - 7.0, point.x + 7.0, point.y + 7.0);
        context.setFillColor(kPanel);
        context.setFrameColor(accent);
        context.setLineWidth(2.0);
        context.drawEllipse(marker, kDrawFilledAndStroked);

        char number[4] {};
        std::snprintf(number, sizeof(number), "%zu", index + 1);
        drawText(context, labelFont_, 9.0, kText, number,
                 CRect(point.x - 7.0, point.y - 6.0, point.x + 7.0, point.y + 7.0),
                 kCenterText, kBoldFace);

        const KickParameterId valueId = pitch ? pitchValueIds[index] : ampValueIds[index];
        char valueBuffer[32] {};
        if (pitch) {
            formatFrequency(value(valueId), valueBuffer);
        } else {
            std::snprintf(valueBuffer, sizeof(valueBuffer), "%.0f dB", value(valueId));
        }
        drawText(context, valueFont_, 9.0, kText, valueBuffer,
                 CRect(point.x - 40.0, point.y - 27.0, point.x + 40.0, point.y - 11.0),
                 kCenterText);

        if (index > 0) {
            const KickParameterId timeId = pitch ? pitchTimeIds[index] : ampTimeIds[index];
            char timeBuffer[32] {};
            formatTime(value(timeId), timeBuffer);
            drawText(context, valueFont_, 8.8, kMutedText, timeBuffer,
                     CRect(point.x - 38.0, point.y + 10.0, point.x + 38.0, point.y + 27.0),
                     kCenterText);
        }
    }

    for (std::size_t segment = 0; segment < 3; ++segment) {
        const CPoint point = curveHandlePoint(kind, segment, timeMax);
        CRect handle(point.x - 4.0, point.y - 4.0, point.x + 4.0, point.y + 4.0);
        context.setFillColor(accent);
        context.setFrameColor(kPanel);
        context.setLineWidth(1.0);
        context.drawEllipse(handle, kDrawFilledAndStroked);
    }
}

void KiqMainView::drawWaveformPreview(CDrawContext& context) {
    const CRect panel(14.0, 528.0, kDesignWidth - 14.0, 680.0);
    fillRoundedRect(context, panel, 12.0, kPanel, kPanelEdge, 1.2);
    CRect inner = panel;
    inner.inset(4.0, 4.0);
    fillRoundedRect(context, inner, 9.0, CColor(13, 16, 17, 255), kPanelInner, 1.0);

    drawText(context, sectionFont_, 17.0, kAmplitude, "RESPONSE",
             CRect(42.0, 538.0, 170.0, 560.0), kLeftText, kBoldFace);
    drawText(context, valueFont_, 9.0, kAmplitude, "WAVEFORM",
             CRect(174.0, 540.0, 252.0, 558.0), kLeftText, kBoldFace);
    char tuningBuffer[48] {};
    if (waveformBinsUsed_ > 1) {
        std::snprintf(tuningBuffer, sizeof(tuningBuffer), "+ TUNING  %.0f -> %.0f Hz",
                      tuningSamplesHz_.front(),
                      tuningSamplesHz_[waveformBinsUsed_ - 1]);
    } else {
        std::snprintf(tuningBuffer, sizeof(tuningBuffer), "+ TUNING");
    }
    drawText(context, valueFont_, 9.0, kPitch, tuningBuffer,
             CRect(255.0, 540.0, 460.0, 558.0), kLeftText, kBoldFace);
    if (referenceAnalysis_) {
        char referenceBuffer[48] {};
        std::snprintf(referenceBuffer, sizeof(referenceBuffer),
                      "REFERENCE  %.0f%% MATCH",
                      referenceAnalysis_->fit.fitConfidence * 100.0f);
        drawText(context, valueFont_, 9.0, kReference, referenceBuffer,
                 CRect(462.0, 540.0, 650.0, 558.0), kLeftText, kBoldFace);
    }

    const CRect graph(42.0, 562.0, 820.0, 653.0);
    fillRoundedRect(context, graph, 5.0, CColor(7, 10, 11, 255),
                    CColor(37, 42, 42, 255), 1.0);

    const CRect plot(88.0, graph.top + 3.0, graph.right - 4.0, graph.bottom - 3.0);
    const double separatorY = (plot.top + plot.bottom) * 0.5;
    const CRect waveformLane(plot.left, plot.top, plot.right, separatorY - 3.0);
    const CRect tuningLane(plot.left, separatorY + 3.0, plot.right, plot.bottom);

    context.setFrameColor(kGrid);
    context.setLineWidth(1.0);
    for (int line = 1; line < 8; ++line) {
        const double x = plot.left + plot.getWidth() * line / 8.0;
        context.drawLine(CPoint(x, plot.top), CPoint(x, plot.bottom));
    }
    context.setFrameColor(CColor(75, 82, 81, 170));
    const double waveformCenterY = (waveformLane.top + waveformLane.bottom) * 0.5;
    context.drawLine(CPoint(plot.left, waveformCenterY),
                     CPoint(plot.right, waveformCenterY));
    context.setFrameColor(CColor(55, 59, 59, 170));
    context.drawLine(CPoint(graph.left + 4.0, separatorY),
                     CPoint(graph.right - 4.0, separatorY));

    drawText(context, valueFont_, 8.0, kAmplitude, "AUDIO",
             CRect(47.0, waveformLane.top + 8.0, 83.0, waveformLane.top + 22.0),
             kLeftText, kBoldFace);
    drawText(context, valueFont_, 8.0, kPitch, "PITCH",
             CRect(47.0, tuningLane.top + 8.0, 83.0, tuningLane.top + 22.0),
             kLeftText, kBoldFace);

    const float referenceDurationMs = referenceAnalysis_
                                          ? referenceAnalysis_->activeDurationMs
                                          : 0.0f;
    const float displayDurationMs = std::max(
        1.0f, std::max(waveformDurationMs_, referenceDurationMs));

    if (referenceAnalysis_ && !referenceAnalysis_->waveform.empty()) {
        const float referenceScale = referenceAnalysis_->sourcePeak > 1.0e-6f
                                         ? 0.92f / referenceAnalysis_->sourcePeak
                                         : 1.0f;
        auto referenceWaveformPath = owned(context.createGraphicsPath());
        auto referencePitchPath = owned(context.createGraphicsPath());
        if (referenceWaveformPath) {
            bool first = true;
            for (const auto& point : referenceAnalysis_->waveform) {
                const double x = plot.left + plot.getWidth() *
                    std::clamp(point.timeMs / displayDurationMs, 0.0f, 1.0f);
                const CPoint graphPoint(
                    x, waveformCenterY - point.sample * referenceScale *
                                           waveformLane.getHeight() * 0.45);
                if (first) {
                    referenceWaveformPath->beginSubpath(graphPoint);
                    first = false;
                } else {
                    referenceWaveformPath->addLine(graphPoint);
                }
            }
            context.setFrameColor(CColor(kReference.red, kReference.green,
                                         kReference.blue, 145));
            context.setLineWidth(1.0);
            context.drawGraphicsPath(referenceWaveformPath,
                                     CDrawContext::kPathStroked);
        }
        if (referencePitchPath && !referenceAnalysis_->pitch.empty()) {
            bool first = true;
            for (const auto& point : referenceAnalysis_->pitch) {
                if (point.hertz <= 0.0f || point.confidence < 0.08f) {
                    first = true;
                    continue;
                }
                const double x = plot.left + plot.getWidth() *
                    std::clamp(point.timeMs / displayDurationMs, 0.0f, 1.0f);
                constexpr float minimumPitchHz = 20.0f;
                constexpr float maximumPitchHz = 1000.0f;
                const float pitchHz = std::clamp(
                    point.hertz, minimumPitchHz, maximumPitchHz);
                const float normalized =
                    (std::log(pitchHz) - std::log(minimumPitchHz)) /
                    (std::log(maximumPitchHz) - std::log(minimumPitchHz));
                const CPoint graphPoint(
                    x, tuningLane.bottom - normalized * tuningLane.getHeight());
                if (first) {
                    referencePitchPath->beginSubpath(graphPoint);
                    first = false;
                } else {
                    referencePitchPath->addLine(graphPoint);
                }
            }
            context.setFrameColor(CColor(kReference.red, kReference.green,
                                         kReference.blue, 190));
            context.setLineWidth(1.3);
            context.drawGraphicsPath(referencePitchPath,
                                     CDrawContext::kPathStroked);
        }
    }

    if (waveformBinsUsed_ > 1) {
        const double waveformHalfHeight = waveformLane.getHeight() * 0.45;
        auto waveformPath = owned(context.createGraphicsPath());
        auto tuningPath = owned(context.createGraphicsPath());
        if (waveformPath && tuningPath) {
            for (std::size_t bin = 0; bin < waveformBinsUsed_; ++bin) {
                const double timeMs = waveformDurationMs_ *
                    static_cast<double>(bin) /
                    static_cast<double>(waveformBinsUsed_ - 1);
                const double x = plot.left + plot.getWidth() *
                    std::clamp(timeMs / displayDurationMs, 0.0, 1.0);
                const CPoint waveformPoint(
                    x, waveformCenterY - waveformSamples_[bin] * waveformHalfHeight);

                constexpr float minimumPitchHz = 20.0f;
                constexpr float maximumPitchHz = 1000.0f;
                const float pitchHz = std::clamp(
                    tuningSamplesHz_[bin], minimumPitchHz, maximumPitchHz);
                const float pitchNormalized =
                    (std::log(pitchHz) - std::log(minimumPitchHz)) /
                    (std::log(maximumPitchHz) - std::log(minimumPitchHz));
                const CPoint tuningPoint(
                    x, tuningLane.bottom - pitchNormalized * tuningLane.getHeight());

                if (bin == 0) {
                    waveformPath->beginSubpath(waveformPoint);
                    tuningPath->beginSubpath(tuningPoint);
                } else {
                    waveformPath->addLine(waveformPoint);
                    tuningPath->addLine(tuningPoint);
                }
            }
            context.setFrameColor(kAmplitude);
            context.setLineWidth(1.25);
            context.drawGraphicsPath(waveformPath, CDrawContext::kPathStroked);
            context.setFrameColor(kPitch);
            context.setLineWidth(1.8);
            context.drawGraphicsPath(tuningPath, CDrawContext::kPathStroked);
        }
    }

    char durationBuffer[32] {};
    formatTime(displayDurationMs, durationBuffer);
    drawText(context, valueFont_, 8.8, kMutedText, "0 ms",
             CRect(graph.left, 655.0, graph.left + 64.0, 672.0), kLeftText);
    drawText(context, valueFont_, 8.8, kMutedText, durationBuffer,
             CRect(graph.right - 80.0, 655.0, graph.right, 672.0), kRightText);

    char peakBuffer[32] {};
    if (waveformPeak_ <= 0.00001f) {
        std::snprintf(peakBuffer, sizeof(peakBuffer), "PEAK  -inf dB");
    } else {
        std::snprintf(peakBuffer, sizeof(peakBuffer), "PEAK  %.1f dB",
                      20.0f * std::log10(waveformPeak_));
    }
    drawText(context, valueFont_, 9.0, kMutedText, peakBuffer,
             CRect(670.0, 540.0, graph.right, 558.0), kRightText);

    context.setFrameColor(CColor(74, 78, 77, 255));
    context.drawLine(CPoint(840.0, 544.0), CPoint(840.0, 664.0));
    drawLoopControls(context);
}

void KiqMainView::drawLoopControls(CDrawContext& context) {
    const CRect button(855.0, 558.0, 944.0, 652.0);
    fillRoundedRect(context, button, 9.0,
                    loopEnabled_ ? CColor(20, 49, 51, 255) : CColor(9, 12, 13, 255),
                    loopEnabled_ ? kAmplitude : CColor(71, 76, 75, 255), 2.0);
    CRect buttonInner = button;
    buttonInner.inset(5.0, 5.0);
    fillRoundedRect(context, buttonInner, 6.0, CColor(18, 22, 23, 255),
                    CColor(4, 6, 7, 255), 1.0);
    context.setFillColor(loopEnabled_ ? kAmplitude : CColor(58, 63, 62, 255));
    context.drawEllipse(CRect(894.0, 572.0, 905.0, 583.0), kDrawFilled);
    drawText(context, labelFont_, 10.0, kText, "LOOP",
             CRect(855.0, 592.0, 944.0, 611.0), kCenterText, kBoldFace);
    drawText(context, valueFont_, 8.5, loopEnabled_ ? kAmplitude : kMutedText,
             loopEnabled_ ? "RUNNING" : "OFF",
             CRect(855.0, 617.0, 944.0, 637.0), kCenterText);

    constexpr double minimumBpm = 40.0;
    constexpr double maximumBpm = 240.0;
    const CPoint center(1019.0, 602.0);
    constexpr double radius = 31.0;
    constexpr double startAngle = 135.0;
    constexpr double sweep = 270.0;
    const double normalized = (loopBpm_ - minimumBpm) / (maximumBpm - minimumBpm);
    const double angle = (startAngle + sweep * normalized) * kPi / 180.0;

    drawText(context, labelFont_, 9.5, kText, "TEMPO",
             CRect(970.0, 540.0, 1068.0, 557.0), kCenterText, kBoldFace);
    CRect outer(center.x - radius - 4.0, center.y - radius - 4.0,
                center.x + radius + 4.0, center.y + radius + 4.0);
    context.setFrameColor(CColor(54, 58, 58, 255));
    context.setLineWidth(4.0);
    context.drawArc(outer, static_cast<float>(startAngle),
                    static_cast<float>(startAngle + sweep), kDrawStroked);
    context.setFrameColor(kAmplitude);
    context.setLineWidth(2.5);
    context.drawArc(outer, static_cast<float>(startAngle),
                    static_cast<float>(startAngle + sweep * normalized), kDrawStroked);
    CRect face(center.x - radius, center.y - radius,
               center.x + radius, center.y + radius);
    context.setFillColor(kKnobFace);
    context.setFrameColor(kKnobEdge);
    context.setLineWidth(1.5);
    context.drawEllipse(face, kDrawFilledAndStroked);
    context.setFrameColor(kText);
    context.setLineWidth(2.5);
    context.drawLine(center,
                     CPoint(center.x + std::cos(angle) * radius * 0.7,
                            center.y + std::sin(angle) * radius * 0.7));

    char bpmBuffer[32] {};
    std::snprintf(bpmBuffer, sizeof(bpmBuffer), "%.0f BPM", loopBpm_);
    const CRect valueRect(974.0, 641.0, 1064.0, 661.0);
    fillRoundedRect(context, valueRect, 4.0, CColor(10, 13, 14, 255),
                    CColor(51, 56, 56, 255), 1.0);
    drawText(context, valueFont_, 9.5, kAmplitude, bpmBuffer, valueRect, kCenterText);
}

const std::array<KiqMainView::KnobDefinition, 8>&
KiqMainView::knobDefinitions() const {
    static const std::array<KnobDefinition, 8> model {{
        {KickParameterId::MembraneLevel, {66.0, 773.0}, "MEMBRANE", 28.0},
        {KickParameterId::ImpactLevel, {166.0, 773.0}, "IMPACT", 28.0},
        {KickParameterId::AirLevel, {266.0, 773.0}, "AIR", 28.0},
        {KickParameterId::SampleLevel, {366.0, 773.0}, "SAMPLE", 28.0},
        {KickParameterId::StrikePosition, {718.0, 773.0}, "STRIKE", 28.0},
        {KickParameterId::PhaseDegrees, {814.0, 773.0}, "PHASE", 28.0},
        {KickParameterId::AirDecayMs, {910.0, 773.0}, "AIR DECAY", 28.0},
        {KickParameterId::BeaterHardnessHz, {1006.0, 773.0}, "BEATER", 28.0},
    }};
    static const std::array<KnobDefinition, 8> output {{
        {KickParameterId::EqLowDb, {100.0, 773.0}, "LOW EQ", 31.0},
        {KickParameterId::EqMidDb, {235.0, 773.0}, "MID EQ", 31.0},
        {KickParameterId::EqHighDb, {370.0, 773.0}, "HIGH EQ", 31.0},
        {KickParameterId::Saturation, {730.0, 773.0}, "SATURATION", 31.0},
        {KickParameterId::LimiterCeilingDb, {865.0, 773.0}, "LIMIT", 31.0},
        {KickParameterId::OutputGain, {1000.0, 773.0}, "OUTPUT", 31.0},
        {KickParameterId::OutputGain, {}, "", 0.0},
        {KickParameterId::OutputGain, {}, "", 0.0},
    }};
    return controlPage_ == ControlPage::Model ? model : output;
}

std::size_t KiqMainView::knobCount() const {
    return controlPage_ == ControlPage::Model ? 8u : 6u;
}

void KiqMainView::drawControls(CDrawContext& context) {
    const CRect panel(14.0, 688.0, kDesignWidth - 14.0, kDesignHeight - 12.0);
    fillRoundedRect(context, panel, 12.0, kPanel, kPanelEdge, 1.2);
    CRect inner = panel;
    inner.inset(4.0, 4.0);
    fillRoundedRect(context, inner, 9.0, CColor(17, 20, 21, 255), kPanelInner, 1.0);

    drawWorkflowButton(context, modelTabRect(), "MODEL", true,
                       controlPage_ == ControlPage::Model);
    drawWorkflowButton(context, outputTabRect(), "OUTPUT", true,
                       controlPage_ == ControlPage::Output);
    if (sampleLayer_) {
        std::string sampleLabel = "SAMPLE READY";
        if (!sampleSourcePath_.empty()) {
            sampleLabel += "  " + std::filesystem::path(sampleSourcePath_).filename().string();
        }
        if (sampleLabel.size() > 38) {
            sampleLabel.resize(37);
            sampleLabel += "…";
        }
        drawText(context, valueFont_, 8.3, kReference, sampleLabel.c_str(),
                 CRect(178.0, 697.0, 450.0, 713.0), kLeftText);
    }

    const auto& definitions = knobDefinitions();
    for (std::size_t index = 0; index < knobCount(); ++index) {
        const auto& knob = definitions[index];
        drawKnob(context, knob.id, knob.center, knob.radius, knob.label);
    }
    drawHitButton(context);
    drawMeter(context);
}

void KiqMainView::drawKnob(CDrawContext& context, KickParameterId id,
                           const CPoint& center, double radius, const char* label) {
    const float normalized = normalizePlain(id, value(id));
    const double startAngle = 135.0;
    const double sweep = 270.0;
    const double angle = (startAngle + sweep * normalized) * kPi / 180.0;

    drawText(context, labelFont_, 9.0, kText, label,
             CRect(center.x - 48.0, 719.0, center.x + 48.0, 734.0),
             kCenterText, kBoldFace);

    CRect outer(center.x - radius - 5.0, center.y - radius - 5.0,
                center.x + radius + 5.0, center.y + radius + 5.0);
    context.setFrameColor(CColor(54, 58, 58, 255));
    context.setLineWidth(5.0);
    context.drawArc(outer, static_cast<float>(startAngle),
                    static_cast<float>(startAngle + sweep), kDrawStroked);
    context.setFrameColor(kAmplitude);
    context.setLineWidth(3.0);
    context.drawArc(outer, static_cast<float>(startAngle),
                    static_cast<float>(startAngle + sweep * normalized), kDrawStroked);

    CRect face(center.x - radius, center.y - radius,
               center.x + radius, center.y + radius);
    context.setFillColor(kKnobFace);
    context.setFrameColor(kKnobEdge);
    context.setLineWidth(2.0);
    context.drawEllipse(face, kDrawFilledAndStroked);
    CRect inner = face;
    inner.inset(6.0, 6.0);
    context.setFrameColor(CColor(5, 7, 7, 255));
    context.setLineWidth(1.0);
    context.drawEllipse(inner, kDrawStroked);

    const double pointerLength = radius * 0.72;
    context.setFrameColor(kText);
    context.setLineWidth(radius > 40.0 ? 4.0 : 3.0);
    context.drawLine(center,
                     CPoint(center.x + std::cos(angle) * pointerLength,
                            center.y + std::sin(angle) * pointerLength));

    char valueBuffer[32] {};
    formatParameter(id, value(id), valueBuffer);
    const double width = 78.0;
    const CRect valueRect(center.x - width * 0.5, 817.0,
                          center.x + width * 0.5, 837.0);
    fillRoundedRect(context, valueRect, 4.0, CColor(10, 13, 14, 255),
                    CColor(51, 56, 56, 255), 1.0);
    drawText(context, valueFont_, 10.0, kAmplitude, valueBuffer, valueRect, kCenterText);
}

void KiqMainView::drawHitButton(CDrawContext& context) {
    const CRect button(493.0, 714.0, 607.0, 820.0);
    fillRoundedRect(context, button, 10.0,
                    hitPressed_ ? CColor(24, 52, 54, 255) : CColor(9, 12, 13, 255),
                    hitPressed_ ? kAmplitude : CColor(77, 81, 80, 255), 2.0);
    CRect inner = button;
    inner.inset(6.0, 6.0);
    fillRoundedRect(context, inner, 7.0, CColor(19, 23, 24, 255),
                    CColor(4, 6, 7, 255), 1.0);
    context.setFillColor(kAmplitude);
    context.drawRect(CRect(535.0, 728.0, 565.0, 732.0), kDrawFilled);
    drawText(context, titleFont_, 29.0, kText, "HIT",
             CRect(493.0, 753.0, 607.0, 796.0), kCenterText, kBoldFace);
}

void KiqMainView::drawMeter(CDrawContext& context) {
    context.setFrameColor(CColor(74, 78, 77, 255));
    context.setLineWidth(1.0);
    context.drawLine(CPoint(1048.0, 718.0), CPoint(1048.0, 836.0));

    drawText(context, labelFont_, 9.0, clipHoldFrames_ > 0 ? kRed : kMutedText, "CLIP",
             CRect(1060.0, 708.0, 1090.0, 722.0), kCenterText, kBoldFace);
    constexpr int segments = 7;
    for (int segment = 0; segment < segments; ++segment) {
        const double bottom = 814.0 - segment * 14.0;
        const CRect led(1067.0, bottom - 8.0, 1085.0, bottom);
        const float threshold = static_cast<float>(segment + 1) / segments;
        CColor on = segment >= segments - 2 ? kRed : kAmplitude;
        const CColor off = CColor(45, 49, 49, 255);
        context.setFillColor(displayedPeak_ >= threshold ? on : off);
        context.drawRect(led, kDrawFilled);
    }
}

bool KiqMainView::beginPointDrag(const CPoint& where) {
    static constexpr std::array<KickParameterId, 4> pitchValueIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1Hz,
        KickParameterId::Pitch2Hz, KickParameterId::Pitch3Hz,
    };
    static constexpr std::array<KickParameterId, 4> pitchTimeIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1TimeMs,
        KickParameterId::Pitch2TimeMs, KickParameterId::Pitch3TimeMs,
    };
    static constexpr std::array<KickParameterId, 4> ampValueIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1Db,
        KickParameterId::Amp2Db, KickParameterId::Amp3Db,
    };
    static constexpr std::array<KickParameterId, 4> ampTimeIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1TimeMs,
        KickParameterId::Amp2TimeMs, KickParameterId::Amp3TimeMs,
    };

    for (const auto kind : {TrajectoryKind::Pitch, TrajectoryKind::Amplitude}) {
        const float timeMax = trajectoryTimeMax(kind);
        for (std::size_t index = 0; index < 4; ++index) {
            if (distanceSquared(where, trajectoryPoint(kind, index, timeMax)) > 15.0f * 15.0f) {
                continue;
            }
            drag_ = {};
            drag_.kind = DragKind::Point;
            drag_.trajectory = kind;
            drag_.index = index;
            drag_.startMouse = where;
            drag_.frozenTimeMax = timeMax;
            const KickParameterId valueId = kind == TrajectoryKind::Pitch
                                                ? pitchValueIds[index]
                                                : ampValueIds[index];
            drag_.startValue = value(valueId);
            drag_.parameterIds.push_back(valueId);
            if (index > 0) {
                const KickParameterId timeId = kind == TrajectoryKind::Pitch
                                                   ? pitchTimeIds[index]
                                                   : ampTimeIds[index];
                drag_.startTime = value(timeId);
                drag_.parameterIds.push_back(timeId);
            }
            for (const auto id : drag_.parameterIds) {
                bridge_.beginParameterEdit(id);
            }
            return true;
        }
    }
    return false;
}

bool KiqMainView::beginCurveDrag(const CPoint& where) {
    static constexpr std::array<KickParameterId, 3> pitchIds {
        KickParameterId::PitchCurve1, KickParameterId::PitchCurve2,
        KickParameterId::PitchCurve3,
    };
    static constexpr std::array<KickParameterId, 3> ampIds {
        KickParameterId::AmpCurve1, KickParameterId::AmpCurve2,
        KickParameterId::AmpCurve3,
    };
    for (const auto kind : {TrajectoryKind::Pitch, TrajectoryKind::Amplitude}) {
        const float timeMax = trajectoryTimeMax(kind);
        for (std::size_t segment = 0; segment < 3; ++segment) {
            if (distanceSquared(where, curveHandlePoint(kind, segment, timeMax)) > 12.0f * 12.0f) {
                continue;
            }
            const KickParameterId id = kind == TrajectoryKind::Pitch
                                           ? pitchIds[segment]
                                           : ampIds[segment];
            drag_ = {};
            drag_.kind = DragKind::Curve;
            drag_.trajectory = kind;
            drag_.index = segment;
            drag_.startMouse = where;
            drag_.startCurve = value(id);
            drag_.parameterIds = {id};
            bridge_.beginParameterEdit(id);
            return true;
        }
    }
    return false;
}

bool KiqMainView::beginKnobDrag(const CPoint& where, bool resetToDefault) {
    const auto& definitions = knobDefinitions();
    for (std::size_t index = 0; index < knobCount(); ++index) {
        const auto& knob = definitions[index];
        const float hitRadius = static_cast<float>(knob.radius + 12.0);
        if (distanceSquared(where, knob.center) > hitRadius * hitRadius) {
            continue;
        }
        const KickParameterId id = knob.id;
        if (resetToDefault) {
            bridge_.beginParameterEdit(id);
            setValue(id, getDefaultKickParameter(id));
            bridge_.endParameterEdit(id);
            recordCurrentState();
            return true;
        }
        drag_ = {};
        drag_.kind = DragKind::Knob;
        drag_.index = index;
        drag_.startMouse = where;
        drag_.lastMouse = where;
        drag_.startValue = value(id);
        drag_.parameterIds = {id};
        bridge_.beginParameterEdit(id);
        return true;
    }
    return false;
}

bool KiqMainView::beginTempoDrag(const CPoint& where, bool resetToDefault) {
    const CPoint center(1019.0, 602.0);
    constexpr float hitRadius = 43.0f;
    if (distanceSquared(where, center) > hitRadius * hitRadius) {
        return false;
    }
    if (resetToDefault) {
        setLoopBpm(120.0f);
        return true;
    }
    drag_ = {};
    drag_.kind = DragKind::Tempo;
    drag_.startMouse = where;
    drag_.lastMouse = where;
    drag_.startValue = loopBpm_;
    return true;
}

bool KiqMainView::beginPhaseLockDrag(const CPoint& where) {
    const float lockMs = value(KickParameterId::PhaseLockMs);
    if (lockMs < 0.0f) {
        return false;
    }
    const CRect graph = trajectoryGraph(TrajectoryKind::Pitch);
    const float timeMax = trajectoryTimeMax(TrajectoryKind::Pitch);
    const double x = graph.left + graph.getWidth() *
        std::clamp(lockMs / timeMax, 0.0f, 1.0f);
    if (where.y < graph.top - 8.0 || where.y > graph.bottom + 8.0 ||
        std::abs(where.x - x) > 10.0) {
        return false;
    }
    drag_ = {};
    drag_.kind = DragKind::PhaseLock;
    drag_.startMouse = where;
    drag_.startValue = lockMs;
    drag_.frozenTimeMax = timeMax;
    drag_.parameterIds = {KickParameterId::PhaseLockMs};
    bridge_.beginParameterEdit(KickParameterId::PhaseLockMs);
    return true;
}

bool KiqMainView::handleWorkflowButton(const CPoint& where) {
    if (presetButtonRect().pointInside(where)) {
        showPresetMenu();
        return true;
    }
    if (undoButtonRect().pointInside(where)) {
        undo();
        return true;
    }
    if (redoButtonRect().pointInside(where)) {
        redo();
        return true;
    }
    if (importButtonRect().pointInside(where)) {
        openReferenceSelector();
        return true;
    }
    if (fitButtonRect().pointInside(where)) {
        if (referenceAnalysis_) {
            applyReferenceFit();
        }
        return true;
    }
    if (alignButtonRect().pointInside(where)) {
        if (referenceAnalysis_) {
            alignReferencePhase();
        }
        return true;
    }
    if (exportButtonRect().pointInside(where)) {
        drag_ = {};
        drag_.kind = DragKind::Export;
        drag_.startMouse = where;
        return true;
    }
    if (phaseLockButtonRect().pointInside(where)) {
        const auto id = KickParameterId::PhaseLockMs;
        const float next = value(id) >= 0.0f
                               ? -1.0f
                               : (referenceAnalysis_
                                      ? referenceAnalysis_->fit.phaseReferenceTimeMs
                                      : value(KickParameterId::Pitch1TimeMs));
        bridge_.beginParameterEdit(id);
        setValue(id, next);
        bridge_.endParameterEdit(id);
        recordCurrentState();
        return true;
    }
    return false;
}

bool KiqMainView::handleControlPageButton(const CPoint& where) {
    if (modelTabRect().pointInside(where)) {
        controlPage_ = ControlPage::Model;
        invalid();
        return true;
    }
    if (outputTabRect().pointInside(where)) {
        controlPage_ = ControlPage::Output;
        invalid();
        return true;
    }
    return false;
}

bool KiqMainView::handleHitButton(const CPoint& where) {
    if (!CRect(493.0, 714.0, 607.0, 820.0).pointInside(where)) {
        return false;
    }
    hitPressed_ = true;
    bridge_.triggerAudition();
    invalid();
    return true;
}

bool KiqMainView::handleLoopButton(const CPoint& where) {
    if (!CRect(855.0, 558.0, 944.0, 652.0).pointInside(where)) {
        return false;
    }
    setLoopEnabled(!loopEnabled_);
    return true;
}

void KiqMainView::onMouseDownEvent(MouseDownEvent& event) {
    if (!event.buttonState.isLeft()) {
        return;
    }
    const bool doubleClick = event.clickCount >= 2;
    if (handleWorkflowButton(event.mousePosition) ||
        handleControlPageButton(event.mousePosition) ||
        handleHitButton(event.mousePosition) ||
        handleLoopButton(event.mousePosition) ||
        beginTempoDrag(event.mousePosition, doubleClick) ||
        beginPhaseLockDrag(event.mousePosition) ||
        beginPointDrag(event.mousePosition) ||
        beginCurveDrag(event.mousePosition) ||
        beginKnobDrag(event.mousePosition, doubleClick)) {
        event.consumed = true;
    }
}

void KiqMainView::updateDrag(const CPoint& where, bool fineAdjustment) {
    if (drag_.kind == DragKind::None) {
        return;
    }
    const float sensitivity = fineAdjustment ? 0.15f : 1.0f;
    if (drag_.kind == DragKind::Export) {
        return;
    }
    if (drag_.kind == DragKind::Tempo) {
        const float delta = static_cast<float>((drag_.lastMouse.y - where.y) / 220.0) *
                            200.0f * sensitivity;
        setLoopBpm(loopBpm_ + delta);
        drag_.lastMouse = where;
        return;
    }
    if (drag_.kind == DragKind::Knob) {
        const KickParameterId id = drag_.parameterIds.front();
        const float currentNormalized = normalizePlain(id, value(id));
        const float delta = static_cast<float>((drag_.lastMouse.y - where.y) / 220.0) *
                            sensitivity;
        setValue(id, denormalizePlain(id, currentNormalized + delta));
        drag_.lastMouse = where;
        return;
    }
    if (drag_.kind == DragKind::PhaseLock) {
        const CRect graph = trajectoryGraph(TrajectoryKind::Pitch);
        const float normalized = std::clamp(
            static_cast<float>((where.x - graph.left) / graph.getWidth()),
            0.0f, 1.0f);
        setValue(KickParameterId::PhaseLockMs,
                 normalized * drag_.frozenTimeMax);
        return;
    }
    if (drag_.kind == DragKind::Curve) {
        const KickParameterId id = drag_.parameterIds.front();
        const float delta = static_cast<float>((drag_.startMouse.y - where.y) / 75.0) * sensitivity;
        setValue(id, drag_.startCurve + delta);
        return;
    }

    static constexpr std::array<KickParameterId, 4> pitchValueIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1Hz,
        KickParameterId::Pitch2Hz, KickParameterId::Pitch3Hz,
    };
    static constexpr std::array<KickParameterId, 4> pitchTimeIds {
        KickParameterId::Pitch0Hz, KickParameterId::Pitch1TimeMs,
        KickParameterId::Pitch2TimeMs, KickParameterId::Pitch3TimeMs,
    };
    static constexpr std::array<KickParameterId, 4> ampValueIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1Db,
        KickParameterId::Amp2Db, KickParameterId::Amp3Db,
    };
    static constexpr std::array<KickParameterId, 4> ampTimeIds {
        KickParameterId::Amp0Db, KickParameterId::Amp1TimeMs,
        KickParameterId::Amp2TimeMs, KickParameterId::Amp3TimeMs,
    };

    const bool pitch = drag_.trajectory == TrajectoryKind::Pitch;
    const CRect graph = trajectoryGraph(drag_.trajectory);
    const KickParameterId valueId = pitch ? pitchValueIds[drag_.index]
                                           : ampValueIds[drag_.index];
    const float normalizedY = std::clamp(
        static_cast<float>((graph.bottom - where.y) / graph.getHeight()), 0.0f, 1.0f);
    float plainValue = 0.0f;
    if (pitch) {
        constexpr float minimumHz = 20.0f;
        constexpr float maximumHz = 1000.0f;
        plainValue = std::exp(std::log(minimumHz) +
                              (std::log(maximumHz) - std::log(minimumHz)) * normalizedY);
    } else {
        plainValue = -60.0f + 66.0f * normalizedY;
    }
    setValue(valueId, plainValue);

    if (drag_.index == 0) {
        return;
    }
    const KickParameterId timeId = pitch ? pitchTimeIds[drag_.index]
                                          : ampTimeIds[drag_.index];
    float time = static_cast<float>((where.x - graph.left) / graph.getWidth()) *
                 drag_.frozenTimeMax;
    const auto& timeIds = pitch ? pitchTimeIds : ampTimeIds;
    const float previousTime = drag_.index == 1 ? 0.0f : value(timeIds[drag_.index - 1]);
    const float nextTime = drag_.index == 3
                               ? std::numeric_limits<float>::max()
                               : value(timeIds[drag_.index + 1]);
    time = std::max(time, previousTime + 0.01f);
    time = std::min(time, nextTime - 0.01f);
    setValue(timeId, time);
}

void KiqMainView::onMouseMoveEvent(MouseMoveEvent& event) {
    if (drag_.kind == DragKind::None || !event.buttonState.isLeft()) {
        return;
    }
    if (drag_.kind == DragKind::Export) {
        if (VSTGUI::shouldStartDrag(drag_.startMouse, event.mousePosition)) {
            drag_ = {};
            startExportDrag();
        }
        event.consumed = true;
        return;
    }
    updateDrag(event.mousePosition, event.modifiers.has(ModifierKey::Shift));
    event.consumed = true;
}

void KiqMainView::endDrag() {
    const DragKind completedKind = drag_.kind;
    for (const auto id : drag_.parameterIds) {
        bridge_.endParameterEdit(id);
    }
    drag_ = {};
    if (completedKind == DragKind::Point || completedKind == DragKind::Curve ||
        completedKind == DragKind::Knob || completedKind == DragKind::PhaseLock) {
        recordCurrentState();
    }
}

void KiqMainView::cancelDrag() {
    if (drag_.kind == DragKind::Tempo) {
        setLoopBpm(drag_.startValue);
    } else if (drag_.kind == DragKind::Knob && !drag_.parameterIds.empty()) {
        setValue(drag_.parameterIds.front(), drag_.startValue);
    } else if (drag_.kind == DragKind::Curve && !drag_.parameterIds.empty()) {
        setValue(drag_.parameterIds.front(), drag_.startCurve);
    } else if (drag_.kind == DragKind::PhaseLock) {
        setValue(KickParameterId::PhaseLockMs, drag_.startValue);
    } else if (drag_.kind == DragKind::Point && !drag_.parameterIds.empty()) {
        setValue(drag_.parameterIds.front(), drag_.startValue);
        if (drag_.parameterIds.size() > 1) {
            setValue(drag_.parameterIds[1], drag_.startTime);
        }
    }
    for (const auto id : drag_.parameterIds) {
        bridge_.endParameterEdit(id);
    }
    drag_ = {};
}

void KiqMainView::onMouseUpEvent(MouseUpEvent& event) {
    if (hitPressed_) {
        hitPressed_ = false;
        invalid();
        event.consumed = true;
    }
    if (drag_.kind == DragKind::Export) {
        drag_ = {};
        openExportSelector();
        event.consumed = true;
        return;
    }
    if (drag_.kind != DragKind::None) {
        endDrag();
        event.consumed = true;
    }
}

void KiqMainView::onMouseCancelEvent(MouseCancelEvent& event) {
    hitPressed_ = false;
    if (drag_.kind != DragKind::None) {
        cancelDrag();
    }
    invalid();
    event.consumed = true;
}

void KiqMainView::onMouseWheelEvent(MouseWheelEvent& event) {
    const CPoint tempoCenter(1019.0, 602.0);
    constexpr float tempoHitRadius = 43.0f;
    if (distanceSquared(event.mousePosition, tempoCenter) <=
        tempoHitRadius * tempoHitRadius) {
        const float increment = event.modifiers.has(ModifierKey::Shift) ? 1.0f : 5.0f;
        setLoopBpm(loopBpm_ + static_cast<float>(event.deltaY) * increment);
        event.consumed = true;
        return;
    }

    const auto& definitions = knobDefinitions();
    for (std::size_t index = 0; index < knobCount(); ++index) {
        const auto& knob = definitions[index];
        const float hitRadius = static_cast<float>(knob.radius + 12.0);
        if (distanceSquared(event.mousePosition, knob.center) >
            hitRadius * hitRadius) {
            continue;
        }
        const KickParameterId id = knob.id;
        const float increment = event.modifiers.has(ModifierKey::Shift) ? 0.002f : 0.015f;
        bridge_.beginParameterEdit(id);
        setValue(id, denormalizePlain(id, normalizePlain(id, value(id)) +
                                             static_cast<float>(event.deltaY) * increment));
        bridge_.endParameterEdit(id);
        recordCurrentState();
        event.consumed = true;
        return;
    }
}

void KiqMainView::setStatus(std::string message) {
    statusMessage_ = std::move(message);
    statusFrames_ = 180;
    invalid();
}

void KiqMainView::showPresetMenu() {
    auto menu = makeOwned<COptionMenu>();
    menu->setStyle(COptionMenu::kPopupStyle);
    const auto& presets = factoryPresets();
    for (std::size_t index = 0; index < presets.size(); ++index) {
        auto* item = new CMenuItem(presets[index].name.c_str(),
                                   static_cast<int32_t>(1000 + index));
        item->setChecked(presets[index].name == presetName_);
        menu->addEntry(item);
    }
    menu->addSeparator();
    menu->addEntry(new CMenuItem("Save Preset…", 2000));
    menu->addEntry(new CMenuItem("Load Preset…", 2001));

    CPoint location = presetButtonRect().getBottomLeft();
    localToFrame(location);
    menu->popup(getFrame(), location,
                [state = callbackState_](COptionMenu* selectedMenu) {
        auto* self = state->view.load(std::memory_order_acquire);
        if (!self) {
            return;
        }
        const auto result = selectedMenu->getLastResult();
        if (result < 0) {
            return;
        }
        const auto* item = selectedMenu->getEntry(result);
        if (!item) {
            return;
        }
        const int32_t tag = item->getTag();
        if (tag >= 1000 && tag < 2000) {
            self->applyFactoryPreset(static_cast<std::size_t>(tag - 1000));
        } else if (tag == 2000) {
            self->openPresetSaveSelector();
        } else if (tag == 2001) {
            self->openPresetLoadSelector();
        }
    });
}

void KiqMainView::applyFactoryPreset(std::size_t index) {
    const auto& presets = factoryPresets();
    if (index >= presets.size()) {
        return;
    }
    presetName_ = presets[index].name;
    applyParams(presets[index].params);
    setStatus("Loaded factory preset: " + presetName_);
}

void KiqMainView::openPresetSaveSelector() {
    auto selector = owned(CNewFileSelector::create(
        getFrame(), CNewFileSelector::Style::kSelectSaveFile));
    if (!selector) {
        setStatus("Could not open the preset save dialog");
        return;
    }
    selector->setTitle("Save Kiq Preset");
    selector->setDefaultSaveName(
        (safeSaveStem(presetName_) + KickPresetIO::kFileExtension).c_str());
    selector->setDefaultExtension(
        CFileExtension("Kiq Preset", "kiqpreset", "application/json"));
    const bool started = selector->run([state = callbackState_](CNewFileSelector* result) {
        auto* self = state->view.load(std::memory_order_acquire);
        if (!self) {
            return;
        }
        if (result->getNumSelectedFiles() == 0) {
            return;
        }
        const auto* selected = result->getSelectedFile(0);
        if (selected) {
            self->savePreset(withExtension(selected, KickPresetIO::kFileExtension));
        }
    });
    if (!started) {
        // Balance CNewFileSelector::run()'s internal remember() when the
        // platform declines to start and therefore cannot invoke its callback.
        selector->forget();
        setStatus("Could not open the preset save dialog");
    }
}

void KiqMainView::openPresetLoadSelector() {
    auto selector = owned(CNewFileSelector::create(
        getFrame(), CNewFileSelector::Style::kSelectFile));
    if (!selector) {
        setStatus("Could not open the preset load dialog");
        return;
    }
    selector->setTitle("Load Kiq Preset");
    selector->setAllowMultiFileSelection(false);
    selector->addFileExtension(
        CFileExtension("Kiq Preset", "kiqpreset", "application/json"));
    const bool started = selector->run([state = callbackState_](CNewFileSelector* result) {
        auto* self = state->view.load(std::memory_order_acquire);
        if (!self) {
            return;
        }
        if (result->getNumSelectedFiles() == 0) {
            return;
        }
        const auto* selected = result->getSelectedFile(0);
        if (selected) {
            self->loadPreset(selected);
        }
    });
    if (!started) {
        selector->forget();
        setStatus("Could not open the preset load dialog");
    }
}

void KiqMainView::savePreset(const std::string& path) {
    KickPresetDocument preset;
    const std::string filenameStem = std::filesystem::path(path).stem().string();
    preset.name = filenameStem.empty() ? presetName_ : filenameStem;
    preset.params = currentParams();
    if (sampleLayer_) {
        KickSamplePayload payload;
        payload.sourcePath = sampleSourcePath_;
        payload.audio = *sampleLayer_;
        preset.sampleLayer = std::move(payload);
    }
    std::string error;
    if (!KickPresetIO::saveToFile(path, preset, &error)) {
        setStatus("Preset save failed: " + error);
        return;
    }
    presetName_ = preset.name;
    setStatus("Saved preset: " + std::filesystem::path(path).filename().string());
}

void KiqMainView::loadPreset(const std::string& path) {
    KickPresetDocument preset;
    std::string error;
    if (!KickPresetIO::loadFromFile(path, preset, &error)) {
        setStatus("Preset load failed: " + error);
        return;
    }
    presetName_ = preset.name;
    referenceAnalysis_.reset();
    sampleSourcePath_.clear();
    if (preset.sampleLayer) {
        sampleSourcePath_ = preset.sampleLayer->sourcePath;
        if (!preset.sampleLayer->audio.samples.empty()) {
            sampleLayer_ = std::make_shared<SampleLayerData>(
                std::move(preset.sampleLayer->audio));
        } else {
            sampleLayer_.reset();
        }
    } else {
        sampleLayer_.reset();
    }
    installSampleLayer(sampleLayer_);
    applyParams(preset.params, false);
    history_.reset(currentParams());
    waveformDirty_ = true;
    setStatus("Loaded preset: " + presetName_);
}

void KiqMainView::installSampleLayer(
    std::shared_ptr<const SampleLayerData> sampleLayer) {
    sampleLayer_ = std::move(sampleLayer);
    bridge_.setSampleLayer(sampleLayer_);
    // The standalone engine sanitizes into its own immutable allocation;
    // controllers can retain the supplied allocation. In either case, keep
    // preview/export on the same authoritative source used by the bridge.
    sampleLayer_ = bridge_.getSampleLayer();
    waveformDirty_ = true;
}

void KiqMainView::openReferenceSelector() {
    auto selector = owned(CNewFileSelector::create(
        getFrame(), CNewFileSelector::Style::kSelectFile));
    if (!selector) {
        setStatus("Could not open the WAV import dialog");
        return;
    }
    selector->setTitle("Import WAV Reference");
    selector->setAllowMultiFileSelection(false);
    selector->addFileExtension(
        CFileExtension("WAVE Audio", "wav", "audio/wav"));
    const bool started = selector->run([state = callbackState_](CNewFileSelector* result) {
        auto* self = state->view.load(std::memory_order_acquire);
        if (!self) {
            return;
        }
        if (result->getNumSelectedFiles() == 0) {
            return;
        }
        const auto* selected = result->getSelectedFile(0);
        if (selected) {
            self->importReference(selected);
        }
    });
    if (!started) {
        selector->forget();
        setStatus("Could not open the WAV import dialog");
    }
}

bool KiqMainView::importReference(const std::string& path) {
    const auto state = callbackState_;
    {
        std::lock_guard<std::mutex> lock(state->referenceMutex);
        if (state->referenceInFlight) {
            setStatus("Reference analysis is already running");
            return false;
        }
        state->completedReference.reset();
        state->referenceInFlight = true;
    }

    setStatus("Analyzing reference: " +
              std::filesystem::path(path).filename().string());
    try {
        if (referenceWorker_.joinable()) {
            referenceWorker_.join();
        }
        referenceWorker_ = std::thread([state, path] {
            ReferenceImportResult completed;

            try {
                completed.path = path;
                // Bound both the encoded file allocation and the decoded mono
                // buffer. This still accommodates several minutes of ordinary
                // stereo reference material while rejecting pathological inputs.
                WavDecodeLimits limits;
                limits.maximumFileBytes = 128u * 1024u * 1024u;
                limits.maximumFrames = 12u * 1024u * 1024u;
                auto decoded = loadWavFile(path, limits);
                if (!decoded) {
                    completed.error = "WAV import failed: " + decoded.message;
                } else {
                    auto analyzed = analyzeReferenceKick(decoded.audio);
                    if (!analyzed) {
                        completed.error =
                            "Reference analysis failed: " + analyzed.message;
                    } else {
                        completed.analysis = std::move(analyzed.analysis);
                    }
                }
            } catch (const std::exception& error) {
                completed.error =
                    "Reference analysis failed: " + std::string(error.what());
            } catch (...) {
                completed.error = "Reference analysis failed unexpectedly";
            }

            std::lock_guard<std::mutex> lock(state->referenceMutex);
            state->completedReference = std::move(completed);
            state->referenceInFlight = false;
        });
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(state->referenceMutex);
            state->referenceInFlight = false;
        }
        setStatus("Could not start reference analysis: " + std::string(error.what()));
        return false;
    }
    return true;
}

void KiqMainView::consumeReferenceImport() {
    std::optional<ReferenceImportResult> completed;
    {
        std::lock_guard<std::mutex> lock(callbackState_->referenceMutex);
        if (!callbackState_->completedReference) {
            return;
        }
        completed = std::move(callbackState_->completedReference);
        callbackState_->completedReference.reset();
    }

    if (!completed->analysis) {
        setStatus(completed->error.empty()
                      ? "Reference analysis failed"
                      : std::move(completed->error));
        return;
    }

    referenceAnalysis_ = std::move(*completed->analysis);
    sampleSourcePath_ = completed->path;
    if (referenceAnalysis_->transientSample &&
        !referenceAnalysis_->transientSample->monoSamples.empty()) {
        auto layer = std::make_shared<SampleLayerData>();
        layer->sourceSampleRate = std::clamp(
            static_cast<float>(referenceAnalysis_->transientSample->sampleRate),
            1000.0f, 768000.0f);
        layer->samples = referenceAnalysis_->transientSample->monoSamples;
        installSampleLayer(std::move(layer));
    } else {
        installSampleLayer(nullptr);
    }
    applyReferenceFit();
    history_.reset(currentParams());
    waveformDirty_ = true;
    setStatus("Matched reference: " +
              std::filesystem::path(completed->path).filename().string());
}

void KiqMainView::applyReferenceFit() {
    if (!referenceAnalysis_) {
        return;
    }
    // Matching is deterministic from the reference alone. Start from a
    // neutral/default post stage instead of carrying EQ or saturation from a
    // previously selected sound into the fitted result. Phase stays under the
    // independent ALIGN workflow.
    const KickParams current = currentParams();
    KickParams params = kDefaultKickParams;
    params.phaseDegrees = current.phaseDegrees;
    params.phaseLockMs = current.phaseLockMs;
    const auto& fit = referenceAnalysis_->fit;
    for (std::size_t index = 0; index < kTrajectoryPointCount; ++index) {
        params.pitch[index].timeMs = fit.pitchHz[index].timeMs;
        params.pitch[index].value = fit.pitchHz[index].value;
        params.amplitude[index].timeMs = fit.amplitudeDb[index].timeMs;
        params.amplitude[index].value = fit.amplitudeDb[index].value;
        if (index + 1 < kTrajectoryPointCount) {
            params.pitch[index].curve = fit.pitchHz[index].curve;
            params.amplitude[index].curve = fit.amplitudeDb[index].curve;
        }
    }
    params.strikePosition = fit.strikePosition;
    params.transient.impactLevel = fit.impactLevel;
    params.transient.airLevel = fit.airLevel;
    params.transient.airDecayMs = fit.airDecayMs;
    params.transient.beaterHardnessHz = fit.beaterHardnessHz;
    params.outputGain = fit.outputGain;
    params.membraneLevel = std::clamp(
        std::sqrt(referenceAnalysis_->components.bodyEnergyFraction), 0.2f, 1.0f);
    if (referenceAnalysis_->transientSample && sampleLayer_) {
        const float outputGain = std::max(params.outputGain, 1.0e-4f);
        params.sampleLevel = std::clamp(
            referenceAnalysis_->transientSample->suggestedGain / outputGain,
            0.0f, 1.0f);
    } else {
        params.sampleLevel = 0.0f;
    }
    applyParams(params);
    presetName_ = "Reference Match";
    setStatus("Physical model fit applied");
}

void KiqMainView::alignReferencePhase() {
    if (!referenceAnalysis_) {
        return;
    }
    float phase = std::fmod(referenceAnalysis_->fit.phaseAtOnsetDegrees, 360.0f);
    if (phase > 180.0f) {
        phase -= 360.0f;
    } else if (phase < -180.0f) {
        phase += 360.0f;
    }
    KickParams params = currentParams();
    params.phaseDegrees = phase;
    params.phaseLockMs = std::max(0.0f,
        referenceAnalysis_->fit.phaseReferenceTimeMs);
    applyParams(params);
    presetName_ = "Edited";
    char message[80] {};
    std::snprintf(message, sizeof(message), "Phase aligned: %+.0f deg (%.0f%% confidence)",
                  phase, referenceAnalysis_->fit.phaseConfidence * 100.0f);
    setStatus(message);
}

void KiqMainView::openExportSelector() {
    auto selector = owned(CNewFileSelector::create(
        getFrame(), CNewFileSelector::Style::kSelectSaveFile));
    if (!selector) {
        setStatus("Could not open the WAV export dialog");
        return;
    }
    selector->setTitle("Export Kiq Kick");
    selector->setDefaultSaveName((safeSaveStem(presetName_) + ".wav").c_str());
    selector->setDefaultExtension(
        CFileExtension("WAVE Audio", "wav", "audio/wav"));
    const bool started = selector->run([state = callbackState_](CNewFileSelector* result) {
        auto* self = state->view.load(std::memory_order_acquire);
        if (!self) {
            return;
        }
        if (result->getNumSelectedFiles() == 0) {
            return;
        }
        const auto* selected = result->getSelectedFile(0);
        if (selected) {
            self->exportWav(withExtension(selected, ".wav"));
        }
    });
    if (!started) {
        selector->forget();
        setStatus("Could not open the WAV export dialog");
    }
}

bool KiqMainView::exportWav(const std::string& path) {
    KickRenderSettings settings;
    settings.sampleRate = 48000;
    settings.sampleLayer = sampleLayer_.get();
    std::string error;
    if (!KickWavExporter::renderToFile(
            path, currentParams(), settings, &error)) {
        setStatus("WAV export failed: " + error);
        return false;
    }
    setStatus("Exported: " + std::filesystem::path(path).filename().string());
    return true;
}

void KiqMainView::startExportDrag() {
    cleanupOldTemporaryExports();
    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch().count();
    temporaryExportPath_ =
        (std::filesystem::temp_directory_path() /
         ("Kiq-Kick-" + std::to_string(stamp) + ".wav")).string();
    if (!exportWav(temporaryExportPath_) ||
        temporaryExportPath_.size() >=
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return;
    }
    const auto package = CDropSource::create(
        temporaryExportPath_.c_str(),
        static_cast<std::uint32_t>(temporaryExportPath_.size() + 1),
        IDataPackage::kFilePath);
    if (!doDrag(DragDescription(package))) {
        setStatus("The host did not start the WAV drag");
    }
}

std::optional<std::string> KiqMainView::firstWavPath(IDataPackage* package) {
    if (!package) {
        return std::nullopt;
    }
    for (const auto& item : package) {
        if (item.type != IDataPackage::kFilePath || !item.data || item.dataSize == 0) {
            continue;
        }
        std::string path(static_cast<const char*>(item.data), item.dataSize);
        while (!path.empty() && path.back() == '\0') {
            path.pop_back();
        }
        std::string extension = std::filesystem::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                       });
        if (extension == ".wav") {
            return path;
        }
    }
    return std::nullopt;
}

DragOperation KiqMainView::onDragEnter(DragEventData data) {
    dropHover_ = firstWavPath(data.drag).has_value();
    invalid();
    return dropHover_ ? DragOperation::Copy : DragOperation::None;
}

DragOperation KiqMainView::onDragMove(DragEventData data) {
    const bool accepts = firstWavPath(data.drag).has_value();
    if (accepts != dropHover_) {
        dropHover_ = accepts;
        invalid();
    }
    return accepts ? DragOperation::Copy : DragOperation::None;
}

void KiqMainView::onDragLeave(DragEventData) {
    dropHover_ = false;
    invalid();
}

bool KiqMainView::onDrop(DragEventData data) {
    dropHover_ = false;
    invalid();
    const auto path = firstWavPath(data.drag);
    return path && importReference(*path);
}

void KiqMainView::onKeyboardEvent(KeyboardEvent& event) {
    const bool command = event.modifiers.has(ModifierKey::Super) ||
                         event.modifiers.has(ModifierKey::Control);
    if (event.type == EventType::KeyDown && command &&
        (event.character == U'z' || event.character == U'Z')) {
        if (event.modifiers.has(ModifierKey::Shift)) {
            redo();
        } else {
            undo();
        }
        event.consumed = true;
    } else if (event.type == EventType::KeyDown &&
        (event.character == U' ' || event.virt == VirtualKey::Return)) {
        hitPressed_ = true;
        bridge_.triggerAudition();
        invalid();
        event.consumed = true;
    } else if (event.type == EventType::KeyUp &&
               (event.character == U' ' || event.virt == VirtualKey::Return)) {
        hitPressed_ = false;
        invalid();
        event.consumed = true;
    }
}

} // namespace KickDrum::UI
