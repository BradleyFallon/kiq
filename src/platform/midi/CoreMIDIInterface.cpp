#ifdef __APPLE__

#include "CoreMIDIInterface.h"
#include "../../audio_engine/midi/MIDIHandler.h"
#include "../../audio_engine/midi/MIDIMessage.h"
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>

namespace KickDrum {

CoreMIDIInterface::CoreMIDIInterface(MIDIHandler* midiHandler)
    : midiHandler_(midiHandler)
    , midiClient_(0)
    , inputPort_(0)
    , connectedEndpoint_(0)
    , initialized_(false)
    , connected_(false)
{
}

CoreMIDIInterface::~CoreMIDIInterface() {
    cleanup();
}

bool CoreMIDIInterface::initialize() {
    if (initialized_) {
        return true;
    }

    // Create MIDI client
    CFStringRef clientName = CFStringCreateWithCString(nullptr, "KickDrumSynth", kCFStringEncodingUTF8);
    OSStatus status = MIDIClientCreate(clientName, nullptr, nullptr, &midiClient_);
    CFRelease(clientName);

    if (status != noErr) {
        std::cerr << "CoreMIDIInterface: Failed to create MIDI client (error " << status << ")" << std::endl;
        return false;
    }

    // Create input port
    CFStringRef portName = CFStringCreateWithCString(nullptr, "KickDrumInput", kCFStringEncodingUTF8);
    status = MIDIInputPortCreate(midiClient_, portName, midiReadCallback, this, &inputPort_);
    CFRelease(portName);

    if (status != noErr) {
        std::cerr << "CoreMIDIInterface: Failed to create input port (error " << status << ")" << std::endl;
        MIDIClientDispose(midiClient_);
        midiClient_ = 0;
        return false;
    }

    initialized_ = true;
    return true;
}

bool CoreMIDIInterface::connectToDevice(MIDIEndpointRef endpoint) {
    if (!initialized_) {
        std::cerr << "CoreMIDIInterface: Cannot connect - not initialized" << std::endl;
        return false;
    }

    // Disconnect from current device if connected
    if (connected_) {
        disconnect();
    }

    // Connect to the new endpoint
    OSStatus status = MIDIPortConnectSource(inputPort_, endpoint, nullptr);
    if (status != noErr) {
        std::cerr << "CoreMIDIInterface: Failed to connect to device (error " << status << ")" << std::endl;
        return false;
    }

    connectedEndpoint_ = endpoint;
    connected_ = true;

    std::string deviceName = getEndpointName(endpoint);
    std::cout << "CoreMIDIInterface: Connected to MIDI device: " << deviceName << std::endl;

    return true;
}

bool CoreMIDIInterface::connectToDeviceByIndex(size_t deviceIndex) {
    std::vector<DeviceInfo> devices = getAvailableDevices();
    
    if (deviceIndex >= devices.size()) {
        std::cerr << "CoreMIDIInterface: Device index " << deviceIndex << " out of range" << std::endl;
        return false;
    }

    return connectToDevice(devices[deviceIndex].endpoint);
}

void CoreMIDIInterface::disconnect() {
    if (!connected_) {
        return;
    }

    if (inputPort_ != 0 && connectedEndpoint_ != 0) {
        MIDIPortDisconnectSource(inputPort_, connectedEndpoint_);
    }

    connectedEndpoint_ = 0;
    connected_ = false;

    std::cout << "CoreMIDIInterface: Disconnected from MIDI device" << std::endl;
}

bool CoreMIDIInterface::isConnected() const {
    return connected_;
}

MIDIEndpointRef CoreMIDIInterface::getConnectedDevice() const {
    return connectedEndpoint_;
}

std::vector<CoreMIDIInterface::DeviceInfo> CoreMIDIInterface::getAvailableDevices() const {
    std::vector<DeviceInfo> devices;

    ItemCount numSources = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < numSources; ++i) {
        MIDIEndpointRef endpoint = MIDIGetSource(i);
        if (endpoint != 0) {
            DeviceInfo info;
            info.endpoint = endpoint;
            info.name = getEndpointName(endpoint);
            info.manufacturer = getEndpointManufacturer(endpoint);
            info.isOnline = isEndpointOnline(endpoint);
            devices.push_back(info);
        }
    }

    return devices;
}

size_t CoreMIDIInterface::getDeviceCount() const {
    return static_cast<size_t>(MIDIGetNumberOfSources());
}

CoreMIDIInterface::DeviceInfo CoreMIDIInterface::getDeviceInfo(size_t index) const {
    if (index >= getDeviceCount()) {
        return DeviceInfo{0, "", "", false};
    }

    MIDIEndpointRef endpoint = MIDIGetSource(static_cast<ItemCount>(index));
    DeviceInfo info;
    info.endpoint = endpoint;
    info.name = getEndpointName(endpoint);
    info.manufacturer = getEndpointManufacturer(endpoint);
    info.isOnline = isEndpointOnline(endpoint);
    return info;
}

void CoreMIDIInterface::setMIDIHandler(MIDIHandler* midiHandler) {
    midiHandler_ = midiHandler;
}

MIDIHandler* CoreMIDIInterface::getMIDIHandler() const {
    return midiHandler_;
}

void CoreMIDIInterface::cleanup() {
    disconnect();

    if (inputPort_ != 0) {
        MIDIPortDispose(inputPort_);
        inputPort_ = 0;
    }

    if (midiClient_ != 0) {
        MIDIClientDispose(midiClient_);
        midiClient_ = 0;
    }

    initialized_ = false;
}

std::string CoreMIDIInterface::getEndpointName(MIDIEndpointRef endpoint) {
    CFStringRef nameRef = nullptr;
    OSStatus status = MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &nameRef);
    
    if (status != noErr || nameRef == nullptr) {
        return "Unknown Device";
    }

    char name[256];
    CFStringGetCString(nameRef, name, sizeof(name), kCFStringEncodingUTF8);
    CFRelease(nameRef);

    return std::string(name);
}

std::string CoreMIDIInterface::getEndpointManufacturer(MIDIEndpointRef endpoint) {
    CFStringRef manufacturerRef = nullptr;
    OSStatus status = MIDIObjectGetStringProperty(endpoint, kMIDIPropertyManufacturer, &manufacturerRef);
    
    if (status != noErr || manufacturerRef == nullptr) {
        return "";
    }

    char manufacturer[256];
    CFStringGetCString(manufacturerRef, manufacturer, sizeof(manufacturer), kCFStringEncodingUTF8);
    CFRelease(manufacturerRef);

    return std::string(manufacturer);
}

bool CoreMIDIInterface::isEndpointOnline(MIDIEndpointRef endpoint) {
    SInt32 isOffline = 0;
    OSStatus status = MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyOffline, &isOffline);
    
    if (status != noErr) {
        return true; // Assume online if property not available
    }

    return (isOffline == 0);
}

void CoreMIDIInterface::midiReadCallback(
    const MIDIPacketList* packetList,
    void* readProcRefCon,
    void* srcConnRefCon)
{
    // readProcRefCon is the CoreMIDIInterface instance pointer
    CoreMIDIInterface* interface = static_cast<CoreMIDIInterface*>(readProcRefCon);
    if (interface != nullptr) {
        interface->processMIDIPacketList(packetList);
    }
}

void CoreMIDIInterface::processMIDIPacketList(const MIDIPacketList* packetList) {
    if (midiHandler_ == nullptr) {
        return;
    }

    const MIDIPacket* packet = &packetList->packet[0];
    for (UInt32 i = 0; i < packetList->numPackets; ++i) {
        processMIDIPacket(packet);
        packet = MIDIPacketNext(packet);
    }
}

void CoreMIDIInterface::processMIDIPacket(const MIDIPacket* packet) {
    if (midiHandler_ == nullptr || packet->length == 0) {
        return;
    }

    // Parse MIDI message from packet data
    const Byte* data = packet->data;
    UInt16 length = packet->length;

    // Process each MIDI message in the packet
    // (A packet can contain multiple messages)
    for (UInt16 i = 0; i < length; ) {
        Byte statusByte = data[i];
        
        // Skip system real-time messages (0xF8-0xFF)
        if (statusByte >= 0xF8) {
            i++;
            continue;
        }

        // Extract message type and channel
        Byte messageType = statusByte & 0xF0;
        Byte channel = statusByte & 0x0F;

        MIDIMessage message;
        message.channel = channel;
        message.timestamp = packet->timeStamp;

        // Parse based on message type
        switch (messageType) {
            case 0x80: // Note Off
                if (i + 2 < length) {
                    message.type = MIDIMessageType::NOTE_OFF;
                    message.data1 = data[i + 1] & 0x7F; // Note number
                    message.data2 = data[i + 2] & 0x7F; // Velocity
                    midiHandler_->processMIDIMessage(message);
                    i += 3;
                } else {
                    i = length; // Invalid message, skip rest
                }
                break;

            case 0x90: // Note On
                if (i + 2 < length) {
                    Byte velocity = data[i + 2] & 0x7F;
                    if (velocity == 0) {
                        // Note on with velocity 0 is treated as note off
                        message.type = MIDIMessageType::NOTE_OFF;
                    } else {
                        message.type = MIDIMessageType::NOTE_ON;
                    }
                    message.data1 = data[i + 1] & 0x7F; // Note number
                    message.data2 = velocity;
                    midiHandler_->processMIDIMessage(message);
                    i += 3;
                } else {
                    i = length; // Invalid message, skip rest
                }
                break;

            case 0xB0: // Control Change
                if (i + 2 < length) {
                    message.type = MIDIMessageType::CC;
                    message.data1 = data[i + 1] & 0x7F; // CC number
                    message.data2 = data[i + 2] & 0x7F; // CC value
                    midiHandler_->processMIDIMessage(message);
                    i += 3;
                } else {
                    i = length; // Invalid message, skip rest
                }
                break;

            case 0xE0: // Pitch Bend
                if (i + 2 < length) {
                    message.type = MIDIMessageType::PITCH_BEND;
                    message.data1 = data[i + 1] & 0x7F; // LSB
                    message.data2 = data[i + 2] & 0x7F; // MSB
                    midiHandler_->processMIDIMessage(message);
                    i += 3;
                } else {
                    i = length; // Invalid message, skip rest
                }
                break;

            case 0xC0: // Program Change
            case 0xD0: // Channel Pressure
                // These are 2-byte messages, skip for now
                i += 2;
                break;

            case 0xA0: // Polyphonic Key Pressure
                // 3-byte message, skip for now
                i += 3;
                break;

            case 0xF0: // System messages
                // Skip system messages for now
                // (Would need to handle variable-length SysEx)
                i++;
                break;

            default:
                // Unknown message type, skip
                i++;
                break;
        }
    }
}

} // namespace KickDrum

#endif // __APPLE__
