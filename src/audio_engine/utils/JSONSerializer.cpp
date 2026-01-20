#include "JSONSerializer.h"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cmath>

namespace KickDrum {

std::string JSONSerializer::serializeParameters(
    const std::map<std::string, float>& parameters,
    const std::string& version
) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    
    json << "{\n";
    json << "  \"version\": \"" << escapeString(version) << "\",\n";
    json << "  \"parameters\": {\n";
    
    bool first = true;
    for (const auto& pair : parameters) {
        if (!first) {
            json << ",\n";
        }
        first = false;
        
        json << "    \"" << escapeString(pair.first) << "\": " << pair.second;
    }
    
    json << "\n  }\n";
    json << "}";
    
    return json.str();
}

bool JSONSerializer::deserializeParameters(
    const std::string& json,
    std::map<std::string, float>& outParameters,
    std::string& outVersion
) {
    outParameters.clear();
    outVersion = "";
    
    size_t pos = 0;
    skipWhitespace(json, pos);
    
    // Expect opening brace
    if (pos >= json.length() || json[pos] != '{') {
        return false;
    }
    pos++;
    
    // Parse top-level object
    bool foundVersion = false;
    bool foundParameters = false;
    
    while (pos < json.length()) {
        skipWhitespace(json, pos);
        
        // Check for closing brace
        if (pos < json.length() && json[pos] == '}') {
            pos++;
            break;
        }
        
        // Parse key
        std::string key;
        if (!parseString(json, pos, key)) {
            return false;
        }
        
        skipWhitespace(json, pos);
        
        // Expect colon
        if (pos >= json.length() || json[pos] != ':') {
            return false;
        }
        pos++;
        
        skipWhitespace(json, pos);
        
        // Parse value based on key
        if (key == "version") {
            if (!parseString(json, pos, outVersion)) {
                return false;
            }
            foundVersion = true;
        } else if (key == "parameters") {
            if (!parseObject(json, pos, outParameters)) {
                return false;
            }
            foundParameters = true;
        } else {
            // Skip unknown key
            // For simplicity, we'll just fail on unknown keys
            return false;
        }
        
        skipWhitespace(json, pos);
        
        // Check for comma
        if (pos < json.length() && json[pos] == ',') {
            pos++;
        }
    }
    
    return foundVersion && foundParameters;
}

bool JSONSerializer::validateJSON(const std::string& json) {
    std::map<std::string, float> parameters;
    std::string version;
    return deserializeParameters(json, parameters, version);
}

std::string JSONSerializer::escapeString(const std::string& str) {
    std::ostringstream escaped;
    
    for (char c : str) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b";  break;
            case '\f': escaped << "\\f";  break;
            case '\n': escaped << "\\n";  break;
            case '\r': escaped << "\\r";  break;
            case '\t': escaped << "\\t";  break;
            default:
                if (c < 32) {
                    // Control character - use unicode escape
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    escaped << c;
                }
                break;
        }
    }
    
    return escaped.str();
}

std::string JSONSerializer::unescapeString(const std::string& str) {
    std::ostringstream unescaped;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            char next = str[i + 1];
            switch (next) {
                case '"':  unescaped << '"';  i++; break;
                case '\\': unescaped << '\\'; i++; break;
                case 'b':  unescaped << '\b'; i++; break;
                case 'f':  unescaped << '\f'; i++; break;
                case 'n':  unescaped << '\n'; i++; break;
                case 'r':  unescaped << '\r'; i++; break;
                case 't':  unescaped << '\t'; i++; break;
                case 'u':
                    // Unicode escape - simplified handling
                    // For now, just skip it
                    i += 5;
                    break;
                default:
                    unescaped << str[i];
                    break;
            }
        } else {
            unescaped << str[i];
        }
    }
    
    return unescaped.str();
}

void JSONSerializer::skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.length() && std::isspace(json[pos])) {
        pos++;
    }
}

bool JSONSerializer::parseString(const std::string& json, size_t& pos, std::string& outValue) {
    skipWhitespace(json, pos);
    
    // Expect opening quote
    if (pos >= json.length() || json[pos] != '"') {
        return false;
    }
    pos++;
    
    std::ostringstream value;
    bool escaped = false;
    
    while (pos < json.length()) {
        char c = json[pos];
        
        if (escaped) {
            value << c;
            escaped = false;
        } else if (c == '\\') {
            value << c;
            escaped = true;
        } else if (c == '"') {
            // End of string
            pos++;
            outValue = unescapeString(value.str());
            return true;
        } else {
            value << c;
        }
        
        pos++;
    }
    
    return false; // Unterminated string
}

bool JSONSerializer::parseNumber(const std::string& json, size_t& pos, float& outValue) {
    skipWhitespace(json, pos);
    
    size_t start = pos;
    
    // Parse optional minus sign
    if (pos < json.length() && json[pos] == '-') {
        pos++;
    }
    
    // Parse digits before decimal point
    if (pos >= json.length() || !std::isdigit(json[pos])) {
        return false;
    }
    
    while (pos < json.length() && std::isdigit(json[pos])) {
        pos++;
    }
    
    // Parse optional decimal point and fractional part
    if (pos < json.length() && json[pos] == '.') {
        pos++;
        
        if (pos >= json.length() || !std::isdigit(json[pos])) {
            return false;
        }
        
        while (pos < json.length() && std::isdigit(json[pos])) {
            pos++;
        }
    }
    
    // Parse optional exponent
    if (pos < json.length() && (json[pos] == 'e' || json[pos] == 'E')) {
        pos++;
        
        if (pos < json.length() && (json[pos] == '+' || json[pos] == '-')) {
            pos++;
        }
        
        if (pos >= json.length() || !std::isdigit(json[pos])) {
            return false;
        }
        
        while (pos < json.length() && std::isdigit(json[pos])) {
            pos++;
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

bool JSONSerializer::parseObject(
    const std::string& json,
    size_t& pos,
    std::map<std::string, float>& outParameters
) {
    skipWhitespace(json, pos);
    
    // Expect opening brace
    if (pos >= json.length() || json[pos] != '{') {
        return false;
    }
    pos++;
    
    while (pos < json.length()) {
        skipWhitespace(json, pos);
        
        // Check for closing brace
        if (pos < json.length() && json[pos] == '}') {
            pos++;
            return true;
        }
        
        // Parse key
        std::string key;
        if (!parseString(json, pos, key)) {
            return false;
        }
        
        skipWhitespace(json, pos);
        
        // Expect colon
        if (pos >= json.length() || json[pos] != ':') {
            return false;
        }
        pos++;
        
        skipWhitespace(json, pos);
        
        // Parse value (number)
        float value;
        if (!parseNumber(json, pos, value)) {
            return false;
        }
        
        outParameters[key] = value;
        
        skipWhitespace(json, pos);
        
        // Check for comma
        if (pos < json.length() && json[pos] == ',') {
            pos++;
        }
    }
    
    return false; // Unterminated object
}

} // namespace KickDrum
