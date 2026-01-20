#pragma once

#include "Preset.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace KickDrum {

/**
 * @brief Manages preset loading, saving, and navigation
 * 
 * The PresetManager handles both factory presets (read-only, bundled with the application)
 * and user presets (read-write, stored in user documents folder). It provides:
 * - Preset list management
 * - Load/save/delete operations
 * - Next/previous navigation
 * - File I/O with .kdpreset extension
 * 
 * Factory presets are loaded from a bundled directory and cannot be modified or deleted.
 * User presets are stored in the user's documents folder and can be freely managed.
 * 
 * Requirements: 10.1, 10.2, 10.3, 10.5, 10.6, 14.5
 */
class PresetManager {
public:
    /**
     * @brief Construct a new PresetManager
     * 
     * @param factoryPresetsPath Path to factory presets directory (optional)
     * @param userPresetsPath Path to user presets directory (optional)
     */
    PresetManager(const std::string& factoryPresetsPath = "", 
                  const std::string& userPresetsPath = "");

    /**
     * @brief Destructor
     */
    ~PresetManager();

    /**
     * @brief Initialize the preset manager
     * 
     * Loads factory presets from the factory presets directory and
     * user presets from the user presets directory.
     * 
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Get the total number of presets (factory + user)
     * @return Total preset count
     */
    size_t getPresetCount() const;

    /**
     * @brief Get the number of factory presets
     * @return Factory preset count
     */
    size_t getFactoryPresetCount() const;

    /**
     * @brief Get the number of user presets
     * @return User preset count
     */
    size_t getUserPresetCount() const;

    /**
     * @brief Get the current preset index
     * @return Current preset index, or -1 if no preset selected
     */
    int getCurrentPresetIndex() const;

    /**
     * @brief Get the current preset
     * @return Pointer to current preset, or nullptr if no preset selected
     */
    const Preset* getCurrentPreset() const;

    /**
     * @brief Get a preset by index
     * 
     * @param index Preset index (0 to getPresetCount()-1)
     * @return Pointer to preset, or nullptr if index out of range
     */
    const Preset* getPreset(size_t index) const;

    /**
     * @brief Get preset name by index
     * 
     * @param index Preset index
     * @return Preset name, or empty string if index out of range
     */
    std::string getPresetName(size_t index) const;

    /**
     * @brief Check if a preset is a factory preset
     * 
     * @param index Preset index
     * @return true if factory preset, false if user preset or invalid index
     */
    bool isFactoryPreset(size_t index) const;

    /**
     * @brief Load a preset by index
     * 
     * Sets the current preset to the specified index and invokes the
     * preset loaded callback if registered.
     * 
     * @param index Preset index
     * @return true if successful, false if index out of range
     */
    bool loadPreset(size_t index);

    /**
     * @brief Load the next preset
     * 
     * Advances to the next preset in the list (wraps around to first preset).
     * 
     * @return true if successful, false if no presets available
     */
    bool nextPreset();

    /**
     * @brief Load the previous preset
     * 
     * Goes back to the previous preset in the list (wraps around to last preset).
     * 
     * @return true if successful, false if no presets available
     */
    bool previousPreset();

    /**
     * @brief Save the current state as a new user preset
     * 
     * Creates a new user preset with the specified name and parameters.
     * The preset is saved to the user presets directory with .kdpreset extension.
     * 
     * @param name Preset name
     * @param parameters Parameter values to save
     * @return true if successful, false on error
     */
    bool savePreset(const std::string& name, const std::map<std::string, float>& parameters);

    /**
     * @brief Save a preset object as a new user preset
     * 
     * @param preset Preset to save
     * @return true if successful, false on error
     */
    bool savePreset(const Preset& preset);

    /**
     * @brief Overwrite an existing user preset
     * 
     * Updates an existing user preset with new parameters. Cannot overwrite factory presets.
     * 
     * @param index Preset index (must be a user preset)
     * @param parameters New parameter values
     * @return true if successful, false if index invalid or is factory preset
     */
    bool overwritePreset(size_t index, const std::map<std::string, float>& parameters);

    /**
     * @brief Delete a user preset
     * 
     * Removes a user preset from the list and deletes the file.
     * Cannot delete factory presets.
     * 
     * @param index Preset index (must be a user preset)
     * @return true if successful, false if index invalid or is factory preset
     */
    bool deletePreset(size_t index);

    /**
     * @brief Load a preset from a file
     * 
     * Loads a preset from the specified file path. Does not add it to the
     * preset list or change the current preset.
     * 
     * @param filePath Path to .kdpreset file
     * @param outPreset Output preset object
     * @return true if successful, false on error
     */
    bool loadPresetFromFile(const std::string& filePath, Preset& outPreset) const;

    /**
     * @brief Save a preset to a file
     * 
     * Saves a preset to the specified file path with .kdpreset extension.
     * 
     * @param preset Preset to save
     * @param filePath Path to save to (should end with .kdpreset)
     * @return true if successful, false on error
     */
    bool savePresetToFile(const Preset& preset, const std::string& filePath) const;

    /**
     * @brief Refresh the preset list
     * 
     * Rescans the factory and user preset directories and reloads all presets.
     * Useful after external changes to preset files.
     * 
     * @return true if successful, false on error
     */
    bool refresh();

    /**
     * @brief Set the factory presets directory path
     * 
     * @param path Directory path
     */
    void setFactoryPresetsPath(const std::string& path);

    /**
     * @brief Set the user presets directory path
     * 
     * @param path Directory path
     */
    void setUserPresetsPath(const std::string& path);

    /**
     * @brief Get the factory presets directory path
     * @return Directory path
     */
    const std::string& getFactoryPresetsPath() const;

    /**
     * @brief Get the user presets directory path
     * @return Directory path
     */
    const std::string& getUserPresetsPath() const;

    /**
     * @brief Register a callback for when a preset is loaded
     * 
     * The callback is invoked whenever a preset is loaded (via loadPreset,
     * nextPreset, or previousPreset). The callback receives the loaded preset.
     * 
     * @param callback Callback function
     */
    void setPresetLoadedCallback(std::function<void(const Preset&)> callback);

    /**
     * @brief Clear the preset loaded callback
     */
    void clearPresetLoadedCallback();

    /**
     * @brief Get the last error message
     * @return Error message, or empty string if no error
     */
    const std::string& getLastError() const;

private:
    struct PresetEntry {
        Preset preset;
        std::string filePath;
        bool isFactory;
    };

    std::vector<PresetEntry> presets_;              ///< All presets (factory + user)
    int currentPresetIndex_;                        ///< Current preset index (-1 if none)
    std::string factoryPresetsPath_;                ///< Factory presets directory
    std::string userPresetsPath_;                   ///< User presets directory
    std::function<void(const Preset&)> presetLoadedCallback_;  ///< Preset loaded callback
    std::string lastError_;                         ///< Last error message

    // Helper methods
    bool loadPresetsFromDirectory(const std::string& directory, bool isFactory);
    bool ensureUserPresetsDirectoryExists();
    std::string generateUniquePresetFileName(const std::string& presetName) const;
    std::string sanitizeFileName(const std::string& name) const;
    void setError(const std::string& error);
    void clearError();
};

} // namespace KickDrum
