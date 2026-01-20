#pragma once

#include <string>
#include <algorithm>

namespace KickDrum {

/**
 * @brief Represents a single synthesis parameter with value, range, and unit
 * 
 * The Parameter class encapsulates a controllable synthesis parameter with:
 * - Current value within a defined range [minValue, maxValue]
 * - Default value for reset functionality
 * - Unit string for display (e.g., "Hz", "dB", "%", "ms")
 * - Normalization/denormalization for host automation (0.0 to 1.0 range)
 * 
 * Parameters support both direct value access and normalized access for
 * integration with plugin hosts and automation systems.
 */
class Parameter {
public:
    /**
     * @brief Construct a new Parameter
     * 
     * @param id Unique identifier for the parameter
     * @param name Display name for the parameter
     * @param defaultValue Default value
     * @param minValue Minimum allowed value
     * @param maxValue Maximum allowed value
     * @param unit Unit string (e.g., "Hz", "dB", "%", "ms")
     */
    Parameter(
        const std::string& id,
        const std::string& name,
        float defaultValue,
        float minValue,
        float maxValue,
        const std::string& unit = ""
    );

    /**
     * @brief Default constructor
     */
    Parameter();

    /**
     * @brief Set the parameter value
     * 
     * The value will be clamped to the valid range [minValue, maxValue].
     * 
     * @param value New value
     */
    void setValue(float value);

    /**
     * @brief Get the current parameter value
     * @return Current value
     */
    float getValue() const;

    /**
     * @brief Get the default value
     * @return Default value
     */
    float getDefaultValue() const;

    /**
     * @brief Get the minimum value
     * @return Minimum value
     */
    float getMinValue() const;

    /**
     * @brief Get the maximum value
     * @return Maximum value
     */
    float getMaxValue() const;

    /**
     * @brief Get the parameter ID
     * @return Parameter ID string
     */
    const std::string& getId() const;

    /**
     * @brief Get the parameter name
     * @return Parameter name string
     */
    const std::string& getName() const;

    /**
     * @brief Get the parameter unit
     * @return Unit string
     */
    const std::string& getUnit() const;

    /**
     * @brief Normalize the current value to [0.0, 1.0] range
     * 
     * This is useful for plugin host automation where parameters are
     * typically represented in normalized form.
     * 
     * @return Normalized value in range [0.0, 1.0]
     */
    float normalize() const;

    /**
     * @brief Set the value from a normalized [0.0, 1.0] range
     * 
     * This converts a normalized value (0.0 to 1.0) to the actual
     * parameter range and sets the value.
     * 
     * @param normalizedValue Normalized value in range [0.0, 1.0]
     */
    void denormalize(float normalizedValue);

    /**
     * @brief Reset the parameter to its default value
     */
    void reset();

    /**
     * @brief Check if the parameter is at its default value
     * @return true if at default value, false otherwise
     */
    bool isDefault() const;

private:
    std::string id_;           ///< Unique identifier
    std::string name_;         ///< Display name
    float value_;              ///< Current value
    float defaultValue_;       ///< Default value
    float minValue_;           ///< Minimum value
    float maxValue_;           ///< Maximum value
    std::string unit_;         ///< Unit string

    /**
     * @brief Clamp a value to the valid range
     * @param value Value to clamp
     * @return Clamped value
     */
    float clamp(float value) const;
};

} // namespace KickDrum
