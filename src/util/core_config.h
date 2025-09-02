#pragma once

// -------------------------------------------------------------------------------------------------------------------------------------------
// Defines that have influence on system behavior
// -------------------------------------------------------------------------------------------------------------------------------------------

#define RENDER_API_OPENGL
// #define RENDER_API_VULKAN


// collect timing-data from every magor function?
#define PROFILE								    0	// general

// log assert and validation behaviour?
// NOTE - expr in assert/validation will still be executed
#define ENABLE_LOGGING_FOR_ASSERTS              1
#define ENABLE_LOGGING_FOR_VALIDATION           1


// Extension for asset files
#define AT_ASSET_EXTENTION			".atasset"

// Extension for project files
#define PROJECT_EXTENTION    		".atproj"

// Configuration file extensions
#define FILE_EXTENSION_CONFIG   	".yml"        	// Extension for YAML config files
#define FILE_EXTENSION_INI      	".ini"          // Extension for INI config files

// Temporary directory for DLL builds
#define PROJECT_TEMP_DLL_PATH 		"build_DLL"

// Directory structure macros
#define METADATA_DIR            	"metadata"      // Directory for metadata files
#define CONFIG_DIR              	"config"        // Directory for configuration files
#define CONTENT_DIR             	"content"       // Directory for content files
#define ASSET_DIR             	    "assets"        // Directory for asset files
#define SOURCE_DIR              	"src"           // Directory for source code

#define PROJECT_PATH				application::get().get_project_path()
#define PROJECT_NAME				application::get().get_project_data().name

#define ASSET_PATH					util::get_executable_path() / "assets"

