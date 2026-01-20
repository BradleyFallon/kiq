#pragma once

#include <string>
#include <map>
#include <sstream>
#include <iomanip>

namespace KickDrum {

/**
 * @brief Simple JSON serializer/deserializer for parameter data
 * 
 * This class provides basic JSON serialization and deserialization
 * functionality for parameter values. It supports:
 * - String values
 * - Numeric values (float)
 * - Nested objects (one level deep)
 * 
 * The implementation is intentionally simple and focused on our
 * specific use case (parameter serialization) rather than being
 * a full-featured JSON library.
 */
class JSONSerializer {
public:
    /**
     * @brief Serialize a map of parameter values to JSON string
     * 
     * @param parameters Map of parameter ID to value
     * @param version Version string to include in JSON
     * @return JSON string
     */
    static std::string serializeParameters(
        const std::map<std::string, float>& parameters,
        const std::string& version = "1.0.0"
    );

    /**
     * @brief Deserialize JSON string to map of parameter values
     * 
     * @param json JSON string
     * @param outParameters Output map of parameter ID to value
     * @param outVersion Output version string (if present)
     * @return true if deserialization successful, false otherwise
     */
    static bool deserializeParameters(
        const std::string& json,
        std::map<std::string, float>& outParameters,
        std::string& outVersion
    );

    /**
     * @brief Validate JSON structure
     * 
     * Performs basic validation to ensure the JSON is well-formed
     * and contains the expected structure for parameter data.
     * 
     * @param json JSON string
     * @return true if valid, false otherwise
     */
    static bool validateJSON(const std::string& json);

private:
    /**
     * @brief Escape a string for JSON
     * @param str String to escape
     * @return Escaped string
     */
    static std::string escapeString(const std::string& str);

    /**
     * @brief Unescape a JSON string
     * @param str Escaped string
     * @return Unescaped string
     */
    static std::string unescapeString(const std::string& str);

    /**
     * @brief Skip whitespace in JSON string
     * @param json JSON string
     * @param pos Current position (will be updated)
     */
    static void skipWhitespace(const std::string& json, size_t& pos);

    /**
     * @brief Parse a JSON string value
     * @param json JSON string
     * @param pos Current position (will be updated)
     * @param outValue Output string value
     * @return true if successful, false otherwise
     */
    static bool parseString(const std::string& json, size_t& pos, std::string& outValue);

    /**
     * @brief Parse a JSON number value
     * @param json JSON string
     * @param pos Current position (will be updated)
     * @param outValue Output numeric value
     * @return true if successful, false otherwise
     */
    static bool parseNumber(const std::string& json, size_t& pos, float& outValue);

    /**
     * @brief Parse a JSON object
     * @param json JSON string
     * @param pos Current position (will be updated)
     * @param outParameters Output map of parameter values
     * @return true if successful, false otherwise
     */
    static bool parseObject(
        const std::string& json,
        size_t& pos,
        std::map<std::string, float>& outParameters
    );
};

} // namespace KickDrum
