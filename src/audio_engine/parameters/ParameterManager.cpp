#include "ParameterManager.h"
#include "KickParams.h"
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
    for (const auto& spec : kKickParameterSpecs) {
        registerParameter(Parameter(std::string(spec.key), std::string(spec.name),
                                    getDefaultKickParameter(spec.id), spec.minimum,
                                    spec.maximum, std::string(spec.unit)));
    }
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
