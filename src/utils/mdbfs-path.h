#ifndef MDBFS_UTILS_PATH_H
#define MDBFS_UTILS_PATH_H

/**
 * Normalize an operating system path.
 *
 * This is basically a C wrapper of the std::filesystem::path::lexically_normal
 * function in C++ standard library, which reduces unnecessary parts in a path
 * (e.g. consecutive directory seperators). The returned memory is allocated
 * with mdbfs_malloc, and the caller is responsible for freeing it.
 *
 * @param path [in] The path string to be normalized.
 * @return The normalized path. On any failure, NULL is returned.
 */
char *mdbfs_path_lexically_normal(const char const* path);

/**
 * Check if a path is an absolute path.
 *
 * This is basically a C wrapper of the std::filesystem::path::is_absolute
 * function in C++ standard librarby, which checks if a std::filesystem::path
 * represents an absolute path or not. The returned memory is allocated with
 * mdbfs_malloc, and the caller is responsible for freeing it.
 *
 * @param path [in] The path string to be checked.
 * @return 1 if the path is an absolute path, 0 otherwise.
 */
int mdbfs_path_is_absolute(const char const* path);

#endif
