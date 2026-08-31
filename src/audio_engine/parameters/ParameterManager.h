#pragma once

#include "Parameter.h"
#include <map>
#include <string>
#include <memory>
#include <vector>

namespace KickDrum {

/**
 * @brief Manages all synthesis parameters for the kick drum synthesizer
 * 
 * The ParameterManager maintains a registry of all synthesis parameters,
 * providing centralized access for getting and setting parameter values.
 * It supports parameter registration, value access, and serialization.
 * 
 * The registered synthesis parameters mirror the authoritative KickParams
 * trajectory model.
 */
class ParameterManager {
public:
    /**
     * @brief Construct a new Parameter Manager
     */
    ParameterManager();

    /**
     * @brief Register a parameter with the manager
     * 
     * @param parameter Parameter to register
     * @return true if registration successful, false if ID already exists
     */
    bool registerParameter(const Parameter& parameter);

    /**
     * @brief Get a parameter by ID
     * 
     * @param id Parameter ID
     * @return Pointer to parameter, or nullptr if not found
     */
    Parameter* getParameter(const std::string& id);

    /**
     * @brief Get a parameter by ID (const version)
     * 
     * @param id Parameter ID
     * @return Const pointer to parameter, or nullptr if not found
     */
    const Parameter* getParameter(const std::string& id) const;

    /**
     * @brief Set a parameter value by ID
     * 
     * @param id Parameter ID
     * @param value New value
     * @return true if parameter found and value set, false otherwise
     */
    bool setParameterValue(const std::string& id, float value);

    /**
     * @brief Get a parameter value by ID
     * 
     * @param id Parameter ID
     * @param defaultValue Value to return if parameter not found
     * @return Parameter value, or defaultValue if not found
     */
    float getParameterValue(const std::string& id, float defaultValue = 0.0f) const;

    /**
     * @brief Set a parameter value from normalized [0.0, 1.0] range
     * 
     * @param id Parameter ID
     * @param normalizedValue Normalized value in range [0.0, 1.0]
     * @return true if parameter found and value set, false otherwise
     */
    bool setParameterNormalized(const std::string& id, float normalizedValue);

    /**
     * @brief Get a parameter value in normalized [0.0, 1.0] range
     * 
     * @param id Parameter ID
     * @param defaultValue Value to return if parameter not found
     * @return Normalized parameter value, or defaultValue if not found
     */
    float getParameterNormalized(const std::string& id, float defaultValue = 0.0f) const;

    /**
     * @brief Check if a parameter exists
     * 
     * @param id Parameter ID
     * @return true if parameter exists, false otherwise
     */
    bool hasParameter(const std::string& id) const;

    /**
     * @brief Get the number of registered parameters
     * 
     * @return Number of parameters
     */
    size_t getParameterCount() const;

    /**
     * @brief Get all parameter IDs
     * 
     * @return Vector of parameter IDs
     */
    std::vector<std::string> getParameterIds() const;

    /**
     * @brief Reset all parameters to their default values
     */
    void resetAllParameters();

    /**
     * @brief Reset a specific parameter to its default value
     * 
     * @param id Parameter ID
     * @return true if parameter found and reset, false otherwise
     */
    bool resetParameter(const std::string& id);

    /**
     * @brief Register all synthesis parameters
     * 
     * This registers the fixed pitch/amplitude trajectory, membrane strike,
     * transient, and output parameters.
     */
    void registerAllSynthesisParameters();

    /**
     * @brief Serialize all parameters to JSON string
     * 
     * @param version Version string to include in JSON (default: "1.0.0")
     * @return JSON string containing all parameter values
     */
    std::string serializeToJSON(const std::string& version = "1.0.0") const;

    /**
     * @brief Deserialize parameters from JSON string
     * 
     * This will update parameter values from the JSON data.
     * Only parameters that exist in both the JSON and the manager
     * will be updated. Unknown parameters in the JSON are ignored.
     * 
     * @param json JSON string
     * @param outVersion Output version string from JSON
     * @return true if deserialization successful, false otherwise
     */
    bool deserializeFromJSON(const std::string& json, std::string& outVersion);

    /**
     * @brief Deserialize parameters from JSON string (simple version)
     * 
     * @param json JSON string
     * @return true if deserialization successful, false otherwise
     */
    bool deserializeFromJSON(const std::string& json);

private:
    std::map<std::string, Parameter> parameters_;  ///< Parameter registry
};

} // namespace KickDrum
