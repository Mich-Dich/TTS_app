#pragma once


namespace AT::io {

	// Reads the content of a file into a string and returns it.
	// @param filepath The path to the file to be read.
	// @return The file content as a string. Returns an empty string on failure.
	std::string read_file(const std::filesystem::path& filepath);

	// Writes [content_buffer] to a file, overriding the previous content.
	// @param file_path The path to the file to be written.
	// @param content_buffer The vector of characters to be written to the file.
	// @return true if the file is successfully written, false otherwise.
	bool write_file(const std::filesystem::path& file_path, const std::vector<char>& content_buffer);

	// Copies a file to the specified target directory. Creates the target directory if it does not exist.
	// @param full_path_to_file The full path to the source file to copy.
	// @param target_directory The directory to which the file will be copied.
	// @return true if the file is successfully copied (overwriting existing file), false otherwise.
	bool copy_file(const std::filesystem::path& full_path_to_file, const std::filesystem::path& target_directory);

	// Creates a directory at the specified path if it doesn't already exist.
	// @param path The path to the directory to be created.
	// @return True if the directory is successfully created or already exists; false otherwise.
	bool create_directory(const std::filesystem::path& path);

	// Returns a list of process names that have the given file open (platform-specific).
	// @param filePath The wide string path to the file to check (platform-specific expected format).
	// @return A vector of process names (or PIDs/names depending on platform) that are using the file. Empty vector if none or on error.
	std::vector<std::string> get_processes_using_file(const std::wstring& filePath);

	// Checks whether the given path refers to a directory.
	// @param path The path to check.
	// @return true if the path is a directory, false otherwise.
	bool is_directory(const std::filesystem::path& path);

	// Checks whether the given path refers to a regular file.
	// @param path The path to check.
	// @return true if the path is a regular file, false otherwise.
	bool is_file(const std::filesystem::path& path);

	// Checks whether the filename is considered hidden by name (leading '.' character).
	// Note: this implementation tests the filename's first character (POSIX-style); it does NOT query filesystem attributes (Windows hidden attribute).
	// @param path The path to check.
	// @return true if the filename begins with a '.', false otherwise.
	bool is_hidden(const std::filesystem::path& path);

	// Returns the absolute (non-relative) form of the given path.
	// @param path The path to convert to an absolute path.
	// @return The absolute path as returned by std::filesystem::absolute.
	const std::filesystem::path get_absolute_path(const std::filesystem::path& path);

	// Returns a non-recursive list of files in the specified directory.
	// @param path The directory path to list files from.
	// @return A vector containing paths to regular files in the directory. Does not include subdirectories.
	std::vector<std::filesystem::path> get_files_in_dir(const std::filesystem::path& path);

	// Returns a non-recursive list of folders (subdirectories) in the specified directory.
	// @param path The directory path to list subdirectories from.
	// @return A vector containing paths to the subdirectories in the directory.
	std::vector<std::filesystem::path> get_folders_in_dir(const std::filesystem::path& path);

	// Writes a null-terminated C-string [data] to a file (text mode).
	// @param data The C-string data to be written to the file.
	// @param filename The path to the file to write to.
	// @return True if the write operation succeeds, false otherwise.
	bool write_to_file(const char* data, const std::filesystem::path& filename);


}
