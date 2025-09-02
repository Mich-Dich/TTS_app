#pragma once

#include <type_traits>
#include <glm/glm.hpp>

// struct ImVec2;
// struct ImVec4;


namespace AT::config {

	// Represents different configuration file types used by the system (underlying type: u8).
	enum class file : u8 {
		ui,				// UI related configuration.
		imgui,			// ImGui specific configuration.
		input,			// Input bindings / input-related configuration.
		app_settings,	// Application-wide settings.
	};

	// Represents operations that can be performed on configuration files (underlying type: u8).
	enum class operation : u8 {
		save,			// Save/write operation.
		load,			// Load/read operation.
	};

	// @brief Initializes the configuration files by creating necessary directories and default files.
	// @param dir The root directory where configuration files and the CONFIG_DIR will be created.
	// @return void This function does not have a return value.
	void init(std::filesystem::path dir);

	// @brief Creates configuration files for a specific project by ensuring the project's CONFIG_DIR exists and creating empty config files.
	// @param project_dir The project directory where project-specific configuration files will be stored.
	// @return void This function does not have a return value.
	void create_config_files_for_project(std::filesystem::path project_dir);

	// @brief Converts a configuration file enum value to its string representation.
	// @param type The file enum value to convert.
	// @return std::string The string name corresponding to the file enum (e.g., "ui", "imgui"). Returns "unknown" if the type is not recognized.
	std::string file_type_to_string(file type);

	// @brief Resolves a configuration file path for a given root directory and configuration file type.
	// @param root The root directory containing the CONFIG_DIR.
	// @param type The configuration file type to resolve.
	// @return std::filesystem::path The full path to the configuration file with the configured extension (e.g., CONFIG_DIR/<type>.config).
	std::filesystem::path get_filepath_from_configtype(std::filesystem::path root, file type);

	// @brief Resolves a configuration file path (INI extension) for a given root directory and configuration file type.
	// @param root The root directory containing the CONFIG_DIR.
	// @param type The configuration file type to resolve.
	// @return std::filesystem::path The full path to the configuration INI file (e.g., CONFIG_DIR/<type>.ini).
	std::filesystem::path get_filepath_from_configtype_ini(std::filesystem::path root, file type);

	// @brief Checks for the existence of a configuration entry in the specified configuration file. If found and override==true, the existing value is replaced.
	//        If found and override==false the current value is loaded into the provided value reference. If not found, the key/value pair is appended.
	// @param target_config_file The configuration file type to inspect (enum file).
	// @param section The section name in the configuration file where the key/value is expected (e.g., "graphics").
	// @param key The key to search for inside the section.
	// @param value Reference to a string that will be updated with the existing value (when override==false) or used to overwrite/append when override==true.
	// @param override If true, existing value is replaced with the provided value; if false, the provided value is overwritten by the existing value if found.
	// @return bool true if the key was found (and possibly updated/appended), false if not found or on failure.
	bool check_for_configuration(const file target_config_file, const std::string& section, const std::string& key, std::string& value, const bool override);

}
