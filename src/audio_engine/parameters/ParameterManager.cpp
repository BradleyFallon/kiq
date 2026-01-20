#include "ParameterManager.h"
#ifdef TEST_BUILD
#include "JSONSerializer.h"
#else
#include "../utils/JSONSerializer.h"
#endif
#include <algorithm>

namespace KickDrum {

ParameterManager::ParameterManager() {
    // Constructor - parameters will be registered via registerAllSynthesisParameters()
}

bool ParameterManager::registerParameter(const Parameter& parameter) {
    const std::string& id = parameter.getId();
    
    // Check if parameter already exists
    if (parameters_.find(id) != parameters_.end()) {
        return false; // Parameter ID already exists
    }
    
    // Register the parameter
    parameters_[id] = parameter;
    return true;
}

Parameter* ParameterManager::getParameter(const std::string& id) {
    auto it = parameters_.find(id);
    if (it != parameters_.end()) {
        return &(it->second);
    }
    return nullptr;
}

const Parameter* ParameterManager::getParameter(const std::string& id) const {
    auto it = parameters_.find(id);
    if (it != parameters_.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool ParameterManager::setParameterValue(const std::string& id, float value) {
    Parameter* param = getParameter(id);
    if (param) {
        param->setValue(value);
        return true;
    }
    return false;
}

float ParameterManager::getParameterValue(const std::string& id, float defaultValue) const {
    const Parameter* param = getParameter(id);
    if (param) {
        return param->getValue();
    }
    return defaultValue;
}

bool ParameterManager::setParameterNormalized(const std::string& id, float normalizedValue) {
    Parameter* param = getParameter(id);
    if (param) {
        param->denormalize(normalizedValue);
        return true;
    }
    return false;
}

float ParameterManager::getParameterNormalized(const std::string& id, float defaultValue) const {
    const Parameter* param = getParameter(id);
    if (param) {
        return param->normalize();
    }
    return defaultValue;
}

bool ParameterManager::hasParameter(const std::string& id) const {
    return parameters_.find(id) != parameters_.end();
}

size_t ParameterManager::getParameterCount() const {
    return parameters_.size();
}

std::vector<std::string> ParameterManager::getParameterIds() const {
    std::vector<std::string> ids;
    ids.reserve(parameters_.size());
    
    for (const auto& pair : parameters_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

void ParameterManager::resetAllParameters() {
    for (auto& pair : parameters_) {
        pair.second.reset();
    }
}

bool ParameterManager::resetParameter(const std::string& id) {
    Parameter* param = getParameter(id);
    if (param) {
        param->reset();
        return true;
    }
    return false;
}

void ParameterManager::registerAllSynthesisParameters() {
    // Generator Parameters
    registerParameter(Parameter("basePitch", "Base Pitch", 50.0f, 20.0f, 200.0f, "Hz"));
    registerParameter(Parameter("sineLevel", "Sine Driver Level", 80.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("harmonicRatio", "Harmonic Ratio", 2.0f, 0.5f, 8.0f, "x"));
    registerParameter(Parameter("harmonicLevel", "Harmonic Level", 30.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("harmonicModDepth", "Harmonic Mod Depth", 50.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("noiseLevel", "Noise Level", 20.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("noiseModDepth", "Noise Mod Depth", 70.0f, 0.0f, 100.0f, "%"));
    
    // Warm-Up Phase Parameters
    registerParameter(Parameter("warmUpDuration", "Warm-Up Duration", 20.0f, 0.0f, 100.0f, "ms"));
    registerParameter(Parameter("warmUpStartFreq", "Warm-Up Start Freq", 10.0f, 5.0f, 50.0f, "Hz"));
    registerParameter(Parameter("warmUpAmplitude", "Warm-Up Amplitude", 50.0f, 0.0f, 100.0f, "%"));
    
    // ADSR Envelope Parameters
    registerParameter(Parameter("attack", "Attack Time", 1.0f, 0.0f, 1000.0f, "ms"));
    registerParameter(Parameter("decay", "Decay Time", 500.0f, 0.0f, 5000.0f, "ms"));
    registerParameter(Parameter("sustain", "Sustain Level", 0.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("release", "Release Time", 100.0f, 0.0f, 5000.0f, "ms"));
    
    // Pitch Envelope Parameters
    registerParameter(Parameter("pitchEnvelopeDepth", "Pitch Envelope Depth", 500.0f, 0.0f, 2000.0f, "Hz"));
    
    // Envelope Curve Parameters (using integer values for enum-like behavior)
    // 0 = LINEAR, 1 = EXPONENTIAL, 2 = LOGARITHMIC, 3 = CUSTOM
    registerParameter(Parameter("attackCurve", "Attack Curve", 0.0f, 0.0f, 3.0f, ""));
    registerParameter(Parameter("decayCurve", "Decay Curve", 1.0f, 0.0f, 3.0f, ""));
    registerParameter(Parameter("releaseCurve", "Release Curve", 1.0f, 0.0f, 3.0f, ""));
    
    // Compressor Parameters
    registerParameter(Parameter("compressorThreshold", "Compressor Threshold", -12.0f, -60.0f, 0.0f, "dB"));
    registerParameter(Parameter("compressorRatio", "Compressor Ratio", 4.0f, 1.0f, 20.0f, ":1"));
    registerParameter(Parameter("compressorAttack", "Compressor Attack", 1.0f, 0.1f, 100.0f, "ms"));
    registerParameter(Parameter("compressorRelease", "Compressor Release", 100.0f, 10.0f, 1000.0f, "ms"));
    registerParameter(Parameter("compressorMix", "Compressor Mix", 50.0f, 0.0f, 100.0f, "%"));
    
    // Reverb Parameters
    registerParameter(Parameter("reverbRoomSize", "Reverb Room Size", 30.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("reverbDecayTime", "Reverb Decay Time", 1.0f, 0.1f, 10.0f, "s"));
    registerParameter(Parameter("reverbDamping", "Reverb Damping", 50.0f, 0.0f, 100.0f, "%"));
    registerParameter(Parameter("reverbMix", "Reverb Mix", 10.0f, 0.0f, 100.0f, "%"));
    
    // Master Output Parameters
    registerParameter(Parameter("masterLevel", "Master Output Level", 80.0f, 0.0f, 100.0f, "%"));
    
    // Pitch Tracking (0 = OFF, 1 = ON)
    registerParameter(Parameter("pitchTracking", "Pitch Tracking", 1.0f, 0.0f, 1.0f, ""));
}

std::string ParameterManager::serializeToJSON(const std::string& version) const {
    // Build map of parameter values
    std::map<std::string, float> parameterValues;
    
    for (const auto& pair : parameters_) {
        parameterValues[pair.first] = pair.second.getValue();
    }
    
    return JSONSerializer::serializeParameters(parameterValues, version);
}

bool ParameterManager::deserializeFromJSON(const std::string& json, std::string& outVersion) {
    std::map<std::string, float> parameterValues;
    
    if (!JSONSerializer::deserializeParameters(json, parameterValues, outVersion)) {
        return false;
    }
    
    // Update parameter values
    for (const auto& pair : parameterValues) {
        // Only update if parameter exists
        if (hasParameter(pair.first)) {
            setParameterValue(pair.first, pair.second);
        }
        // Silently ignore unknown parameters for forward compatibility
    }
    
    return true;
}

bool ParameterManager::deserializeFromJSON(const std::string& json) {
    std::string version;
    return deserializeFromJSON(json, version);
}

} // namespace KickDrum
