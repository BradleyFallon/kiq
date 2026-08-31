#pragma once

#include "KickParams.h"

#include <memory>

namespace KickDrum {
struct SampleLayerData;
}

namespace KickDrum::UI {

/** Host-facing bridge used by the shared VST3 and standalone editor. */
class KiqUIBridge {
public:
    virtual ~KiqUIBridge() = default;

    virtual float getParameter(KickParameterId id) = 0;
    virtual void beginParameterEdit(KickParameterId id) = 0;
    virtual void performParameterEdit(KickParameterId id, float plainValue) = 0;
    virtual void endParameterEdit(KickParameterId id) = 0;

    virtual void triggerAudition() = 0;
    virtual void setAuditionLoop(bool enabled, float bpm) = 0;
    /** Replace the optional immutable sample layer for subsequent hits. */
    virtual void setSampleLayer(
        std::shared_ptr<const SampleLayerData> sampleLayer) = 0;
    /** Return the layer currently associated with editor/processor state. */
    virtual std::shared_ptr<const SampleLayerData> getSampleLayer() const = 0;
    virtual float getOutputPeak() = 0;
    virtual bool getOutputClip() = 0;
};

} // namespace KickDrum::UI
