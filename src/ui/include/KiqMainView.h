#pragma once

#include "KiqUIBridge.h"

#include "KickParamsHistory.h"
#include "ReferenceAudio.h"

#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cview.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/lib/dragging.h"

#include <atomic>
#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace KickDrum::UI {

class KiqMainView final : public VSTGUI::CView,
                          public VSTGUI::IDropTarget {
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
    VSTGUI::SharedPointer<VSTGUI::IDropTarget> getDropTarget() override {
        return this;
    }
    VSTGUI::DragOperation onDragEnter(VSTGUI::DragEventData data) override;
    VSTGUI::DragOperation onDragMove(VSTGUI::DragEventData data) override;
    void onDragLeave(VSTGUI::DragEventData data) override;
    bool onDrop(VSTGUI::DragEventData data) override;

private:
    enum class TrajectoryKind { Pitch, Amplitude };
    enum class ControlPage { Model, Output };
    enum class DragKind { None, Point, Curve, Knob, Tempo, PhaseLock, Export };

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

    struct KnobDefinition {
        KickParameterId id = KickParameterId::StrikePosition;
        VSTGUI::CPoint center;
        const char* label = "";
        double radius = 29.0;
    };

    struct ReferenceImportResult {
        std::string path;
        std::optional<KickReferenceAnalysis> analysis;
        std::string error;
    };

    struct AsyncCallbackState {
        std::atomic<KiqMainView*> view {nullptr};
        std::mutex referenceMutex;
        std::optional<ReferenceImportResult> completedReference;
        bool referenceInFlight = false;
    };

    KiqUIBridge& bridge_;
    std::shared_ptr<AsyncCallbackState> callbackState_;
    std::thread referenceWorker_;
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
    KickParamsHistory history_;
    std::optional<KickReferenceAnalysis> referenceAnalysis_;
    std::shared_ptr<const SampleLayerData> sampleLayer_;
    std::string sampleSourcePath_;
    std::string presetName_ {"Init — Bass House"};
    std::string statusMessage_;
    std::string temporaryExportPath_;
    std::size_t waveformBinsUsed_ = 0;
    float waveformDurationMs_ = 0.0f;
    float waveformPeak_ = 0.0f;
    float loopBpm_ = 120.0f;
    int clipHoldFrames_ = 0;
    int statusFrames_ = 0;
    bool hitPressed_ = false;
    bool waveformDirty_ = true;
    bool loopEnabled_ = false;
    bool dropHover_ = false;
    ControlPage controlPage_ = ControlPage::Model;

    float value(KickParameterId id) const;
    void setValue(KickParameterId id, float plainValue);
    KickParams currentParams() const;
    void applyParams(const KickParams& params, bool recordHistory = true);
    void recordCurrentState();
    void undo();
    void redo();
    void syncFromBridge();
    void timerTick();
    void consumeReferenceImport();
    void rebuildWaveformPreview();
    void setLoopBpm(float bpm);
    void setLoopEnabled(bool enabled);

    void drawBackground(VSTGUI::CDrawContext& context);
    void drawHeader(VSTGUI::CDrawContext& context);
    void drawWorkflowButton(VSTGUI::CDrawContext& context,
                            const VSTGUI::CRect& rect, const char* label,
                            bool active = true, bool accent = false);
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
    bool beginPhaseLockDrag(const VSTGUI::CPoint& where);
    bool handleHitButton(const VSTGUI::CPoint& where);
    bool handleLoopButton(const VSTGUI::CPoint& where);
    bool handleWorkflowButton(const VSTGUI::CPoint& where);
    bool handleControlPageButton(const VSTGUI::CPoint& where);
    void updateDrag(const VSTGUI::CPoint& where, bool fineAdjustment);
    void endDrag();
    void cancelDrag();

    const std::array<KnobDefinition, 8>& knobDefinitions() const;
    std::size_t knobCount() const;

    void setStatus(std::string message);
    void showPresetMenu();
    void applyFactoryPreset(std::size_t index);
    void openPresetSaveSelector();
    void openPresetLoadSelector();
    void savePreset(const std::string& path);
    void loadPreset(const std::string& path);
    void installSampleLayer(std::shared_ptr<const SampleLayerData> sampleLayer);
    void openReferenceSelector();
    bool importReference(const std::string& path);
    void applyReferenceFit();
    void alignReferencePhase();
    void openExportSelector();
    bool exportWav(const std::string& path);
    void startExportDrag();
    static std::optional<std::string> firstWavPath(VSTGUI::IDataPackage* package);
};

} // namespace KickDrum::UI
