#pragma once

#include "KiqUIBridge.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Kick Drum Synthesizer VST3 Controller
 * 
 * This is the UI/parameter management component of the VST3 plugin.
 * It handles:
 * - Parameter registration and management
 * - UI creation and updates
 * - Communication with the processor
 */
class KickSynthController : public EditController,
                            public VSTGUI::VST3EditorDelegate,
                            public KickDrum::UI::KiqUIBridge
{
public:
    KickSynthController();
    ~KickSynthController() override;

    // Create function used by factory
    static FUnknown* createInstance(void* /*context*/)
    {
        return (IEditController*)new KickSynthController;
    }

    //--- EditController overrides --------
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
    
    //--- IEditController overrides --------
    IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;

    //--- VSTGUI::VST3EditorDelegate --------
    VSTGUI::CView* createCustomView(
        VSTGUI::UTF8StringPtr name,
        const VSTGUI::UIAttributes& attributes,
        const VSTGUI::IUIDescription* description,
        VSTGUI::VST3Editor* editor) override;

    //--- KickDrum::UI::KiqUIBridge --------
    float getParameter(KickDrum::KickParameterId id) override;
    void beginParameterEdit(KickDrum::KickParameterId id) override;
    void performParameterEdit(KickDrum::KickParameterId id, float plainValue) override;
    void endParameterEdit(KickDrum::KickParameterId id) override;
    void triggerAudition() override;
    float getOutputPeak() override;
    bool getOutputClip() override;

private:
    // Helper to register all parameters
    void registerParameters();
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
