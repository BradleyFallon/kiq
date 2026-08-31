#include "KiqMainView.h"

#include "Trajectory.h"

#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/events.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

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
    return std::clamp(plainValue, spec->minimum, spec->maximum);
}

float normalizePlain(KickParameterId id, float plainValue) {
    const auto* spec = findKickParameterSpec(id);
    if (!spec || spec->maximum <= spec->minimum) {
        return 0.0f;
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
        case KickParameterId::StartPhase:
            std::snprintf(buffer, sizeof(buffer), "%.0f deg", plainValue * 360.0f);
            break;
        case KickParameterId::ClickLevel:
        case KickParameterId::NoiseLevel:
            std::snprintf(buffer, sizeof(buffer), "%.0f %%", plainValue * 100.0f);
            break;
        case KickParameterId::NoiseDecayMs:
            std::snprintf(buffer, sizeof(buffer), "%.1f ms", plainValue);
            break;
        case KickParameterId::NoiseToneHz:
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
        default:
            std::snprintf(buffer, sizeof(buffer), "%.2f", plainValue);
            break;
    }
    return buffer;
}

} // namespace

KiqMainView::KiqMainView(const CRect& size, KiqUIBridge& bridge)
    : CView(size)
    , bridge_(bridge)
    , titleFont_(makeOwned<CFontDesc>("Helvetica Neue", 40.0, kBoldFace))
    , sectionFont_(makeOwned<CFontDesc>("Helvetica Neue", 17.0, kBoldFace))
    , labelFont_(makeOwned<CFontDesc>("Helvetica Neue", 11.0, kBoldFace))
    , valueFont_(makeOwned<CFontDesc>("Menlo", 10.5, kNormalFace)) {
    setTransparency(false);
    setMouseEnabled(true);
    setWantsFocus(true);
    syncFromBridge();
    timer_ = makeOwned<CVSTGUITimer>([this](CVSTGUITimer*) { syncFromBridge(); }, 33);
}

KiqMainView::~KiqMainView() noexcept {
    if (timer_) {
        timer_->stop();
    }
}

float KiqMainView::value(KickParameterId id) const {
    return values_[parameterIndex(id)];
}

void KiqMainView::setValue(KickParameterId id, float plainValue) {
    const float clamped = clampPlain(id, plainValue);
    values_[parameterIndex(id)] = clamped;
    bridge_.performParameterEdit(id, clamped);
    invalid();
}

void KiqMainView::syncFromBridge() {
    bool changed = false;
    for (const auto& spec : kKickParameterSpecs) {
        const auto index = parameterIndex(spec.id);
        const float next = clampPlain(spec.id, bridge_.getParameter(spec.id));
        if (std::abs(values_[index] - next) > 1.0e-5f) {
            values_[index] = next;
            changed = true;
        }
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

void KiqMainView::draw(CDrawContext* context) {
    if (!context) {
        return;
    }
    context->setDrawMode(kAntiAliasing);
    drawBackground(*context);
    drawHeader(*context);
    drawTrajectory(*context, TrajectoryKind::Pitch);
    drawTrajectory(*context, TrajectoryKind::Amplitude);
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
}

void KiqMainView::drawHeader(CDrawContext& context) {
    drawText(context, titleFont_, 40.0, kText, "K I Q",
             CRect(0.0, 8.0, kDesignWidth, 54.0), kCenterText, kBoldFace);
    drawText(context, labelFont_, 11.0, kText, "K I C K   D E S I G N E R",
             CRect(0.0, 52.0, kDesignWidth, 72.0), kCenterText, kBoldFace);
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

    drawText(context, sectionFont_, 17.0, accent, pitch ? "PITCH" : "AMPLITUDE",
             CRect(42.0, panel.top + 10.0, 240.0, panel.top + 34.0), kLeftText, kBoldFace);

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

const std::array<KickParameterId, 6>& KiqMainView::knobParameterIds() {
    static constexpr std::array<KickParameterId, 6> ids {
        KickParameterId::StartPhase,
        KickParameterId::ClickLevel,
        KickParameterId::NoiseLevel,
        KickParameterId::NoiseDecayMs,
        KickParameterId::NoiseToneHz,
        KickParameterId::OutputGain,
    };
    return ids;
}

std::array<CPoint, 6> KiqMainView::knobCenters() const {
    return {CPoint(96.0, 607.0), CPoint(244.0, 607.0), CPoint(392.0, 607.0),
            CPoint(708.0, 607.0), CPoint(836.0, 607.0), CPoint(995.0, 607.0)};
}

void KiqMainView::drawControls(CDrawContext& context) {
    const CRect panel(14.0, 528.0, kDesignWidth - 14.0, kDesignHeight - 12.0);
    fillRoundedRect(context, panel, 12.0, kPanel, kPanelEdge, 1.2);
    CRect inner = panel;
    inner.inset(4.0, 4.0);
    fillRoundedRect(context, inner, 9.0, CColor(17, 20, 21, 255), kPanelInner, 1.0);

    static constexpr std::array<const char*, 6> labels {
        "START PHASE", "CLICK", "NOISE", "DECAY", "TONE", "OUTPUT",
    };
    const auto centers = knobCenters();
    const auto& ids = knobParameterIds();
    for (std::size_t index = 0; index < ids.size(); ++index) {
        drawKnob(context, ids[index], centers[index], index == 5 ? 49.0 : 37.0, labels[index]);
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

    drawText(context, labelFont_, 10.5, kText, label,
             CRect(center.x - 65.0, 544.0, center.x + 65.0, 561.0),
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
                     CPoint(center.x + std::sin(angle) * pointerLength,
                            center.y - std::cos(angle) * pointerLength));

    char valueBuffer[32] {};
    formatParameter(id, value(id), valueBuffer);
    const double width = radius > 40.0 ? 90.0 : 78.0;
    const CRect valueRect(center.x - width * 0.5, 661.0,
                          center.x + width * 0.5, 681.0);
    fillRoundedRect(context, valueRect, 4.0, CColor(10, 13, 14, 255),
                    CColor(51, 56, 56, 255), 1.0);
    drawText(context, valueFont_, 10.0, kAmplitude, valueBuffer, valueRect, kCenterText);
}

void KiqMainView::drawHitButton(CDrawContext& context) {
    const CRect button(493.0, 554.0, 607.0, 660.0);
    fillRoundedRect(context, button, 10.0,
                    hitPressed_ ? CColor(24, 52, 54, 255) : CColor(9, 12, 13, 255),
                    hitPressed_ ? kAmplitude : CColor(77, 81, 80, 255), 2.0);
    CRect inner = button;
    inner.inset(6.0, 6.0);
    fillRoundedRect(context, inner, 7.0, CColor(19, 23, 24, 255),
                    CColor(4, 6, 7, 255), 1.0);
    context.setFillColor(kAmplitude);
    context.drawRect(CRect(535.0, 568.0, 565.0, 572.0), kDrawFilled);
    drawText(context, titleFont_, 29.0, kText, "HIT",
             CRect(493.0, 593.0, 607.0, 636.0), kCenterText, kBoldFace);
}

void KiqMainView::drawMeter(CDrawContext& context) {
    context.setFrameColor(CColor(74, 78, 77, 255));
    context.setLineWidth(1.0);
    context.drawLine(CPoint(925.0, 548.0), CPoint(925.0, 676.0));

    drawText(context, labelFont_, 9.0, clipHoldFrames_ > 0 ? kRed : kMutedText, "CLIP",
             CRect(1060.0, 548.0, 1090.0, 562.0), kCenterText, kBoldFace);
    constexpr int segments = 7;
    for (int segment = 0; segment < segments; ++segment) {
        const double bottom = 654.0 - segment * 14.0;
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
    const auto centers = knobCenters();
    const auto& ids = knobParameterIds();
    for (std::size_t index = 0; index < ids.size(); ++index) {
        const float radius = index == 5 ? 58.0f : 48.0f;
        if (distanceSquared(where, centers[index]) > radius * radius) {
            continue;
        }
        const KickParameterId id = ids[index];
        if (resetToDefault) {
            bridge_.beginParameterEdit(id);
            setValue(id, getDefaultKickParameter(id));
            bridge_.endParameterEdit(id);
            return true;
        }
        drag_ = {};
        drag_.kind = DragKind::Knob;
        drag_.index = index;
        drag_.startMouse = where;
        drag_.startValue = value(id);
        drag_.parameterIds = {id};
        bridge_.beginParameterEdit(id);
        return true;
    }
    return false;
}

bool KiqMainView::handleHitButton(const CPoint& where) {
    if (!CRect(493.0, 554.0, 607.0, 660.0).pointInside(where)) {
        return false;
    }
    hitPressed_ = true;
    bridge_.triggerAudition();
    invalid();
    return true;
}

void KiqMainView::onMouseDownEvent(MouseDownEvent& event) {
    if (!event.buttonState.isLeft()) {
        return;
    }
    const bool doubleClick = event.clickCount >= 2;
    if (handleHitButton(event.mousePosition) ||
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
    if (drag_.kind == DragKind::Knob) {
        const KickParameterId id = drag_.parameterIds.front();
        const float startNormalized = normalizePlain(id, drag_.startValue);
        const float delta = static_cast<float>((drag_.startMouse.y - where.y) / 135.0) * sensitivity;
        setValue(id, denormalizePlain(id, startNormalized + delta));
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
    updateDrag(event.mousePosition, event.modifiers.has(ModifierKey::Shift));
    event.consumed = true;
}

void KiqMainView::endDrag() {
    for (const auto id : drag_.parameterIds) {
        bridge_.endParameterEdit(id);
    }
    drag_ = {};
}

void KiqMainView::cancelDrag() {
    if (drag_.kind == DragKind::Knob && !drag_.parameterIds.empty()) {
        setValue(drag_.parameterIds.front(), drag_.startValue);
    } else if (drag_.kind == DragKind::Curve && !drag_.parameterIds.empty()) {
        setValue(drag_.parameterIds.front(), drag_.startCurve);
    } else if (drag_.kind == DragKind::Point && !drag_.parameterIds.empty()) {
        setValue(drag_.parameterIds.front(), drag_.startValue);
        if (drag_.parameterIds.size() > 1) {
            setValue(drag_.parameterIds[1], drag_.startTime);
        }
    }
    endDrag();
}

void KiqMainView::onMouseUpEvent(MouseUpEvent& event) {
    if (hitPressed_) {
        hitPressed_ = false;
        invalid();
        event.consumed = true;
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
    const auto centers = knobCenters();
    const auto& ids = knobParameterIds();
    for (std::size_t index = 0; index < ids.size(); ++index) {
        const float radius = index == 5 ? 58.0f : 48.0f;
        if (distanceSquared(event.mousePosition, centers[index]) > radius * radius) {
            continue;
        }
        const KickParameterId id = ids[index];
        const float increment = event.modifiers.has(ModifierKey::Shift) ? 0.002f : 0.015f;
        bridge_.beginParameterEdit(id);
        setValue(id, denormalizePlain(id, normalizePlain(id, value(id)) +
                                             static_cast<float>(event.deltaY) * increment));
        bridge_.endParameterEdit(id);
        event.consumed = true;
        return;
    }
}

void KiqMainView::onKeyboardEvent(KeyboardEvent& event) {
    if (event.type == EventType::KeyDown &&
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
