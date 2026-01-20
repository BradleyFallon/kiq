#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

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
class KickSynthController : public EditController
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

private:
    // Helper to register all parameters
    void registerParameters();
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
