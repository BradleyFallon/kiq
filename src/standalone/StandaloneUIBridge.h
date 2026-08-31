#pragma once

#include "KiqUIBridge.h"

#include <array>
#include <cstddef>
#include <memory>

namespace KickDrum {

class AudioEngine;
class CoreAudioInterface;
class CoreMIDIInterface;
class MIDIHandler;

namespace Standalone {

/** Owns the native audio/MIDI shell and adapts it to the shared editor. */
class StandaloneUIBridge final : public UI::KiqUIBridge {
public:
    StandaloneUIBridge();
    ~StandaloneUIBridge() override;

    bool initialize();
    void shutdown();

    float getParameter(KickParameterId id) override;
    void beginParameterEdit(KickParameterId id) override;
    void performParameterEdit(KickParameterId id, float plainValue) override;
    void endParameterEdit(KickParameterId id) override;
    void triggerAudition() override;
    float getOutputPeak() override;
    bool getOutputClip() override;

private:
    static constexpr std::size_t kParameterCount =
        static_cast<std::size_t>(KickParameterId::Count);

    std::array<float, kParameterCount> values_ {};
    std::unique_ptr<AudioEngine> audioEngine_;
    std::unique_ptr<MIDIHandler> midiHandler_;
    std::unique_ptr<CoreAudioInterface> audioInterface_;
    std::unique_ptr<CoreMIDIInterface> midiInterface_;
};

} // namespace Standalone
} // namespace KickDrum
