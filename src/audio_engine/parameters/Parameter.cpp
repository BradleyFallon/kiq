#include "Parameter.h"
#include <algorithm>
#include <cmath>

namespace KickDrum {

Parameter::Parameter(
    const std::string& id,
    const std::string& name,
    float defaultValue,
    float minValue,
    float maxValue,
    const std::string& unit
)
    : id_(id)
    , name_(name)
    , value_(defaultValue)
    , defaultValue_(defaultValue)
    , minValue_(minValue)
    , maxValue_(maxValue)
    , unit_(unit)
{
    // Ensure min <= max
    if (minValue_ > maxValue_) {
        std::swap(minValue_, maxValue_);
    }
    
    // Clamp default value to valid range
    defaultValue_ = clamp(defaultValue_);
    value_ = defaultValue_;
}

Parameter::Parameter()
    : id_("")
    , name_("")
    , value_(0.0f)
    , defaultValue_(0.0f)
    , minValue_(0.0f)
    , maxValue_(1.0f)
    , unit_("")
{
}

void Parameter::setValue(float value) {
    value_ = clamp(value);
}

float Parameter::getValue() const {
    return value_;
}

float Parameter::getDefaultValue() const {
    return defaultValue_;
}

float Parameter::getMinValue() const {
    return minValue_;
}

float Parameter::getMaxValue() const {
    return maxValue_;
}

const std::string& Parameter::getId() const {
    return id_;
}

const std::string& Parameter::getName() const {
    return name_;
}

const std::string& Parameter::getUnit() const {
    return unit_;
}

float Parameter::normalize() const {
    // Handle case where min == max (avoid division by zero)
    if (std::abs(maxValue_ - minValue_) < 1e-6f) {
        return 0.0f;
    }
    
    // Map [minValue, maxValue] to [0.0, 1.0]
    return (value_ - minValue_) / (maxValue_ - minValue_);
}

void Parameter::denormalize(float normalizedValue) {
    // Clamp normalized value to [0.0, 1.0]
    normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
    
    // Map [0.0, 1.0] to [minValue, maxValue]
    float denormalizedValue = minValue_ + normalizedValue * (maxValue_ - minValue_);
    
    setValue(denormalizedValue);
}

void Parameter::reset() {
    value_ = defaultValue_;
}

bool Parameter::isDefault() const {
    // Use small epsilon for floating point comparison
    return std::abs(value_ - defaultValue_) < 1e-6f;
}

float Parameter::clamp(float value) const {
    return std::max(minValue_, std::min(maxValue_, value));
}

} // namespace KickDrum
