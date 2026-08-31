#pragma once

#ifdef __APPLE__

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace KickDrum {

class AudioEngine;

/**
 * @brief CoreAudio integration for macOS audio output
 * 
 * This class provides a bridge between the AudioEngine and CoreAudio,
 * handling device initialization, buffer configuration, and real-time
 * audio callback routing.
 * 
 * Requirements: 9.2, 9.4, 9.5
 */
class CoreAudioInterface {
public:
    /**
     * @brief Construct a CoreAudio interface
     * @param audioEngine Pointer to the audio engine to render audio from
     */
    explicit CoreAudioInterface(AudioEngine* audioEngine);
    
    /**
     * @brief Destructor - stops audio and cleans up resources
     */
    ~CoreAudioInterface();

    /**
     * @brief Initialize CoreAudio with default output device
     * @return true if initialization succeeded, false otherwise
     * 
     * This will:
     * - Find the default audio output device
     * - Query its sample rate and buffer size
     * - Create and configure an AudioUnit
     * - Set up the render callback
     */
    bool initialize();

    /**
     * @brief Initialize CoreAudio with a specific device
     * @param deviceId CoreAudio device ID
     * @return true if initialization succeeded, false otherwise
     */
    bool initializeWithDevice(AudioDeviceID deviceId);

    /**
     * @brief Start audio processing
     * @return true if started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stop audio processing
     */
    void stop();

    /**
     * @brief Check if audio is currently running
     * @return true if audio is running, false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Get the current sample rate
     * @return Sample rate in Hz (e.g., 44100, 48000)
     */
    double getSampleRate() const;

    /**
     * @brief Get the current buffer size
     * @return Buffer size in frames
     */
    UInt32 getBufferSize() const;

    /**
     * @brief Get the current device ID
     * @return CoreAudio device ID, or 0 if not initialized
     */
    AudioDeviceID getDeviceId() const;

    /**
     * @brief Get the device name
     * @return Device name string, or empty if not initialized
     */
    std::string getDeviceName() const;

    /**
     * @brief Set the buffer size (must be called before start())
     * @param bufferSize Desired buffer size in frames
     * @return true if set successfully, false otherwise
     * 
     * Note: The actual buffer size may differ from the requested size
     * depending on hardware constraints.
     */
    bool setBufferSize(UInt32 bufferSize);

    /**
     * @brief Get list of available audio output devices
     * @return Vector of device IDs
     */
    static std::vector<AudioDeviceID> getAvailableDevices();

    /**
     * @brief Get the name of a device
     * @param deviceId Device ID to query
     * @return Device name, or empty string if device not found
     */
    static std::string getDeviceName(AudioDeviceID deviceId);

    /**
     * @brief Get the default output device ID
     * @return Default device ID, or 0 if none found
     */
    static AudioDeviceID getDefaultOutputDevice();

private:
    // Audio engine to render from
    AudioEngine* audioEngine_;

    // CoreAudio components
    AudioComponentInstance audioUnit_;
    AudioDeviceID deviceId_;
    
    // Audio format information
    double sampleRate_;
    UInt32 bufferSize_;
    UInt32 numChannels_;
    
    // State
    bool initialized_;
    bool running_;
    std::vector<float> interleavedBuffer_;

    // Internal initialization helpers
    bool createAudioUnit();
    bool configureAudioUnit();
    bool queryDeviceFormat();
    void cleanup();

    // Static render callback (C-style function required by CoreAudio)
    static OSStatus renderCallback(
        void* inRefCon,
        AudioUnitRenderActionFlags* ioActionFlags,
        const AudioTimeStamp* inTimeStamp,
        UInt32 inBusNumber,
        UInt32 inNumberFrames,
        AudioBufferList* ioData);

    // Instance render method
    OSStatus render(
        AudioUnitRenderActionFlags* ioActionFlags,
        const AudioTimeStamp* inTimeStamp,
        UInt32 inBusNumber,
        UInt32 inNumberFrames,
        AudioBufferList* ioData);
};

} // namespace KickDrum

#endif // __APPLE__
