#pragma once

#ifdef __APPLE__

#include <memory>
#include <string>
#include <sstream>

namespace KickDrum {

class AudioEngine;
class MIDIHandler;
class CoreAudioInterface;
class CoreMIDIInterface;

/**
 * @brief Standalone macOS application for Kick Drum Synthesizer
 * 
 * This class provides a simple terminal-based interface for testing
 * the synthesizer. It integrates CoreAudio for audio output and
 * CoreMIDI for MIDI input.
 * 
 * Features:
 * - Audio output via CoreAudio
 * - MIDI input via CoreMIDI
 * - Command-line interface for testing
 * - Parameter control
 * - Note triggering
 * 
 * Requirements: 9.1, 9.2, 9.3, 9.4, 9.5, 9.6, 9.7, 9.8
 */
class StandaloneApp {
public:
    /**
     * @brief Construct the standalone application
     */
    StandaloneApp();
    
    /**
     * @brief Destructor - cleans up resources
     */
    ~StandaloneApp();

    /**
     * @brief Initialize the application
     * @return true if initialization succeeded, false otherwise
     * 
     * This will:
     * - Create and initialize the audio engine
     * - Initialize CoreAudio for audio output
     * - Initialize CoreMIDI for MIDI input
     * - Connect to available MIDI devices
     * - Start audio processing
     */
    bool initialize();

    /**
     * @brief Run the application main loop
     * 
     * This enters a command-line interface where the user can:
     * - Trigger notes
     * - Adjust parameters
     * - Load/save presets
     * - List MIDI devices
     */
    void run();

    /**
     * @brief Shutdown the application
     * 
     * This will:
     * - Stop audio processing
     * - Disconnect from MIDI devices
     * - Clean up resources
     */
    void shutdown();

private:
    // Core components
    std::unique_ptr<AudioEngine> audioEngine_;
    std::unique_ptr<MIDIHandler> midiHandler_;
    std::unique_ptr<CoreAudioInterface> audioInterface_;
    std::unique_ptr<CoreMIDIInterface> midiInterface_;
    
    // State
    bool running_;

    // Command processing
    void printHelp();
    void processCommand(const std::string& input);
};

} // namespace KickDrum

#endif // __APPLE__
