#pragma once

#include <string>
#include <map>

namespace KickDrum {

/**
 * @brief Represents a saved collection of parameter values
 * 
 * The Preset class encapsulates a named collection of synthesis parameters
 * that can be saved to and loaded from JSON files. Each preset includes:
 * - A user-friendly name
 * - A version string for compatibility tracking
 * - A map of parameter IDs to their values
 * 
 * Presets support JSON serialization for persistent storage and sharing.
 * The JSON format follows the structure defined in the design document.
 * 
 * Example JSON format:
 * {
 *   "name": "Deep Sub Kick",
 *   "version": "1.0.0",
 *   "parameters": {
 *     "basePitch": 50.0,
 *     "sineLevel": 0.8,
 *     ...
 *   }
 * }
 */
class Preset {
public:
    /**
     * @brief Construct a new Preset with default values
     */
    Preset();

    /**
     * @brief Construct a new Preset with specified name and version
     * 
     * @param name Preset name
     * @param version Version string (default: "1.0.0")
     */
    Preset(const std::string& name, const std::string& version = "1.0.0");

    /**
     * @brief Get the preset name
     * @return Preset name
     */
    const std::string& getName() const;

    /**
     * @brief Set the preset name
     * @param name New preset name
     */
    void setName(const std::string& name);

    /**
     * @brief Get the preset version
     * @return Version string
     */
    const std::string& getVersion() const;

    /**
     * @brief Set the preset version
     * @param version New version string
     */
    void setVersion(const std::string& version);

    /**
     * @brief Get all parameter values
     * @return Map of parameter ID to value
     */
    const std::map<std::string, float>& getParameters() const;

    /**
     * @brief Set all parameter values
     * @param parameters Map of parameter ID to value
     */
    void setParameters(const std::map<std::string, float>& parameters);

    /**
     * @brief Get a specific parameter value
     * 
     * @param id Parameter ID
     * @param defaultValue Value to return if parameter not found
     * @return Parameter value, or defaultValue if not found
     */
    float getParameter(const std::string& id, float defaultValue = 0.0f) const;

    /**
     * @brief Set a specific parameter value
     * 
     * @param id Parameter ID
     * @param value Parameter value
     */
    void setParameter(const std::string& id, float value);

    /**
     * @brief Check if a parameter exists in the preset
     * 
     * @param id Parameter ID
     * @return true if parameter exists, false otherwise
     */
    bool hasParameter(const std::string& id) const;

    /**
     * @brief Get the number of parameters in the preset
     * @return Number of parameters
     */
    size_t getParameterCount() const;

    /**
     * @brief Clear all parameters
     */
    void clearParameters();

    /**
     * @brief Serialize the preset to JSON string
     * 
     * Creates a JSON representation of the preset including name,
     * version, and all parameter values.
     * 
     * @return JSON string
     */
    std::string toJSON() const;

    /**
     * @brief Deserialize a preset from JSON string
     * 
     * Parses a JSON string and creates a Preset object. The JSON
     * must contain "name", "version", and "parameters" fields.
     * 
     * @param json JSON string
     * @return Preset object, or empty preset if parsing fails
     */
    static Preset fromJSON(const std::string& json);

    /**
     * @brief Load preset data from JSON string
     * 
     * Updates this preset's data from a JSON string. Returns true
     * if successful, false if parsing fails. On failure, the preset
     * remains unchanged.
     * 
     * @param json JSON string
     * @return true if successful, false otherwise
     */
    bool loadFromJSON(const std::string& json);

    /**
     * @brief Validate a JSON string for preset format
     * 
     * Checks if the JSON string has the correct structure for a preset
     * (contains name, version, and parameters fields).
     * 
     * @param json JSON string
     * @return true if valid, false otherwise
     */
    static bool validateJSON(const std::string& json);

    /**
     * @brief Check if the preset is empty (no parameters)
     * @return true if empty, false otherwise
     */
    bool isEmpty() const;

private:
    std::string name_;                          ///< Preset name
    std::string version_;                       ///< Version string
    std::map<std::string, float> parameters_;   ///< Parameter values

    // JSON parsing helper methods
    static std::string escapeJSONString(const std::string& str);
    static void skipWhitespace(const std::string& json, size_t& pos);
    static bool parseString(const std::string& json, size_t& pos, std::string& outValue);
    static bool parseNumber(const std::string& json, size_t& pos, float& outValue);
    static bool parseParametersObject(const std::string& json, size_t& pos, std::map<std::string, float>& outParameters);
    static bool skipValue(const std::string& json, size_t& pos);
};

} // namespace KickDrum
