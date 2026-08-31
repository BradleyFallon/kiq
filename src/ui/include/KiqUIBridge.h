#pragma once

#include "KickParams.h"

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
    virtual float getOutputPeak() = 0;
    virtual bool getOutputClip() = 0;
};

} // namespace KickDrum::UI
