#ifndef UTILS_FS_H__
#define UTILS_FS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @def FS_SECTOR_SIZE
 * @brief The size of a file system sector in bytes.
 */
#define FS_SECTOR_SIZE      (512)

/**
 * @def FS_MAX_PATH_SCAN_LENGTH
 * @brief Maximum path length scanned for defensive string validation.
 */
#define FS_MAX_PATH_SCAN_LENGTH (1024)

/**
 * @def FS_MAX_EXTENSION_LENGTH
 * @brief Maximum extension length scanned for defensive string validation.
 */
#define FS_MAX_EXTENSION_LENGTH (16)

/**
 * @file fs.h
 * @brief File system utility functions for file and directory operations.
 * @ingroup utils
 */

/**
 * @brief Strip the file system prefix from a path.
 *
 * Removes the file system prefix (such as ":/") from the provided path string.
 *
 * @param path The path from which to strip the prefix.
 * @return A pointer to the path without the prefix.
 */
char *strip_fs_prefix(char *path);

/**
 * @brief Get the basename of a path.
 *
 * Returns a pointer to the basename (the final component) of the provided path.
 *
 * @param path The path from which to get the basename.
 * @return A pointer to the basename of the path.
 */
char *file_basename(char *path);

/**
 * @brief Check if a file exists at the given path.
 *
 * Checks if a file exists at the specified path.
 *
 * @param path The path to the file.
 * @return true if the file exists, false otherwise.
 */
bool file_exists(char *path);

/**
 * @brief Get the size of a file at the given path.
 *
 * Returns the size of the file at the specified path in bytes.
 *
 * @param path The path to the file.
 * @return The size of the file in bytes, or -1 if the file does not exist or an error occurs.
 */
int64_t file_get_size(char *path);

/**
 * @brief Allocate a file of the specified size at the given path.
 *
 * Creates a file of the specified size at the provided path. The file is filled with zeros.
 *
 * @param path The path to the file.
 * @param size The size of the file to create in bytes.
 * @return true if the file was successfully created, false otherwise.
 */
bool file_allocate(char *path, size_t size);

/**
 * @brief Fill a file with the specified value.
 *
 * Fills the file at the given path with the specified byte value.
 *
 * @param path The path to the file.
 * @param value The value to fill the file with (byte).
 * @return true if the file was successfully filled, false otherwise.
 */
bool file_fill(char *path, uint8_t value);

/**
 * @brief Read a text file into a newly allocated buffer.
 *
 * Reads the full file from disk into a heap-allocated, NUL-terminated buffer.
 *
 * @param path The path to the file.
 * @param max_size Maximum allowed file size in bytes.
 * @param contents Output pointer receiving allocated text contents.
 * @param length Output pointer receiving file length (without terminator).
 * @return true if the file was read successfully, false otherwise.
 */
bool file_try_read_text(const char *path, size_t max_size, char **contents, size_t *length);

/** @brief Error codes for text file load helper. */
typedef enum {
	FILE_READ_TEXT_OK = 0,
	FILE_READ_TEXT_ERR_INVALID_ARGS,
	FILE_READ_TEXT_ERR_OPEN,
	FILE_READ_TEXT_ERR_SEEK,
	FILE_READ_TEXT_ERR_SIZE,
	FILE_READ_TEXT_ERR_EMPTY,
	FILE_READ_TEXT_ERR_TOO_BIG,
	FILE_READ_TEXT_ERR_ALLOC,
	FILE_READ_TEXT_ERR_READ,
	FILE_READ_TEXT_ERR_CLOSE,
} file_read_text_err_t;

/**
 * @brief Read a text file into a newly allocated buffer with detailed error codes.
 *
 * Reads the full file from disk into a heap-allocated, NUL-terminated buffer.
 *
 * @param path The path to the file.
 * @param max_size Maximum allowed file size in bytes.
 * @param contents Output pointer receiving allocated text contents.
 * @param length Output pointer receiving file length (without terminator).
 * @param error Output pointer receiving specific failure reason (optional).
 * @return true if the file was read successfully, false otherwise.
 */
bool file_try_read_text_ex(const char *path, size_t max_size, char **contents, size_t *length, file_read_text_err_t *error);

/**
 * @brief Read an exact number of bytes from a file into a caller-provided buffer.
 *
 * @param path The path to the file.
 * @param buffer Output buffer to fill.
 * @param size Number of bytes required.
 * @return true if exactly size bytes were read and the file was closed successfully.
 */
bool file_try_read_exact(const char *path, void *buffer, size_t size);

/**
 * @brief Check if a file has one of the specified extensions.
 *
 * Checks if the file at the given path has one of the specified extensions.
 *
 * @param path The path to the file.
 * @param extensions An array of extensions to check (NULL-terminated).
 * @return true if the file has one of the specified extensions, false otherwise.
 */
bool file_has_extensions(char *path, const char *extensions[]);

/**
 * @brief Check if a directory exists at the given path.
 *
 * Checks if a directory exists at the specified path.
 *
 * @param path The path to the directory.
 * @return true if the directory exists, false otherwise.
 */
bool directory_exists(char *path);

/**
 * @brief Create a directory at the given path.
 *
 * Creates a directory at the specified path, including any necessary parent directories.
 *
 * @param path The path to the directory.
 * @return false if the directory was successfully created, true if there was an error.
 */
bool directory_create(char *path);

#endif // UTILS_FS_H__
