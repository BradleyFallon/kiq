#include "PresetManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

namespace KickDrum {

PresetManager::PresetManager(const std::string& factoryPresetsPath, 
                             const std::string& userPresetsPath)
    : presets_()
    , currentPresetIndex_(-1)
    , factoryPresetsPath_(factoryPresetsPath)
    , userPresetsPath_(userPresetsPath)
    , presetLoadedCallback_(nullptr)
    , lastError_()
{
}

PresetManager::~PresetManager() {
}

bool PresetManager::initialize() {
    clearError();
    presets_.clear();
    currentPresetIndex_ = -1;

    // Load factory presets if path is set
    if (!factoryPresetsPath_.empty()) {
        if (!loadPresetsFromDirectory(factoryPresetsPath_, true)) {
            // Non-fatal error - continue with user presets
            // Error message already set by loadPresetsFromDirectory
        }
    }

    // Load user presets if path is set
    if (!userPresetsPath_.empty()) {
        // Ensure user presets directory exists
        if (!ensureUserPresetsDirectoryExists()) {
            setError("Failed to create user presets directory");
            return false;
        }

        if (!loadPresetsFromDirectory(userPresetsPath_, false)) {
            // Non-fatal error - we can still use factory presets
            // Error message already set by loadPresetsFromDirectory
        }
    }

    // Select first preset if available
    if (!presets_.empty()) {
        currentPresetIndex_ = 0;
    }

    return true;
}

size_t PresetManager::getPresetCount() const {
    return presets_.size();
}

size_t PresetManager::getFactoryPresetCount() const {
    size_t count = 0;
    for (const auto& entry : presets_) {
        if (entry.isFactory) {
            count++;
        }
    }
    return count;
}

size_t PresetManager::getUserPresetCount() const {
    size_t count = 0;
    for (const auto& entry : presets_) {
        if (!entry.isFactory) {
            count++;
        }
    }
    return count;
}

int PresetManager::getCurrentPresetIndex() const {
    return currentPresetIndex_;
}

const Preset* PresetManager::getCurrentPreset() const {
    if (currentPresetIndex_ >= 0 && currentPresetIndex_ < static_cast<int>(presets_.size())) {
        return &presets_[currentPresetIndex_].preset;
    }
    return nullptr;
}

const Preset* PresetManager::getPreset(size_t index) const {
    if (index < presets_.size()) {
        return &presets_[index].preset;
    }
    return nullptr;
}

std::string PresetManager::getPresetName(size_t index) const {
    if (index < presets_.size()) {
        return presets_[index].preset.getName();
    }
    return "";
}

bool PresetManager::isFactoryPreset(size_t index) const {
    if (index < presets_.size()) {
        return presets_[index].isFactory;
    }
    return false;
}

bool PresetManager::loadPreset(size_t index) {
    clearError();

    if (index >= presets_.size()) {
        setError("Preset index out of range");
        return false;
    }

    currentPresetIndex_ = static_cast<int>(index);

    // Invoke callback if registered
    if (presetLoadedCallback_) {
        presetLoadedCallback_(presets_[index].preset);
    }

    return true;
}

bool PresetManager::nextPreset() {
    clearError();

    if (presets_.empty()) {
        setError("No presets available");
        return false;
    }

    // Wrap around to first preset
    currentPresetIndex_ = (currentPresetIndex_ + 1) % static_cast<int>(presets_.size());

    // Invoke callback if registered
    if (presetLoadedCallback_) {
        presetLoadedCallback_(presets_[currentPresetIndex_].preset);
    }

    return true;
}

bool PresetManager::previousPreset() {
    clearError();

    if (presets_.empty()) {
        setError("No presets available");
        return false;
    }

    // Wrap around to last preset
    currentPresetIndex_--;
    if (currentPresetIndex_ < 0) {
        currentPresetIndex_ = static_cast<int>(presets_.size()) - 1;
    }

    // Invoke callback if registered
    if (presetLoadedCallback_) {
        presetLoadedCallback_(presets_[currentPresetIndex_].preset);
    }

    return true;
}

bool PresetManager::savePreset(const std::string& name, const std::map<std::string, float>& parameters) {
    Preset preset(name);
    preset.setParameters(parameters);
    return savePreset(preset);
}

bool PresetManager::savePreset(const Preset& preset) {
    clearError();

    if (userPresetsPath_.empty()) {
        setError("User presets path not set");
        return false;
    }

    // Ensure user presets directory exists
    if (!ensureUserPresetsDirectoryExists()) {
        setError("Failed to create user presets directory");
        return false;
    }

    // Generate unique file name
    std::string fileName = generateUniquePresetFileName(preset.getName());
    std::string filePath = userPresetsPath_ + "/" + fileName;

    // Save to file
    if (!savePresetToFile(preset, filePath)) {
        return false; // Error already set
    }

    // Add to preset list
    PresetEntry entry;
    entry.preset = preset;
    entry.filePath = filePath;
    entry.isFactory = false;
    presets_.push_back(entry);

    return true;
}

bool PresetManager::overwritePreset(size_t index, const std::map<std::string, float>& parameters) {
    clearError();

    if (index >= presets_.size()) {
        setError("Preset index out of range");
        return false;
    }

    if (presets_[index].isFactory) {
        setError("Cannot overwrite factory preset");
        return false;
    }

    // Update preset parameters
    presets_[index].preset.setParameters(parameters);

    // Save to file
    if (!savePresetToFile(presets_[index].preset, presets_[index].filePath)) {
        return false; // Error already set
    }

    return true;
}

bool PresetManager::deletePreset(size_t index) {
    clearError();

    if (index >= presets_.size()) {
        setError("Preset index out of range");
        return false;
    }

    if (presets_[index].isFactory) {
        setError("Cannot delete factory preset");
        return false;
    }

    // Delete file
    if (unlink(presets_[index].filePath.c_str()) != 0) {
        setError("Failed to delete preset file");
        return false;
    }

    // Remove from list
    presets_.erase(presets_.begin() + index);

    // Adjust current preset index
    if (currentPresetIndex_ >= static_cast<int>(index)) {
        currentPresetIndex_--;
        if (currentPresetIndex_ < 0 && !presets_.empty()) {
            currentPresetIndex_ = 0;
        }
    }

    return true;
}

bool PresetManager::loadPresetFromFile(const std::string& filePath, Preset& outPreset) const {
    // Read file contents
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();

    // Parse JSON
    return outPreset.loadFromJSON(json);
}

bool PresetManager::savePresetToFile(const Preset& preset, const std::string& filePath) const {
    // Ensure file has .kdpreset extension
    std::string finalPath = filePath;
    if (finalPath.length() < 9 || finalPath.substr(finalPath.length() - 9) != ".kdpreset") {
        finalPath += ".kdpreset";
    }

    // Serialize to JSON
    std::string json = preset.toJSON();

    // Write to file
    std::ofstream file(finalPath);
    if (!file.is_open()) {
        return false;
    }

    file << json;
    file.close();

    return true;
}

bool PresetManager::refresh() {
    return initialize();
}

void PresetManager::setFactoryPresetsPath(const std::string& path) {
    factoryPresetsPath_ = path;
}

void PresetManager::setUserPresetsPath(const std::string& path) {
    userPresetsPath_ = path;
}

const std::string& PresetManager::getFactoryPresetsPath() const {
    return factoryPresetsPath_;
}

const std::string& PresetManager::getUserPresetsPath() const {
    return userPresetsPath_;
}

void PresetManager::setPresetLoadedCallback(std::function<void(const Preset&)> callback) {
    presetLoadedCallback_ = callback;
}

void PresetManager::clearPresetLoadedCallback() {
    presetLoadedCallback_ = nullptr;
}

const std::string& PresetManager::getLastError() const {
    return lastError_;
}

// Private helper methods

bool PresetManager::loadPresetsFromDirectory(const std::string& directory, bool isFactory) {
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        setError("Failed to open directory: " + directory);
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;

        // Skip . and ..
        if (fileName == "." || fileName == "..") {
            continue;
        }

        // Check for .kdpreset extension
        if (fileName.length() < 9 || fileName.substr(fileName.length() - 9) != ".kdpreset") {
            continue;
        }

        // Load preset from file
        std::string filePath = directory + "/" + fileName;
        Preset preset;
        if (loadPresetFromFile(filePath, preset)) {
            PresetEntry presetEntry;
            presetEntry.preset = preset;
            presetEntry.filePath = filePath;
            presetEntry.isFactory = isFactory;
            presets_.push_back(presetEntry);
        }
    }

    closedir(dir);

    // Directory iteration order is unspecified. Keep presets deterministic for
    // navigation, UI display, and state restoration by sorting each newly
    // loaded group by its file name.
    std::stable_sort(presets_.begin(), presets_.end(),
                     [](const PresetEntry& lhs, const PresetEntry& rhs) {
                         if (lhs.isFactory != rhs.isFactory) {
                             return lhs.isFactory && !rhs.isFactory;
                         }
                         return lhs.filePath < rhs.filePath;
                     });
    return true;
}

bool PresetManager::ensureUserPresetsDirectoryExists() {
    if (userPresetsPath_.empty()) {
        return false;
    }

    // Check if directory exists
    struct stat st;
    if (stat(userPresetsPath_.c_str(), &st) == 0) {
        // Directory exists
        return S_ISDIR(st.st_mode);
    }

    // Create directory
    if (mkdir(userPresetsPath_.c_str(), 0755) != 0) {
        return false;
    }

    return true;
}

std::string PresetManager::generateUniquePresetFileName(const std::string& presetName) const {
    std::string baseName = sanitizeFileName(presetName);
    std::string fileName = baseName + ".kdpreset";
    std::string filePath = userPresetsPath_ + "/" + fileName;

    // Check if file exists
    struct stat st;
    if (stat(filePath.c_str(), &st) != 0) {
        // File doesn't exist, use base name
        return fileName;
    }

    // File exists, append number
    int counter = 1;
    while (true) {
        fileName = baseName + "_" + std::to_string(counter) + ".kdpreset";
        filePath = userPresetsPath_ + "/" + fileName;

        if (stat(filePath.c_str(), &st) != 0) {
            // File doesn't exist
            return fileName;
        }

        counter++;
    }
}

std::string PresetManager::sanitizeFileName(const std::string& name) const {
    std::string sanitized;
    sanitized.reserve(name.length());

    for (char c : name) {
        if (std::isalnum(c) || c == '_' || c == '-' || c == ' ') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }

    // Trim whitespace
    size_t start = 0;
    while (start < sanitized.length() && std::isspace(sanitized[start])) {
        start++;
    }

    size_t end = sanitized.length();
    while (end > start && std::isspace(sanitized[end - 1])) {
        end--;
    }

    sanitized = sanitized.substr(start, end - start);

    // Replace multiple spaces with single underscore
    std::string result;
    bool lastWasSpace = false;
    for (char c : sanitized) {
        if (std::isspace(c)) {
            if (!lastWasSpace) {
                result += '_';
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }

    // Ensure not empty
    if (result.empty()) {
        result = "preset";
    }

    return result;
}

void PresetManager::setError(const std::string& error) {
    lastError_ = error;
}

void PresetManager::clearError() {
    lastError_.clear();
}

} // namespace KickDrum
