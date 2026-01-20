#ifdef __APPLE__

#include "StandaloneApp.h"
#include "../platform/audio/CoreAudioInterface.h"
#include "../platform/midi/CoreMIDIInterface.h"
#include "../audio_engine/midi/MIDIHandler.h"
#include "AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace KickDrum {

StandaloneApp::StandaloneApp()
    : audioEngine_(nullptr)
    , midiHandler_(nullptr)
    , audioInterface_(nullptr)
    , midiInterface_(nullptr)
    , running_(false)
{
}

StandaloneApp::~StandaloneApp() {
    shutdown();
}

bool StandaloneApp::initialize() {
    std::cout << "=== Kick Drum Synthesizer ===" << std::endl;
    std::cout << "Initializing..." << std::endl;
    std::cout << std::endl;
    
    // Create audio engine
    audioEngine_ = std::make_unique<AudioEngine>();
    
    // Create CoreAudio interface
    audioInterface_ = std::make_unique<CoreAudioInterface>(audioEngine_.get());
    
    // Initialize CoreAudio
    if (!audioInterface_->initialize()) {
        std::cerr << "ERROR: Failed to initialize audio" << std::endl;
        return false;
    }
    
    std::cout << "Audio initialized:" << std::endl;
    std::cout << "  Device: " << audioInterface_->getDeviceName() << std::endl;
    std::cout << "  Sample Rate: " << audioInterface_->getSampleRate() << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << audioInterface_->getBufferSize() << " frames" << std::endl;
    std::cout << std::endl;
    
    // Create MIDI handler
    midiHandler_ = std::make_unique<MIDIHandler>(
        audioEngine_->getVoiceAllocator(),
        audioEngine_->getParameterManager()
    );
    
    // Create CoreMIDI interface
    midiInterface_ = std::make_unique<CoreMIDIInterface>(midiHandler_.get());
    
    // Initialize CoreMIDI
    if (!midiInterface_->initialize()) {
        std::cerr << "WARNING: Failed to initialize MIDI (continuing without MIDI)" << std::endl;
    } else {
        // List available MIDI devices
        auto devices = midiInterface_->getAvailableDevices();
        
        if (devices.empty()) {
            std::cout << "No MIDI devices found" << std::endl;
        } else {
            std::cout << "Available MIDI devices:" << std::endl;
            for (size_t i = 0; i < devices.size(); ++i) {
                std::cout << "  [" << i << "] " << devices[i].name;
                if (!devices[i].manufacturer.empty()) {
                    std::cout << " (" << devices[i].manufacturer << ")";
                }
                std::cout << (devices[i].isOnline ? " - Online" : " - Offline") << std::endl;
            }
            
            // Connect to first device
            if (!devices.empty()) {
                if (midiInterface_->connectToDeviceByIndex(0)) {
                    std::cout << "Connected to MIDI device: " << devices[0].name << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }
    
    // Start audio
    if (!audioInterface_->start()) {
        std::cerr << "ERROR: Failed to start audio" << std::endl;
        return false;
    }
    
    std::cout << "Audio started successfully" << std::endl;
    std::cout << std::endl;
    
    running_ = true;
    return true;
}

void StandaloneApp::run() {
    if (!running_) {
        std::cerr << "ERROR: Application not initialized" << std::endl;
        return;
    }
    
    printHelp();
    
    // Main loop
    std::string input;
    while (running_) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        processCommand(input);
    }
}

void StandaloneApp::shutdown() {
    if (!running_) {
        return;
    }
    
    std::cout << std::endl;
    std::cout << "Shutting down..." << std::endl;
    
    // Stop audio
    if (audioInterface_) {
        audioInterface_->stop();
    }
    
    // Disconnect MIDI
    if (midiInterface_) {
        midiInterface_->disconnect();
    }
    
    // Clean up
    midiInterface_.reset();
    audioInterface_.reset();
    midiHandler_.reset();
    audioEngine_.reset();
    
    running_ = false;
    
    std::cout << "Goodbye!" << std::endl;
}

void StandaloneApp::printHelp() {
    std::cout << "Commands:" << std::endl;
    std::cout << "  play <note> [velocity]  - Trigger a note (note: 0-127, velocity: 0.0-1.0)" << std::endl;
    std::cout << "  stop <note>             - Release a note" << std::endl;
    std::cout << "  param <name> <value>    - Set a parameter" << std::endl;
    std::cout << "  list                    - List all parameters" << std::endl;
    std::cout << "  preset <name>           - Load a preset" << std::endl;
    std::cout << "  save <name>             - Save current settings as preset" << std::endl;
    std::cout << "  midi                    - List MIDI devices" << std::endl;
    std::cout << "  help                    - Show this help" << std::endl;
    std::cout << "  quit                    - Exit application" << std::endl;
    std::cout << std::endl;
    std::cout << "Example: play 60 0.8" << std::endl;
    std::cout << "Example: param basePitch 50.0" << std::endl;
    std::cout << std::endl;
}

void StandaloneApp::processCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string command;
    iss >> command;
    
    if (command == "quit" || command == "exit" || command == "q") {
        running_ = false;
    }
    else if (command == "help" || command == "h" || command == "?") {
        printHelp();
    }
    else if (command == "play" || command == "p") {
        int note;
        float velocity = 0.8f;
        iss >> note;
        if (iss >> velocity) {
            // Velocity provided
        }
        
        if (note >= 0 && note <= 127) {
            audioEngine_->noteOn(note, velocity);
            std::cout << "Playing note " << note << " with velocity " << velocity << std::endl;
        } else {
            std::cout << "Invalid note number (must be 0-127)" << std::endl;
        }
    }
    else if (command == "stop" || command == "s") {
        int note;
        iss >> note;
        
        if (note >= 0 && note <= 127) {
            audioEngine_->noteOff(note);
            std::cout << "Stopping note " << note << std::endl;
        } else {
            std::cout << "Invalid note number (must be 0-127)" << std::endl;
        }
    }
    else if (command == "param") {
        std::string paramName;
        float value;
        iss >> paramName >> value;
        
        if (!paramName.empty()) {
            audioEngine_->setParameter(paramName, value);
            std::cout << "Set " << paramName << " = " << value << std::endl;
        } else {
            std::cout << "Usage: param <name> <value>" << std::endl;
        }
    }
    else if (command == "list" || command == "l") {
        std::cout << "Available parameters:" << std::endl;
        std::cout << "  basePitch (20-200 Hz)" << std::endl;
        std::cout << "  sineLevel (0-1)" << std::endl;
        std::cout << "  harmonicRatio (0.5-8)" << std::endl;
        std::cout << "  harmonicLevel (0-1)" << std::endl;
        std::cout << "  harmonicModDepth (0-1)" << std::endl;
        std::cout << "  noiseLevel (0-1)" << std::endl;
        std::cout << "  noiseModDepth (0-1)" << std::endl;
        std::cout << "  attack (0-1 seconds)" << std::endl;
        std::cout << "  decay (0-5 seconds)" << std::endl;
        std::cout << "  sustain (0-1)" << std::endl;
        std::cout << "  release (0-5 seconds)" << std::endl;
        std::cout << "  pitchEnvelopeDepth (0-2000 Hz)" << std::endl;
        std::cout << "  masterLevel (0-1)" << std::endl;
    }
    else if (command == "preset") {
        std::string presetName;
        std::getline(iss, presetName);
        // Trim leading whitespace
        size_t start = presetName.find_first_not_of(" \t");
        if (start != std::string::npos) {
            presetName = presetName.substr(start);
        }
        
        if (!presetName.empty()) {
            // TODO: Load preset
            std::cout << "Loading preset: " << presetName << std::endl;
            std::cout << "(Preset loading not yet implemented)" << std::endl;
        } else {
            std::cout << "Usage: preset <name>" << std::endl;
        }
    }
    else if (command == "save") {
        std::string presetName;
        std::getline(iss, presetName);
        // Trim leading whitespace
        size_t start = presetName.find_first_not_of(" \t");
        if (start != std::string::npos) {
            presetName = presetName.substr(start);
        }
        
        if (!presetName.empty()) {
            // TODO: Save preset
            std::cout << "Saving preset: " << presetName << std::endl;
            std::cout << "(Preset saving not yet implemented)" << std::endl;
        } else {
            std::cout << "Usage: save <name>" << std::endl;
        }
    }
    else if (command == "midi" || command == "m") {
        if (midiInterface_) {
            auto devices = midiInterface_->getAvailableDevices();
            
            if (devices.empty()) {
                std::cout << "No MIDI devices found" << std::endl;
            } else {
                std::cout << "Available MIDI devices:" << std::endl;
                for (size_t i = 0; i < devices.size(); ++i) {
                    std::cout << "  [" << i << "] " << devices[i].name;
                    if (!devices[i].manufacturer.empty()) {
                        std::cout << " (" << devices[i].manufacturer << ")";
                    }
                    std::cout << (devices[i].isOnline ? " - Online" : " - Offline");
                    
                    if (midiInterface_->isConnected() && 
                        midiInterface_->getConnectedDevice() == devices[i].endpoint) {
                        std::cout << " [CONNECTED]";
                    }
                    std::cout << std::endl;
                }
            }
        } else {
            std::cout << "MIDI not initialized" << std::endl;
        }
    }
    else {
        std::cout << "Unknown command: " << command << std::endl;
        std::cout << "Type 'help' for available commands" << std::endl;
    }
}

} // namespace KickDrum

#endif // __APPLE__
