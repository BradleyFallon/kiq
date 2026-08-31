#pragma once

#include "KiqUIBridge.h"

#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cview.h"
#include "vstgui/lib/cvstguitimer.h"

#include <array>
#include <cstddef>
#include <vector>

namespace KickDrum::UI {

class KiqMainView final : public VSTGUI::CView {
public:
    static constexpr VSTGUI::CCoord kDesignWidth = 1100.0;
    static constexpr VSTGUI::CCoord kDesignHeight = 860.0;

    KiqMainView(const VSTGUI::CRect& size, KiqUIBridge& bridge);
    ~KiqMainView() noexcept override;

    void draw(VSTGUI::CDrawContext* context) override;
    void onMouseDownEvent(VSTGUI::MouseDownEvent& event) override;
    void onMouseMoveEvent(VSTGUI::MouseMoveEvent& event) override;
    void onMouseUpEvent(VSTGUI::MouseUpEvent& event) override;
    void onMouseCancelEvent(VSTGUI::MouseCancelEvent& event) override;
    void onMouseWheelEvent(VSTGUI::MouseWheelEvent& event) override;
    void onKeyboardEvent(VSTGUI::KeyboardEvent& event) override;

private:
    enum class TrajectoryKind { Pitch, Amplitude };
    enum class DragKind { None, Point, Curve, Knob, Tempo };

    static constexpr std::size_t kWaveformBinCount = 760;

    struct DragState {
        DragKind kind = DragKind::None;
        TrajectoryKind trajectory = TrajectoryKind::Pitch;
        std::size_t index = 0;
        VSTGUI::CPoint startMouse;
        VSTGUI::CPoint lastMouse;
        float startValue = 0.0f;
        float startTime = 0.0f;
        float startCurve = 0.0f;
        float frozenTimeMax = 0.0f;
        std::vector<KickParameterId> parameterIds;
    };

    KiqUIBridge& bridge_;
    std::array<float, static_cast<std::size_t>(KickParameterId::Count)> values_ {};
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer_;
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> titleFont_;
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> sectionFont_;
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> labelFont_;
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> valueFont_;
    DragState drag_;
    float displayedPeak_ = 0.0f;
    std::array<float, kWaveformBinCount> waveformSamples_ {};
    std::array<float, kWaveformBinCount> tuningSamplesHz_ {};
    std::size_t waveformBinsUsed_ = 0;
    float waveformDurationMs_ = 0.0f;
    float waveformPeak_ = 0.0f;
    float loopBpm_ = 120.0f;
    int clipHoldFrames_ = 0;
    bool hitPressed_ = false;
    bool waveformDirty_ = true;
    bool loopEnabled_ = false;

    float value(KickParameterId id) const;
    void setValue(KickParameterId id, float plainValue);
    void syncFromBridge();
    void timerTick();
    void rebuildWaveformPreview();
    void setLoopBpm(float bpm);
    void setLoopEnabled(bool enabled);

    void drawBackground(VSTGUI::CDrawContext& context);
    void drawHeader(VSTGUI::CDrawContext& context);
    void drawTrajectory(VSTGUI::CDrawContext& context, TrajectoryKind kind);
    void drawWaveformPreview(VSTGUI::CDrawContext& context);
    void drawLoopControls(VSTGUI::CDrawContext& context);
    void drawControls(VSTGUI::CDrawContext& context);
    void drawKnob(VSTGUI::CDrawContext& context, KickParameterId id,
                  const VSTGUI::CPoint& center, double radius, const char* label);
    void drawHitButton(VSTGUI::CDrawContext& context);
    void drawMeter(VSTGUI::CDrawContext& context);

    VSTGUI::CRect trajectoryPanel(TrajectoryKind kind) const;
    VSTGUI::CRect trajectoryGraph(TrajectoryKind kind) const;
    VSTGUI::CPoint trajectoryPoint(TrajectoryKind kind, std::size_t index,
                                   float timeMax = 0.0f) const;
    VSTGUI::CPoint curveHandlePoint(TrajectoryKind kind, std::size_t segment,
                                   float timeMax = 0.0f) const;
    float trajectoryTimeMax(TrajectoryKind kind) const;

    bool beginPointDrag(const VSTGUI::CPoint& where);
    bool beginCurveDrag(const VSTGUI::CPoint& where);
    bool beginKnobDrag(const VSTGUI::CPoint& where, bool resetToDefault);
    bool beginTempoDrag(const VSTGUI::CPoint& where, bool resetToDefault);
    bool handleHitButton(const VSTGUI::CPoint& where);
    bool handleLoopButton(const VSTGUI::CPoint& where);
    void updateDrag(const VSTGUI::CPoint& where, bool fineAdjustment);
    void endDrag();
    void cancelDrag();

    std::array<VSTGUI::CPoint, 6> knobCenters() const;
    static const std::array<KickParameterId, 6>& knobParameterIds();
};

} // namespace KickDrum::UI
