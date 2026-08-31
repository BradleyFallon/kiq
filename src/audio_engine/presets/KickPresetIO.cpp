#include "KickPresetIO.h"

#include "Preset.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <vector>

namespace KickDrum {
namespace {

constexpr std::size_t kMaximumPresetBytes = 16 * 1024 * 1024;
constexpr const char* kSampleEncoding = "float32-le-base64";

struct JsonSpan {
    std::size_t begin = 0;
    std::size_t end = 0;
};

enum class FindFieldResult {
    Missing,
    Found,
    Invalid,
};

void setError(std::string* error, const std::string& message) {
    if (error) {
        *error = message;
    }
}

std::string escapeJSONString(const std::string& value) {
    std::ostringstream escaped;
    escaped << std::hex << std::setfill('0');
    for (const unsigned char character : value) {
        switch (character) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (character < 0x20) {
                    escaped << "\\u" << std::setw(4)
                            << static_cast<unsigned int>(character);
                } else {
                    escaped << static_cast<char>(character);
                }
                break;
        }
    }
    return escaped.str();
}

void skipWhitespace(const std::string& json, std::size_t& position,
                    std::size_t end) {
    while (position < end &&
           std::isspace(static_cast<unsigned char>(json[position]))) {
        ++position;
    }
}

int hexDigit(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

bool parseJSONString(const std::string& json, std::size_t& position,
                     std::size_t end, std::string& output) {
    skipWhitespace(json, position, end);
    if (position >= end || json[position++] != '"') {
        return false;
    }

    output.clear();
    while (position < end) {
        const unsigned char character =
            static_cast<unsigned char>(json[position++]);
        if (character == '"') {
            return true;
        }
        if (character < 0x20) {
            return false;
        }
        if (character != '\\') {
            output.push_back(static_cast<char>(character));
            continue;
        }
        if (position >= end) {
            return false;
        }
        const char escaped = json[position++];
        switch (escaped) {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case 'u': {
                if (position + 4 > end) {
                    return false;
                }
                unsigned int codePoint = 0;
                for (int digit = 0; digit < 4; ++digit) {
                    const int value = hexDigit(json[position++]);
                    if (value < 0) {
                        return false;
                    }
                    codePoint = (codePoint << 4u) |
                                static_cast<unsigned int>(value);
                }
                if (codePoint <= 0x7f) {
                    output.push_back(static_cast<char>(codePoint));
                } else if (codePoint <= 0x7ff) {
                    output.push_back(static_cast<char>(0xc0u | (codePoint >> 6u)));
                    output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
                } else {
                    output.push_back(static_cast<char>(0xe0u | (codePoint >> 12u)));
                    output.push_back(static_cast<char>(
                        0x80u | ((codePoint >> 6u) & 0x3fu)));
                    output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
                }
                break;
            }
            default: return false;
        }
    }
    return false;
}

bool skipJSONValue(const std::string& json, std::size_t& position,
                   std::size_t end) {
    skipWhitespace(json, position, end);
    if (position >= end) {
        return false;
    }

    if (json[position] == '"') {
        std::string ignored;
        return parseJSONString(json, position, end, ignored);
    }
    if (json[position] == '{') {
        ++position;
        skipWhitespace(json, position, end);
        if (position < end && json[position] == '}') {
            ++position;
            return true;
        }
        while (position < end) {
            std::string key;
            if (!parseJSONString(json, position, end, key)) {
                return false;
            }
            skipWhitespace(json, position, end);
            if (position >= end || json[position++] != ':') {
                return false;
            }
            if (!skipJSONValue(json, position, end)) {
                return false;
            }
            skipWhitespace(json, position, end);
            if (position < end && json[position] == ',') {
                ++position;
                continue;
            }
            if (position < end && json[position] == '}') {
                ++position;
                return true;
            }
            return false;
        }
        return false;
    }
    if (json[position] == '[') {
        ++position;
        skipWhitespace(json, position, end);
        if (position < end && json[position] == ']') {
            ++position;
            return true;
        }
        while (position < end) {
            if (!skipJSONValue(json, position, end)) {
                return false;
            }
            skipWhitespace(json, position, end);
            if (position < end && json[position] == ',') {
                ++position;
                continue;
            }
            if (position < end && json[position] == ']') {
                ++position;
                return true;
            }
            return false;
        }
        return false;
    }

    const auto consumeLiteral = [&](const char* literal) {
        const std::size_t length = std::strlen(literal);
        if (position + length > end ||
            json.compare(position, length, literal) != 0) {
            return false;
        }
        position += length;
        return true;
    };
    if (json[position] == 't') {
        return consumeLiteral("true");
    }
    if (json[position] == 'f') {
        return consumeLiteral("false");
    }
    if (json[position] == 'n') {
        return consumeLiteral("null");
    }

    // RFC 8259 number grammar. In particular, reject NaN/Infinity, leading
    // plus signs, incomplete exponents, and leading zeroes.
    std::size_t cursor = position;
    if (cursor < end && json[cursor] == '-') {
        ++cursor;
    }
    if (cursor >= end) {
        return false;
    }
    if (json[cursor] == '0') {
        ++cursor;
        if (cursor < end && std::isdigit(
                                static_cast<unsigned char>(json[cursor]))) {
            return false;
        }
    } else if (json[cursor] >= '1' && json[cursor] <= '9') {
        do {
            ++cursor;
        } while (cursor < end && std::isdigit(
                                      static_cast<unsigned char>(json[cursor])));
    } else {
        return false;
    }
    if (cursor < end && json[cursor] == '.') {
        ++cursor;
        const std::size_t fractionStart = cursor;
        while (cursor < end && std::isdigit(
                                   static_cast<unsigned char>(json[cursor]))) {
            ++cursor;
        }
        if (cursor == fractionStart) {
            return false;
        }
    }
    if (cursor < end && (json[cursor] == 'e' || json[cursor] == 'E')) {
        ++cursor;
        if (cursor < end && (json[cursor] == '+' || json[cursor] == '-')) {
            ++cursor;
        }
        const std::size_t exponentStart = cursor;
        while (cursor < end && std::isdigit(
                                   static_cast<unsigned char>(json[cursor]))) {
            ++cursor;
        }
        if (cursor == exponentStart) {
            return false;
        }
    }
    position = cursor;
    return true;
}

FindFieldResult findObjectField(const std::string& json, JsonSpan object,
                                const std::string& wantedKey,
                                JsonSpan& valueSpan) {
    std::size_t position = object.begin;
    skipWhitespace(json, position, object.end);
    if (position >= object.end || json[position++] != '{') {
        return FindFieldResult::Invalid;
    }

    while (position < object.end) {
        skipWhitespace(json, position, object.end);
        if (position < object.end && json[position] == '}') {
            return FindFieldResult::Missing;
        }
        std::string key;
        if (!parseJSONString(json, position, object.end, key)) {
            return FindFieldResult::Invalid;
        }
        skipWhitespace(json, position, object.end);
        if (position >= object.end || json[position++] != ':') {
            return FindFieldResult::Invalid;
        }
        skipWhitespace(json, position, object.end);
        const std::size_t valueBegin = position;
        if (!skipJSONValue(json, position, object.end)) {
            return FindFieldResult::Invalid;
        }
        if (key == wantedKey) {
            valueSpan = {valueBegin, position};
            return FindFieldResult::Found;
        }
        skipWhitespace(json, position, object.end);
        if (position < object.end && json[position] == ',') {
            ++position;
            continue;
        }
        if (position < object.end && json[position] == '}') {
            return FindFieldResult::Missing;
        }
        return FindFieldResult::Invalid;
    }
    return FindFieldResult::Invalid;
}

bool parseSpanString(const std::string& json, JsonSpan span,
                     std::string& output) {
    std::size_t position = span.begin;
    if (!parseJSONString(json, position, span.end, output)) {
        return false;
    }
    skipWhitespace(json, position, span.end);
    return position == span.end;
}

bool parseSpanUnsigned(const std::string& json, JsonSpan span,
                       std::uint64_t& output) {
    std::size_t position = span.begin;
    skipWhitespace(json, position, span.end);
    if (position >= span.end) {
        return false;
    }
    std::uint64_t value = 0;
    for (; position < span.end && std::isdigit(
             static_cast<unsigned char>(json[position])); ++position) {
        const unsigned int digit = static_cast<unsigned int>(json[position] - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10u) {
            return false;
        }
        value = value * 10u + digit;
    }
    skipWhitespace(json, position, span.end);
    if (position != span.end) {
        return false;
    }
    output = value;
    return true;
}

bool parseSpanFloat(const std::string& json, JsonSpan span, float& output) {
    std::size_t begin = span.begin;
    std::size_t end = span.end;
    skipWhitespace(json, begin, end);
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(json[end - 1]))) {
        --end;
    }
    if (begin == end) {
        return false;
    }
    try {
        std::size_t consumed = 0;
        const std::string token = json.substr(begin, end - begin);
        const float value = std::stof(token, &consumed);
        if (consumed != token.size() || !std::isfinite(value)) {
            return false;
        }
        output = value;
        return true;
    } catch (...) {
        return false;
    }
}

std::string base64Encode(const std::vector<unsigned char>& bytes) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve(((bytes.size() + 2) / 3) * 4);
    for (std::size_t index = 0; index < bytes.size(); index += 3) {
        const std::uint32_t first = bytes[index];
        const std::uint32_t second =
            index + 1 < bytes.size() ? bytes[index + 1] : 0u;
        const std::uint32_t third =
            index + 2 < bytes.size() ? bytes[index + 2] : 0u;
        const std::uint32_t word = (first << 16u) | (second << 8u) | third;
        encoded.push_back(alphabet[(word >> 18u) & 0x3fu]);
        encoded.push_back(alphabet[(word >> 12u) & 0x3fu]);
        encoded.push_back(index + 1 < bytes.size()
                              ? alphabet[(word >> 6u) & 0x3fu]
                              : '=');
        encoded.push_back(index + 2 < bytes.size() ? alphabet[word & 0x3fu] : '=');
    }
    return encoded;
}

int base64Value(char character) {
    if (character >= 'A' && character <= 'Z') return character - 'A';
    if (character >= 'a' && character <= 'z') return character - 'a' + 26;
    if (character >= '0' && character <= '9') return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

bool base64Decode(const std::string& encoded,
                  std::vector<unsigned char>& bytes) {
    bytes.clear();
    if (encoded.size() % 4 != 0) {
        return false;
    }
    bytes.reserve((encoded.size() / 4) * 3);
    for (std::size_t index = 0; index < encoded.size(); index += 4) {
        const bool finalBlock = index + 4 == encoded.size();
        const bool padThird = encoded[index + 2] == '=';
        const bool padFourth = encoded[index + 3] == '=';
        if ((!finalBlock && (padThird || padFourth)) ||
            (padThird && !padFourth)) {
            return false;
        }
        const int a = base64Value(encoded[index]);
        const int b = base64Value(encoded[index + 1]);
        const int c = padThird ? 0 : base64Value(encoded[index + 2]);
        const int d = padFourth ? 0 : base64Value(encoded[index + 3]);
        if (a < 0 || b < 0 || c < 0 || d < 0) {
            return false;
        }
        const std::uint32_t word =
            (static_cast<std::uint32_t>(a) << 18u) |
            (static_cast<std::uint32_t>(b) << 12u) |
            (static_cast<std::uint32_t>(c) << 6u) |
            static_cast<std::uint32_t>(d);
        bytes.push_back(static_cast<unsigned char>((word >> 16u) & 0xffu));
        if (!padThird) {
            bytes.push_back(static_cast<unsigned char>((word >> 8u) & 0xffu));
        }
        if (!padFourth) {
            bytes.push_back(static_cast<unsigned char>(word & 0xffu));
        }
    }
    return true;
}

std::string encodeMonoSamples(const std::vector<float>& samples) {
    static_assert(sizeof(float) == sizeof(std::uint32_t),
                  "Kiq preset audio requires 32-bit float");
    std::vector<unsigned char> bytes;
    bytes.reserve(samples.size() * 4);
    for (float sample : samples) {
        if (!std::isfinite(sample)) {
            sample = 0.0f;
        }
        std::uint32_t word = 0;
        std::memcpy(&word, &sample, sizeof(word));
        bytes.push_back(static_cast<unsigned char>(word & 0xffu));
        bytes.push_back(static_cast<unsigned char>((word >> 8u) & 0xffu));
        bytes.push_back(static_cast<unsigned char>((word >> 16u) & 0xffu));
        bytes.push_back(static_cast<unsigned char>((word >> 24u) & 0xffu));
    }
    return base64Encode(bytes);
}

bool requireObjectField(const std::string& json, JsonSpan object,
                        const char* key, JsonSpan& value, std::string* error) {
    const FindFieldResult result = findObjectField(json, object, key, value);
    if (result == FindFieldResult::Found) {
        return true;
    }
    setError(error, result == FindFieldResult::Missing
                        ? "Sample layer is missing field: " + std::string(key)
                        : "Sample layer metadata is invalid");
    return false;
}

bool decodeSampleLayer(const std::string& json, JsonSpan object,
                       KickSamplePayload& output, std::string* error) {
    JsonSpan sourceSpan;
    JsonSpan rateSpan;
    JsonSpan encodingSpan;
    JsonSpan framesSpan;
    JsonSpan dataSpan;
    if (!requireObjectField(json, object, "sourcePath", sourceSpan, error) ||
        !requireObjectField(json, object, "sampleRate", rateSpan, error) ||
        !requireObjectField(json, object, "encoding", encodingSpan, error) ||
        !requireObjectField(json, object, "frames", framesSpan, error) ||
        !requireObjectField(json, object, "data", dataSpan, error)) {
        return false;
    }

    KickSamplePayload decoded;
    std::string encoding;
    std::string encodedData;
    float sampleRate = 0.0f;
    std::uint64_t frames = 0;
    if (!parseSpanString(json, sourceSpan, decoded.sourcePath) ||
        !parseSpanFloat(json, rateSpan, sampleRate) ||
        !parseSpanString(json, encodingSpan, encoding) ||
        !parseSpanUnsigned(json, framesSpan, frames) ||
        !parseSpanString(json, dataSpan, encodedData)) {
        setError(error, "Sample layer metadata has an invalid value");
        return false;
    }
    if (encoding != kSampleEncoding) {
        setError(error, "Sample layer encoding is unsupported");
        return false;
    }
    if (sampleRate < 0.0f ||
        (frames > 0 && (sampleRate < 1000.0f || sampleRate > 768000.0f)) ||
        frames > std::numeric_limits<std::size_t>::max() / sizeof(float)) {
        setError(error, "Sample layer size or sample rate is invalid");
        return false;
    }

    std::vector<unsigned char> bytes;
    if (!base64Decode(encodedData, bytes) || bytes.size() != frames * 4u) {
        setError(error, "Sample layer audio payload is invalid");
        return false;
    }
    decoded.audio.sourceSampleRate = sampleRate;
    decoded.audio.samples.reserve(static_cast<std::size_t>(frames));
    for (std::size_t offset = 0; offset < bytes.size(); offset += 4) {
        const std::uint32_t word =
            static_cast<std::uint32_t>(bytes[offset]) |
            (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
        float sample = 0.0f;
        std::memcpy(&sample, &word, sizeof(sample));
        if (!std::isfinite(sample)) {
            setError(error, "Sample layer contains a non-finite sample");
            return false;
        }
        decoded.audio.samples.push_back(sample);
    }
    output = std::move(decoded);
    return true;
}

bool paramsAreUnchangedBySanitizing(const KickParams& params) {
    const KickParams sanitized = sanitizeKickParams(params);
    for (const auto& spec : kKickParameterSpecs) {
        if (getKickParameter(params, spec.id) !=
            getKickParameter(sanitized, spec.id)) {
            return false;
        }
    }
    return true;
}

} // namespace

std::string KickPresetIO::serialize(const KickPresetDocument& preset) {
    const KickParams params = sanitizeKickParams(preset.params);
    std::ostringstream json;
    json.imbue(std::locale::classic());
    json << std::setprecision(std::numeric_limits<float>::max_digits10);
    json << "{\n"
         << "  \"name\": \"" << escapeJSONString(preset.name) << "\",\n"
         << "  \"version\": \"" << kFormatVersion << "\",\n"
         << "  \"parameters\": {\n";

    for (std::size_t index = 0; index < kKickParameterSpecs.size(); ++index) {
        const auto& spec = kKickParameterSpecs[index];
        json << "    \"" << spec.key << "\": "
             << getKickParameter(params, spec.id);
        json << (index + 1 < kKickParameterSpecs.size() ? ",\n" : "\n");
    }
    json << "  }";
    if (preset.sampleLayer) {
        const auto& sample = *preset.sampleLayer;
        const float sourceSampleRate =
            std::isfinite(sample.audio.sourceSampleRate) &&
                    sample.audio.sourceSampleRate >= 0.0f
                ? sample.audio.sourceSampleRate
                : 0.0f;
        json << ",\n"
             << "  \"sampleLayer\": {\n"
             << "    \"sourcePath\": \""
             << escapeJSONString(sample.sourcePath) << "\",\n"
             << "    \"sampleRate\": " << sourceSampleRate << ",\n"
             << "    \"encoding\": \"" << kSampleEncoding << "\",\n"
             << "    \"frames\": " << sample.audio.samples.size() << ",\n"
             << "    \"data\": \"" << encodeMonoSamples(sample.audio.samples)
             << "\"\n"
             << "  }";
    }
    json << "\n}\n";
    return json.str();
}

bool KickPresetIO::deserialize(const std::string& json,
                               KickPresetDocument& output,
                               std::string* error) {
    if (error) {
        error->clear();
    }
    if (json.size() > kMaximumPresetBytes) {
        setError(error, "Preset file is too large");
        return false;
    }

    // Validate exactly one complete JSON value before using the legacy core
    // field decoder. This closes its permissive handling of trailing bytes,
    // malformed separators, and arbitrary unquoted primitive tokens.
    std::size_t documentEnd = 0;
    if (!skipJSONValue(json, documentEnd, json.size())) {
        setError(error, "Preset is not valid JSON");
        return false;
    }
    skipWhitespace(json, documentEnd, json.size());
    if (documentEnd != json.size()) {
        setError(error, "Preset is not valid JSON");
        return false;
    }

    JsonSpan sampleLayerSpan;
    const JsonSpan documentSpan {0, json.size()};
    const FindFieldResult sampleLayerResult =
        findObjectField(json, documentSpan, "sampleLayer", sampleLayerSpan);
    if (sampleLayerResult == FindFieldResult::Invalid) {
        setError(error, "Preset is not valid JSON");
        return false;
    }

    // The legacy generic Preset parser remains useful for its well-tested core
    // fields. Hide the optional structured payload from it and decode that
    // payload independently below.
    std::string coreJson = json;
    if (sampleLayerResult == FindFieldResult::Found) {
        coreJson.replace(sampleLayerSpan.begin,
                         sampleLayerSpan.end - sampleLayerSpan.begin, "null");
    }

    Preset parsed;
    if (!parsed.loadFromJSON(coreJson)) {
        setError(error, "Preset is not valid JSON");
        return false;
    }
    if (parsed.getVersion() != kFormatVersion) {
        setError(error, "Preset uses an unsupported format version");
        return false;
    }
    if (parsed.getParameterCount() != kKickParameterSpecs.size()) {
        setError(error, "Preset must contain the complete current parameter set");
        return false;
    }

    KickParams params = kDefaultKickParams;
    for (const auto& spec : kKickParameterSpecs) {
        const std::string key(spec.key);
        if (!parsed.hasParameter(key)) {
            setError(error, "Preset is missing parameter: " + key);
            return false;
        }

        const float value = parsed.getParameter(key);
        if (!std::isfinite(value) || value < spec.minimum ||
            value > spec.maximum) {
            setError(error, "Preset parameter is out of range: " + key);
            return false;
        }
        setKickParameter(params, spec.id, value);
    }

    if (!paramsAreUnchangedBySanitizing(params)) {
        setError(error, "Preset trajectory times must be strictly increasing");
        return false;
    }

    KickPresetDocument decoded;
    decoded.name = parsed.getName();
    decoded.params = params;
    if (sampleLayerResult == FindFieldResult::Found) {
        KickSamplePayload sample;
        if (!decodeSampleLayer(json, sampleLayerSpan, sample, error)) {
            return false;
        }
        decoded.sampleLayer = std::move(sample);
    }
    output = std::move(decoded);
    return true;
}

bool KickPresetIO::saveToFile(const std::string& path,
                              const KickPresetDocument& preset,
                              std::string* error) {
    if (error) {
        error->clear();
    }
    if (path.empty()) {
        setError(error, "Preset path is empty");
        return false;
    }

    if (preset.sampleLayer) {
        const float sourceSampleRate = preset.sampleLayer->audio.sourceSampleRate;
        if (!std::isfinite(sourceSampleRate) || sourceSampleRate < 0.0f ||
            (!preset.sampleLayer->audio.samples.empty() &&
             (sourceSampleRate < 1000.0f || sourceSampleRate > 768000.0f))) {
            setError(error, "Embedded sample audio requires a valid sample rate");
            return false;
        }
    }

    const std::string json = serialize(preset);
    if (json.size() > kMaximumPresetBytes) {
        setError(error, "Preset file is too large");
        return false;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(error, "Could not open preset for writing");
        return false;
    }
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    if (!file) {
        setError(error, "Could not write preset");
        return false;
    }
    return true;
}

bool KickPresetIO::loadFromFile(const std::string& path,
                                KickPresetDocument& output,
                                std::string* error) {
    if (error) {
        error->clear();
    }
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        setError(error, "Could not open preset");
        return false;
    }

    const std::streampos end = file.tellg();
    if (end < 0 || static_cast<std::uintmax_t>(end) > kMaximumPresetBytes) {
        setError(error, "Preset file is too large");
        return false;
    }
    std::string json(static_cast<std::size_t>(end), '\0');
    file.seekg(0, std::ios::beg);
    if (!json.empty()) {
        file.read(&json[0], static_cast<std::streamsize>(json.size()));
    }
    if (!file && !file.eof()) {
        setError(error, "Could not read preset");
        return false;
    }
    return deserialize(json, output, error);
}

} // namespace KickDrum
