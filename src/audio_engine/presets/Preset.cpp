#include "Preset.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cctype>

namespace KickDrum {

Preset::Preset()
    : name_("Untitled")
    , version_("1.0.0")
    , parameters_()
{
}

Preset::Preset(const std::string& name, const std::string& version)
    : name_(name)
    , version_(version)
    , parameters_()
{
}

const std::string& Preset::getName() const {
    return name_;
}

void Preset::setName(const std::string& name) {
    name_ = name;
}

const std::string& Preset::getVersion() const {
    return version_;
}

void Preset::setVersion(const std::string& version) {
    version_ = version;
}

const std::map<std::string, float>& Preset::getParameters() const {
    return parameters_;
}

void Preset::setParameters(const std::map<std::string, float>& parameters) {
    parameters_ = parameters;
}

float Preset::getParameter(const std::string& id, float defaultValue) const {
    auto it = parameters_.find(id);
    if (it != parameters_.end()) {
        return it->second;
    }
    return defaultValue;
}

void Preset::setParameter(const std::string& id, float value) {
    parameters_[id] = value;
}

bool Preset::hasParameter(const std::string& id) const {
    return parameters_.find(id) != parameters_.end();
}

size_t Preset::getParameterCount() const {
    return parameters_.size();
}

void Preset::clearParameters() {
    parameters_.clear();
}

std::string Preset::toJSON() const {
    // Build JSON manually to include name and version
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"name\": \"" << escapeJSONString(name_) << "\",\n";
    oss << "  \"version\": \"" << escapeJSONString(version_) << "\",\n";
    oss << "  \"parameters\": {\n";
    
    // Add parameters
    bool first = true;
    for (const auto& param : parameters_) {
        if (!first) {
            oss << ",\n";
        }
        first = false;
        oss << "    \"" << escapeJSONString(param.first) << "\": " << param.second;
    }
    
    oss << "\n  }\n";
    oss << "}";
    
    return oss.str();
}

Preset Preset::fromJSON(const std::string& json) {
    Preset preset;
    preset.loadFromJSON(json);
    return preset;
}

bool Preset::loadFromJSON(const std::string& json) {
    // Parse JSON manually
    size_t pos = 0;
    
    // Skip whitespace and find opening brace
    skipWhitespace(json, pos);
    if (pos >= json.length() || json[pos] != '{') {
        return false;
    }
    pos++; // Skip '{'
    
    std::string tempName;
    std::string tempVersion;
    std::map<std::string, float> tempParameters;
    
    bool foundName = false;
    bool foundVersion = false;
    bool foundParameters = false;
    
    // Parse object fields
    while (pos < json.length()) {
        skipWhitespace(json, pos);
        
        // Check for closing brace
        if (json[pos] == '}') {
            break;
        }
        
        // Parse field name
        std::string fieldName;
        if (!parseString(json, pos, fieldName)) {
            return false;
        }
        
        // Skip colon
        skipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != ':') {
            return false;
        }
        pos++; // Skip ':'
        skipWhitespace(json, pos);
        
        // Parse field value based on field name
        if (fieldName == "name") {
            if (!parseString(json, pos, tempName)) {
                return false;
            }
            foundName = true;
        } else if (fieldName == "version") {
            if (!parseString(json, pos, tempVersion)) {
                return false;
            }
            foundVersion = true;
        } else if (fieldName == "parameters") {
            if (!parseParametersObject(json, pos, tempParameters)) {
                return false;
            }
            foundParameters = true;
        } else {
            // Unknown field, skip it
            if (!skipValue(json, pos)) {
                return false;
            }
        }
        
        // Skip comma if present
        skipWhitespace(json, pos);
        if (pos < json.length() && json[pos] == ',') {
            pos++;
        }
    }
    
    // Verify all required fields were found
    if (!foundName || !foundVersion || !foundParameters) {
        return false;
    }
    
    // Update this preset's data
    name_ = tempName;
    version_ = tempVersion;
    parameters_ = tempParameters;
    
    return true;
}

bool Preset::validateJSON(const std::string& json) {
    Preset temp;
    return temp.loadFromJSON(json);
}

bool Preset::isEmpty() const {
    return parameters_.empty();
}

// Private helper methods

std::string Preset::escapeJSONString(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 0x20) {
                    // Control character, use unicode escape
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

void Preset::skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.length() && std::isspace(json[pos])) {
        pos++;
    }
}

bool Preset::parseString(const std::string& json, size_t& pos, std::string& outValue) {
    skipWhitespace(json, pos);
    
    if (pos >= json.length() || json[pos] != '"') {
        return false;
    }
    pos++; // Skip opening quote
    
    std::ostringstream oss;
    while (pos < json.length()) {
        char c = json[pos];
        
        if (c == '"') {
            // End of string
            pos++;
            outValue = oss.str();
            return true;
        } else if (c == '\\') {
            // Escape sequence
            pos++;
            if (pos >= json.length()) {
                return false;
            }
            
            char escaped = json[pos];
            switch (escaped) {
                case '"':  oss << '"'; break;
                case '\\': oss << '\\'; break;
                case '/':  oss << '/'; break;
                case 'b':  oss << '\b'; break;
                case 'f':  oss << '\f'; break;
                case 'n':  oss << '\n'; break;
                case 'r':  oss << '\r'; break;
                case 't':  oss << '\t'; break;
                case 'u':
                    // Unicode escape (simplified - just skip for now)
                    pos += 4;
                    if (pos >= json.length()) {
                        return false;
                    }
                    break;
                default:
                    return false;
            }
            pos++;
        } else {
            oss << c;
            pos++;
        }
    }
    
    return false; // Unterminated string
}

bool Preset::parseNumber(const std::string& json, size_t& pos, float& outValue) {
    skipWhitespace(json, pos);
    
    size_t start = pos;
    
    // Parse optional minus sign
    if (pos < json.length() && json[pos] == '-') {
        pos++;
    }
    
    // Parse digits
    bool hasDigits = false;
    while (pos < json.length() && std::isdigit(json[pos])) {
        hasDigits = true;
        pos++;
    }
    
    if (!hasDigits) {
        return false;
    }
    
    // Parse optional decimal part
    if (pos < json.length() && json[pos] == '.') {
        pos++;
        hasDigits = false;
        while (pos < json.length() && std::isdigit(json[pos])) {
            hasDigits = true;
            pos++;
        }
        if (!hasDigits) {
            return false;
        }
    }
    
    // Parse optional exponent
    if (pos < json.length() && (json[pos] == 'e' || json[pos] == 'E')) {
        pos++;
        if (pos < json.length() && (json[pos] == '+' || json[pos] == '-')) {
            pos++;
        }
        hasDigits = false;
        while (pos < json.length() && std::isdigit(json[pos])) {
            hasDigits = true;
            pos++;
        }
        if (!hasDigits) {
            return false;
        }
    }
    
    // Convert to float
    std::string numStr = json.substr(start, pos - start);
    try {
        outValue = std::stof(numStr);
        return true;
    } catch (...) {
        return false;
    }
}

bool Preset::parseParametersObject(const std::string& json, size_t& pos, std::map<std::string, float>& outParameters) {
    skipWhitespace(json, pos);
    
    if (pos >= json.length() || json[pos] != '{') {
        return false;
    }
    pos++; // Skip '{'
    
    outParameters.clear();
    
    while (pos < json.length()) {
        skipWhitespace(json, pos);
        
        // Check for closing brace
        if (json[pos] == '}') {
            pos++;
            return true;
        }
        
        // Parse parameter name
        std::string paramName;
        if (!parseString(json, pos, paramName)) {
            return false;
        }
        
        // Skip colon
        skipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != ':') {
            return false;
        }
        pos++; // Skip ':'
        
        // Parse parameter value
        float paramValue;
        if (!parseNumber(json, pos, paramValue)) {
            return false;
        }
        
        outParameters[paramName] = paramValue;
        
        // Skip comma if present
        skipWhitespace(json, pos);
        if (pos < json.length() && json[pos] == ',') {
            pos++;
        }
    }
    
    return false; // Unterminated object
}

bool Preset::skipValue(const std::string& json, size_t& pos) {
    skipWhitespace(json, pos);
    
    if (pos >= json.length()) {
        return false;
    }
    
    char c = json[pos];
    
    if (c == '"') {
        // String value
        std::string dummy;
        return parseString(json, pos, dummy);
    } else if (c == '{') {
        // Object value
        pos++;
        int depth = 1;
        while (pos < json.length() && depth > 0) {
            if (json[pos] == '{') depth++;
            else if (json[pos] == '}') depth--;
            pos++;
        }
        return depth == 0;
    } else if (c == '[') {
        // Array value
        pos++;
        int depth = 1;
        while (pos < json.length() && depth > 0) {
            if (json[pos] == '[') depth++;
            else if (json[pos] == ']') depth--;
            pos++;
        }
        return depth == 0;
    } else if (std::isdigit(c) || c == '-') {
        // Number value
        float dummy;
        return parseNumber(json, pos, dummy);
    } else if (json.substr(pos, 4) == "true") {
        pos += 4;
        return true;
    } else if (json.substr(pos, 5) == "false") {
        pos += 5;
        return true;
    } else if (json.substr(pos, 4) == "null") {
        pos += 4;
        return true;
    }
    
    return false;
}

} // namespace KickDrum
