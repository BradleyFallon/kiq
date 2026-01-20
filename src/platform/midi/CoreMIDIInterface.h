#pragma once

#ifdef __APPLE__

#include <CoreMIDI/CoreMIDI.h>
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace KickDrum {

class MIDIHandler;
class AudioEngine;

/**
 * @brief CoreMIDI integration for macOS MIDI input
 * 
 * This class provides a bridge between CoreMIDI and the AudioEngine's
 * MIDIHandler, handling device enumeration, MIDI input callbacks, and
 * message parsing.
 * 
 * Features:
 * - Enumerate MIDI input devices
 * - Connect to specific MIDI input devices
 * - Parse CoreMIDI packets into MIDIMessage objects
 * - Route MIDI messages to MIDIHandler
 * 
 * Requirements: 9.3, 9.6
 */
class CoreMIDIInterface {
public:
    /**
     * @brief MIDI device information
     */
    struct DeviceInfo {
        MIDIEndpointRef endpoint;
        std::string name;
        std::string manufacturer;
        bool isOnline;
    };

    /**
     * @brief Construct a CoreMIDI interface
     * @param midiHandler Pointer to the MIDI handler to route messages to
     */
    explicit CoreMIDIInterface(MIDIHandler* midiHandler);
    
    /**
     * @brief Destructor - disconnects from devices and cleans up resources
     */
    ~CoreMIDIInterface();

    /**
     * @brief Initialize CoreMIDI
     * @return true if initialization succeeded, false otherwise
     * 
     * This will:
     * - Create a MIDI client
     * - Create an input port
     * - Set up the MIDI read callback
     */
    bool initialize();

    /**
     * @brief Connect to a MIDI input device
     * @param endpoint MIDI endpoint reference to connect to
     * @return true if connected successfully, false otherwise
     */
    bool connectToDevice(MIDIEndpointRef endpoint);

    /**
     * @brief Connect to a MIDI input device by index
     * @param deviceIndex Index in the available devices list
     * @return true if connected successfully, false otherwise
     */
    bool connectToDeviceByIndex(size_t deviceIndex);

    /**
     * @brief Disconnect from the current MIDI input device
     */
    void disconnect();

    /**
     * @brief Check if connected to a MIDI device
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Get the currently connected device endpoint
     * @return MIDI endpoint reference, or 0 if not connected
     */
    MIDIEndpointRef getConnectedDevice() const;

    /**
     * @brief Get list of available MIDI input devices
     * @return Vector of device information
     */
    std::vector<DeviceInfo> getAvailableDevices() const;

    /**
     * @brief Get the number of available MIDI input devices
     * @return Number of devices
     */
    size_t getDeviceCount() const;

    /**
     * @brief Get device information by index
     * @param index Device index
     * @return Device information, or empty DeviceInfo if index out of range
     */
    DeviceInfo getDeviceInfo(size_t index) const;

    /**
     * @brief Set the MIDI handler
     * @param midiHandler Pointer to the MIDI handler
     */
    void setMIDIHandler(MIDIHandler* midiHandler);

    /**
     * @brief Get the MIDI handler
     * @return Pointer to the MIDI handler
     */
    MIDIHandler* getMIDIHandler() const;

private:
    // MIDI handler to route messages to
    MIDIHandler* midiHandler_;

    // CoreMIDI components
    MIDIClientRef midiClient_;
    MIDIPortRef inputPort_;
    MIDIEndpointRef connectedEndpoint_;
    
    // State
    bool initialized_;
    bool connected_;

    // Internal helpers
    void cleanup();
    static std::string getEndpointName(MIDIEndpointRef endpoint);
    static std::string getEndpointManufacturer(MIDIEndpointRef endpoint);
    static bool isEndpointOnline(MIDIEndpointRef endpoint);

    // Static MIDI read callback (C-style function required by CoreMIDI)
    static void midiReadCallback(
        const MIDIPacketList* packetList,
        void* readProcRefCon,
        void* srcConnRefCon);

    // Instance MIDI read method
    void processMIDIPacketList(const MIDIPacketList* packetList);
    void processMIDIPacket(const MIDIPacket* packet);
};

} // namespace KickDrum

#endif // __APPLE__
