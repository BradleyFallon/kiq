#ifdef __APPLE__

#include "CoreAudioInterface.h"
#include "AudioEngine.h"
#include <iostream>
#include <vector>
#include <cstring>

namespace KickDrum {

CoreAudioInterface::CoreAudioInterface(AudioEngine* audioEngine)
    : audioEngine_(audioEngine)
    , audioUnit_(nullptr)
    , deviceId_(0)
    , sampleRate_(48000.0)
    , bufferSize_(512)
    , numChannels_(2)
    , initialized_(false)
    , running_(false)
{
}

CoreAudioInterface::~CoreAudioInterface() {
    stop();
    cleanup();
}

bool CoreAudioInterface::initialize() {
    // Get default output device
    AudioDeviceID defaultDevice = getDefaultOutputDevice();
    if (defaultDevice == 0) {
        std::cerr << "CoreAudioInterface: Failed to find default output device" << std::endl;
        return false;
    }
    
    return initializeWithDevice(defaultDevice);
}

bool CoreAudioInterface::initializeWithDevice(AudioDeviceID deviceId) {
    if (initialized_) {
        std::cerr << "CoreAudioInterface: Already initialized" << std::endl;
        return false;
    }
    
    if (running_) {
        std::cerr << "CoreAudioInterface: Cannot initialize while running" << std::endl;
        return false;
    }
    
    deviceId_ = deviceId;
    
    // Query device format (sample rate, channels)
    if (!queryDeviceFormat()) {
        std::cerr << "CoreAudioInterface: Failed to query device format" << std::endl;
        return false;
    }
    
    // Create and configure AudioUnit
    if (!createAudioUnit()) {
        std::cerr << "CoreAudioInterface: Failed to create AudioUnit" << std::endl;
        cleanup();
        return false;
    }
    
    if (!configureAudioUnit()) {
        std::cerr << "CoreAudioInterface: Failed to configure AudioUnit" << std::endl;
        cleanup();
        return false;
    }
    
    // Initialize AudioUnit
    OSStatus status = AudioUnitInitialize(audioUnit_);
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: AudioUnitInitialize failed with status " << status << std::endl;
        cleanup();
        return false;
    }
    
    // Initialize audio engine with device sample rate
    if (audioEngine_) {
        audioEngine_->initialize(static_cast<float>(sampleRate_));
        std::cout << "CoreAudioInterface: Initialized audio engine at " << sampleRate_ << " Hz" << std::endl;
    }
    
    initialized_ = true;
    
    std::cout << "CoreAudioInterface: Successfully initialized" << std::endl;
    std::cout << "  Device: " << getDeviceName() << std::endl;
    std::cout << "  Sample Rate: " << sampleRate_ << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << bufferSize_ << " frames" << std::endl;
    std::cout << "  Channels: " << numChannels_ << std::endl;
    
    return true;
}

bool CoreAudioInterface::start() {
    if (!initialized_) {
        std::cerr << "CoreAudioInterface: Not initialized" << std::endl;
        return false;
    }
    
    if (running_) {
        std::cerr << "CoreAudioInterface: Already running" << std::endl;
        return false;
    }
    
    OSStatus status = AudioOutputUnitStart(audioUnit_);
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: AudioOutputUnitStart failed with status " << status << std::endl;
        return false;
    }
    
    running_ = true;
    std::cout << "CoreAudioInterface: Audio started" << std::endl;
    return true;
}

void CoreAudioInterface::stop() {
    if (!running_) {
        return;
    }
    
    if (audioUnit_) {
        AudioOutputUnitStop(audioUnit_);
    }
    
    running_ = false;
    std::cout << "CoreAudioInterface: Audio stopped" << std::endl;
}

bool CoreAudioInterface::isRunning() const {
    return running_;
}

double CoreAudioInterface::getSampleRate() const {
    return sampleRate_;
}

UInt32 CoreAudioInterface::getBufferSize() const {
    return bufferSize_;
}

AudioDeviceID CoreAudioInterface::getDeviceId() const {
    return deviceId_;
}

std::string CoreAudioInterface::getDeviceName() const {
    if (deviceId_ == 0) {
        return "";
    }
    return getDeviceName(deviceId_);
}

bool CoreAudioInterface::setBufferSize(UInt32 bufferSize) {
    if (running_) {
        std::cerr << "CoreAudioInterface: Cannot change buffer size while running" << std::endl;
        return false;
    }
    
    if (deviceId_ == 0) {
        // Not initialized yet, just store the value
        bufferSize_ = bufferSize;
        return true;
    }
    
    // Try to set the buffer size on the device
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyBufferFrameSize,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    
    UInt32 requestedSize = bufferSize;
    OSStatus status = AudioObjectSetPropertyData(
        deviceId_,
        &propertyAddress,
        0,
        nullptr,
        sizeof(UInt32),
        &requestedSize
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to set buffer size, status " << status << std::endl;
        return false;
    }
    
    // Read back the actual buffer size
    UInt32 actualSize = 0;
    UInt32 propertySize = sizeof(UInt32);
    status = AudioObjectGetPropertyData(
        deviceId_,
        &propertyAddress,
        0,
        nullptr,
        &propertySize,
        &actualSize
    );
    
    if (status == noErr) {
        bufferSize_ = actualSize;
        std::cout << "CoreAudioInterface: Buffer size set to " << bufferSize_ << " frames" << std::endl;
        return true;
    }
    
    return false;
}

std::vector<AudioDeviceID> CoreAudioInterface::getAvailableDevices() {
    std::vector<AudioDeviceID> devices;
    
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    
    UInt32 propertySize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(
        kAudioObjectSystemObject,
        &propertyAddress,
        0,
        nullptr,
        &propertySize
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to get device list size" << std::endl;
        return devices;
    }
    
    UInt32 deviceCount = propertySize / sizeof(AudioDeviceID);
    if (deviceCount == 0) {
        return devices;
    }
    
    devices.resize(deviceCount);
    status = AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &propertyAddress,
        0,
        nullptr,
        &propertySize,
        devices.data()
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to get device list" << std::endl;
        devices.clear();
        return devices;
    }
    
    // Filter to only output devices
    std::vector<AudioDeviceID> outputDevices;
    for (AudioDeviceID deviceId : devices) {
        // Check if device has output streams
        AudioObjectPropertyAddress streamAddress = {
            kAudioDevicePropertyStreams,
            kAudioDevicePropertyScopeOutput,
            kAudioObjectPropertyElementMain
        };
        
        UInt32 streamSize = 0;
        status = AudioObjectGetPropertyDataSize(
            deviceId,
            &streamAddress,
            0,
            nullptr,
            &streamSize
        );
        
        if (status == noErr && streamSize > 0) {
            outputDevices.push_back(deviceId);
        }
    }
    
    return outputDevices;
}

std::string CoreAudioInterface::getDeviceName(AudioDeviceID deviceId) {
    if (deviceId == 0) {
        return "";
    }
    
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDeviceNameCFString,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    
    CFStringRef deviceName = nullptr;
    UInt32 propertySize = sizeof(CFStringRef);
    
    OSStatus status = AudioObjectGetPropertyData(
        deviceId,
        &propertyAddress,
        0,
        nullptr,
        &propertySize,
        &deviceName
    );
    
    if (status != noErr || deviceName == nullptr) {
        return "";
    }
    
    // Convert CFString to std::string
    char buffer[256];
    Boolean success = CFStringGetCString(
        deviceName,
        buffer,
        sizeof(buffer),
        kCFStringEncodingUTF8
    );
    
    CFRelease(deviceName);
    
    if (success) {
        return std::string(buffer);
    }
    
    return "";
}

AudioDeviceID CoreAudioInterface::getDefaultOutputDevice() {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    
    AudioDeviceID deviceId = 0;
    UInt32 propertySize = sizeof(AudioDeviceID);
    
    OSStatus status = AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &propertyAddress,
        0,
        nullptr,
        &propertySize,
        &deviceId
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to get default output device" << std::endl;
        return 0;
    }
    
    return deviceId;
}

bool CoreAudioInterface::createAudioUnit() {
    // Describe the output audio unit
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    // Find the component
    AudioComponent component = AudioComponentFindNext(nullptr, &desc);
    if (component == nullptr) {
        std::cerr << "CoreAudioInterface: Failed to find output audio component" << std::endl;
        return false;
    }
    
    // Create an instance
    OSStatus status = AudioComponentInstanceNew(component, &audioUnit_);
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to create audio unit instance, status " << status << std::endl;
        return false;
    }
    
    return true;
}

bool CoreAudioInterface::configureAudioUnit() {
    if (audioUnit_ == nullptr) {
        return false;
    }
    
    // Set up the audio format
    AudioStreamBasicDescription format;
    format.mSampleRate = sampleRate_;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
    format.mBytesPerPacket = sizeof(float);
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = sizeof(float);
    format.mChannelsPerFrame = numChannels_;
    format.mBitsPerChannel = 32;
    format.mReserved = 0;
    
    // Set the format on the output scope of the audio unit
    OSStatus status = AudioUnitSetProperty(
        audioUnit_,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        0,
        &format,
        sizeof(format)
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to set stream format, status " << status << std::endl;
        return false;
    }
    
    // Set up the render callback
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = renderCallback;
    callbackStruct.inputProcRefCon = this;
    
    status = AudioUnitSetProperty(
        audioUnit_,
        kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input,
        0,
        &callbackStruct,
        sizeof(callbackStruct)
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to set render callback, status " << status << std::endl;
        return false;
    }
    
    return true;
}

bool CoreAudioInterface::queryDeviceFormat() {
    if (deviceId_ == 0) {
        return false;
    }
    
    // Get sample rate
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyNominalSampleRate,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    
    Float64 sampleRate = 0.0;
    UInt32 propertySize = sizeof(Float64);
    
    OSStatus status = AudioObjectGetPropertyData(
        deviceId_,
        &propertyAddress,
        0,
        nullptr,
        &propertySize,
        &sampleRate
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to get sample rate, status " << status << std::endl;
        return false;
    }
    
    sampleRate_ = sampleRate;
    
    // Get buffer size
    propertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
    UInt32 bufferSize = 0;
    propertySize = sizeof(UInt32);
    
    status = AudioObjectGetPropertyData(
        deviceId_,
        &propertyAddress,
        0,
        nullptr,
        &propertySize,
        &bufferSize
    );
    
    if (status != noErr) {
        std::cerr << "CoreAudioInterface: Failed to get buffer size, status " << status << std::endl;
        // Use default buffer size
        bufferSize_ = 512;
    } else {
        bufferSize_ = bufferSize;
    }
    
    // Get stream configuration to determine channel count
    propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
    propertySize = 0;
    
    status = AudioObjectGetPropertyDataSize(
        deviceId_,
        &propertyAddress,
        0,
        nullptr,
        &propertySize
    );
    
    if (status == noErr && propertySize > 0) {
        AudioBufferList* bufferList = (AudioBufferList*)malloc(propertySize);
        
        status = AudioObjectGetPropertyData(
            deviceId_,
            &propertyAddress,
            0,
            nullptr,
            &propertySize,
            bufferList
        );
        
        if (status == noErr && bufferList->mNumberBuffers > 0) {
            numChannels_ = bufferList->mBuffers[0].mNumberChannels;
        }
        
        free(bufferList);
    }
    
    // Default to stereo if we couldn't determine channel count
    if (numChannels_ == 0) {
        numChannels_ = 2;
    }
    
    return true;
}

void CoreAudioInterface::cleanup() {
    if (audioUnit_ != nullptr) {
        AudioUnitUninitialize(audioUnit_);
        AudioComponentInstanceDispose(audioUnit_);
        audioUnit_ = nullptr;
    }
    
    initialized_ = false;
}

OSStatus CoreAudioInterface::renderCallback(
    void* inRefCon,
    AudioUnitRenderActionFlags* ioActionFlags,
    const AudioTimeStamp* inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList* ioData)
{
    // Cast the refcon to our instance
    CoreAudioInterface* self = static_cast<CoreAudioInterface*>(inRefCon);
    return self->render(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

OSStatus CoreAudioInterface::render(
    AudioUnitRenderActionFlags* ioActionFlags,
    const AudioTimeStamp* inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList* ioData)
{
    // Validate inputs
    if (ioData == nullptr || audioEngine_ == nullptr) {
        // Silence the output
        if (ioData != nullptr) {
            for (UInt32 i = 0; i < ioData->mNumberBuffers; ++i) {
                memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
            }
        }
        return noErr;
    }
    
    // CoreAudio uses non-interleaved format, but our AudioEngine expects interleaved
    // We need to handle the conversion
    
    UInt32 numChannels = ioData->mNumberBuffers;
    
    // Allocate temporary interleaved buffer
    std::vector<float> interleavedBuffer(inNumberFrames * numChannels);
    
    // Process audio through the engine (interleaved format)
    audioEngine_->processBlock(
        interleavedBuffer.data(),
        inNumberFrames,
        numChannels
    );
    
    // Convert from interleaved to non-interleaved
    for (UInt32 ch = 0; ch < numChannels; ++ch) {
        float* channelData = static_cast<float*>(ioData->mBuffers[ch].mData);
        
        for (UInt32 frame = 0; frame < inNumberFrames; ++frame) {
            channelData[frame] = interleavedBuffer[frame * numChannels + ch];
        }
    }
    
    return noErr;
}

} // namespace KickDrum

#endif // __APPLE__
