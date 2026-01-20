#include "KickSynthProcessor.h"
#include "KickSynthController.h"
#include "KickSynthCIDs.h"
#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "Kick Drum Synthesizer"

//------------------------------------------------------------------------
// VST3 Plugin Factory
//------------------------------------------------------------------------
BEGIN_FACTORY_DEF("KickDrum",
                  "https://kickdrum.com",
                  "mailto:info@kickdrum.com")

    //--- Processor (Audio Component) ---
    DEF_CLASS2(INLINE_UID_FROM_FUID(Steinberg::Vst::kKickSynthProcessorUID),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               stringPluginName,
               Vst::kDistributable,
               Vst::PlugType::kInstrumentSynth,
               "1.0.0",
               kVstVersionString,
               Steinberg::Vst::KickSynthProcessor::createInstance)

    //--- Controller (UI Component) ---
    DEF_CLASS2(INLINE_UID_FROM_FUID(Steinberg::Vst::kKickSynthControllerUID),
               PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               stringPluginName " Controller",
               0,
               "",
               "1.0.0",
               kVstVersionString,
               Steinberg::Vst::KickSynthController::createInstance)

END_FACTORY
