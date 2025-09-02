#pragma once


namespace AT::util {
    
    // Determines whether [path] is a valid project directory.
    // A valid project directory must exist, be a directory, and contain at least
    // one regular file whose extension equals PROJECT_EXTENTION.
    // @param path The filesystem path to test.
    // @return True if [path] exists, is a directory, and contains a project file; false otherwise.
    bool is_valid_project_dir(const std::filesystem::path& path);

    // Extracts a subpath starting at the project's content folder marker (CONTENT_DIR).
    // The returned path begins with the CONTENT_DIR marker and contains the remainder
    // of [full_path] from that marker onwards. If the marker is not found an empty path is returned.
    // @param full_path The full filesystem path to extract from.
    // @return A path starting at CONTENT_DIR (inclusive) if found; otherwise an empty path.
    std::filesystem::path extract_path_from_project_folder(const std::filesystem::path& full_path);

    // Extracts the portion of [full_path] that comes after the project's content folder marker (CONTENT_DIR).
    // This function iterates the path parts and returns the subpath composed of the parts that follow
    // CONTENT_DIR. Note: CONTENT_DIR itself is NOT included in the returned path.
    // @param full_path The full filesystem path to extract from.
    // @return A path composed of the parts following CONTENT_DIR; empty if CONTENT_DIR is not present.
    std::filesystem::path extract_path_from_project_content_folder(const std::filesystem::path& full_path);

    // Extracts the portion of [full_path] that comes after the specified [directory] name.
    // Iterates the path parts and returns the subpath composed of the parts that follow [directory].
    // The [directory] part itself is NOT included in the returned path.
    // @param full_path The full filesystem path to extract from.
    // @param directory The directory name to search for inside [full_path].
    // @return A path composed of the parts following [directory]; empty if [directory] is not present.
    std::filesystem::path extract_path_from_directory(const std::filesystem::path& full_path, const std::string& directory);

}
