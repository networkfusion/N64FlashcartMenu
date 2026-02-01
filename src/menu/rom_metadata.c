/**
 * @file rom_metadata.c
 * @brief ROM Metadata extraction implementation
 * @ingroup menu
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <mini.c/src/mini.h>
#include <miniz/miniz.h>

#include "rom_metadata.h"
#include "utils/fs.h"

/**
 * @brief Check if a file is a ZIP archive
 *
 * Detects ZIP files by checking for the ZIP local file header signature.
 * ZIP files start with 0x04034b50 (little-endian), appearing as 'PK\x03\x04' in ASCII.
 *
 * @param filepath Path to the file to check
 * @return True if file is a ZIP archive, false otherwise
 */
static bool is_zip_file(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        return false;
    }

    // ZIP files start with the local file header signature: 0x04034b50 (little-endian)
    // which appears as 'PK\x03\x04' in ASCII
    uint32_t signature;
    bool is_zip = (fread(&signature, sizeof(uint32_t), 1, f) == 1) && (signature == 0x04034b50);
    fclose(f);

    return is_zip;
}

/**
 * @brief Attempt to extract metadata.ini from a ZIP file
 *
 * Extracts metadata.ini from a ZIP archive by:
 * 1. Extracting to memory using miniz
 * 2. Writing to a temporary file
 * 3. Loading INI data with mini_load
 * 4. Cleaning up the temporary file
 *
 * @param filepath Path to the ZIP file
 * @return Loaded INI structure on success, NULL on failure
 */
static mini_t *try_load_meta_from_zip(const char *filepath) {
    size_t extracted_size = 0;
    void *extracted_data = mz_zip_extract_archive_file_to_heap(filepath, "metadata.ini", &extracted_size, 0);

    if (!extracted_data || extracted_size == 0) {
        return NULL;
    }

    // Create a temporary file to hold the extracted INI data
    // Use /tmp or system temp directory
    char temp_path[] = "/tmp/rom_metadata_XXXXXX";
    int temp_fd = mkstemp(temp_path);

    if (temp_fd < 0) {
        free(extracted_data);
        return NULL;
    }

    // Write extracted data to temporary file
    ssize_t written = write(temp_fd, extracted_data, extracted_size);
    close(temp_fd);

    if (written != (ssize_t)extracted_size) {
        free(extracted_data);
        unlink(temp_path);
        return NULL;
    }

    // Load INI from temporary file
    mini_t *rom_meta_ini = mini_load(temp_path);

    // Clean up temporary file and extracted data
    unlink(temp_path);
    free(extracted_data);

    return rom_meta_ini;
}

/**
 * @brief Check if ROM has embedded metadata
 *
 * Examines byte 0x38 bit 0 in the ROM header to determine if embedded
 * metadata (appended ZIP) is present. This follows the N64Brew specification.
 *
 * @param rom_path Path to the ROM file
 * @return True if embedded metadata indicator is set, false otherwise
 */
static bool has_embedded_metadata(const char *rom_path) {
    FILE *f = fopen(rom_path, "rb");
    if (!f) {
        return false;
    }

    // Seek to byte 0x38 in the ROM header
    if (fseek(f, 0x38, SEEK_SET) != 0) {
        fclose(f);
        return false;
    }

    uint8_t byte;
    bool has_embedded = (fread(&byte, 1, 1, f) == 1) && (byte & 0x01);
    fclose(f);

    return has_embedded;
}

/**
 * @brief Attempt to extract metadata from ROM-embedded ZIP
 *
 * Checks if the ROM has the embedded metadata indicator set (byte 0x38 bit 0),
 * then extracts metadata.ini from the ZIP appended to the ROM by:
 * 1. Checking for embedded metadata indicator
 * 2. Extracting to memory using miniz
 * 3. Writing to a temporary file
 * 4. Loading INI data with mini_load
 * 5. Cleaning up the temporary file
 *
 * Since ZIP files are parsed from the end, miniz can read the appended ZIP
 * directly from the ROM file without needing to extract it first.
 *
 * @param rom_path Path to the ROM file
 * @return Loaded INI structure on success, NULL on failure
 *
 * @see https://n64brew.dev/wiki/ROM_Metadata#Embedded_metadata
 */
static mini_t *try_load_meta_from_rom_embedded(const char *rom_path) {
    // Check if embedded metadata is present
    if (!has_embedded_metadata(rom_path)) {
        return NULL;
    }

    // Try to extract metadata.ini from the ROM (which contains an appended ZIP)
    // ZIP libraries can parse from the end of the file, so we pass the ROM itself
    size_t extracted_size = 0;
    void *extracted_data = mz_zip_extract_archive_file_to_heap(rom_path, "metadata.ini", &extracted_size, 0);

    if (!extracted_data || extracted_size == 0) {
        return NULL;
    }

    // Create a temporary file to hold the extracted INI data
    char temp_path[] = "/tmp/rom_metadata_XXXXXX";
    int temp_fd = mkstemp(temp_path);

    if (temp_fd < 0) {
        free(extracted_data);
        return NULL;
    }

    // Write extracted data to temporary file
    ssize_t written = write(temp_fd, extracted_data, extracted_size);
    close(temp_fd);

    if (written != (ssize_t)extracted_size) {
        free(extracted_data);
        unlink(temp_path);
        return NULL;
    }

    // Load INI from temporary file
    mini_t *rom_meta_ini = mini_load(temp_path);

    // Clean up temporary file and extracted data
    unlink(temp_path);
    free(extracted_data);

    return rom_meta_ini;
}

/**
 * @brief Load ROM metadata from external or embedded sources
 *
 * Attempts to load ROM metadata using the following priority order:
 *
 * 1. **External .meta file** (ZIP archive next to ROM)
 *    - Can be created by users to provide custom metadata
 *    - Takes precedence over all other sources
 *
 * 2. **Embedded metadata in ROM** (ZIP appended to ROM)
 *    - Indicated by bit 0 set in byte 0x38 of ROM header
 *    - Preferred for homebrew/romhacks (self-contained)
 *    - Follows N64Brew ROM Metadata specification v1.0
 *
 * 3. **External metadata.ini** (plain INI in same directory)
 *    - Already-extracted metadata files
 *    - Useful if ZIP extraction was previously done
 *
 * 4. **Centralized database** (metadata/GAMECODE/metadata.ini)
 *    - Game code extracted from ROM header
 *    - Allows organizing metadata by game code
 *    - Useful for managing metadata across multiple ROMs
 *
 * 5. **Default values**
 *    - Name: "" (empty)
 *    - Author: "Unknown"
 *    - Release date: "Unknown"
 *    - License: "Unknown"
 *    - Website: "Unknown"
 *    - Age rating: 0
 *    - Description: "" (empty)
 *
 * @param path Path to the ROM file (path_t structure)
 * @param rom_info Pointer to rom_info_t structure to populate with metadata
 *
 * @see https://n64brew.dev/wiki/ROM_Metadata
 * @see https://n64brew.dev/wiki/ROM_Header
 */
void rom_metadata_load(path_t *path, rom_info_t *rom_info) {
    path_t *rom_info_meta_path = path_clone(path);
    const char *meta_path = NULL;

    // 1. First, check external .meta file (highest priority per spec)
    path_ext_replace(rom_info_meta_path, "meta");
    if (file_exists(path_get(rom_info_meta_path))) {
        // Check if .meta file is a ZIP archive
        if (is_zip_file(path_get(rom_info_meta_path))) {
            // Try to extract metadata.ini from the ZIP
            mini_t *zip_meta = try_load_meta_from_zip(path_get(rom_info_meta_path));
            if (zip_meta) {
                // Successfully loaded metadata from ZIP, use it directly
                rom_info->meta.name = strdup(mini_get_string(zip_meta, "meta", "name", ""));
                rom_info->meta.author = strdup(mini_get_string(zip_meta, "meta", "author", ""));
                rom_info->meta.release_date = strdup(mini_get_string(zip_meta, "meta", "release-date", ""));
                rom_info->meta.osi_license = strdup(mini_get_string(zip_meta, "meta", "osi-license", ""));
                rom_info->meta.website = strdup(mini_get_string(zip_meta, "meta", "website", ""));
                rom_info->meta.age_rating = mini_get_int(zip_meta, "meta", "age-rating", 0);
                rom_info->meta.short_description = strdup(mini_get_string(zip_meta, "meta", "short-desc", ""));
                mini_free(zip_meta);
                path_free(rom_info_meta_path);
                return;
            }
            // If ZIP extraction fails, the fallback will try other sources
        } else {
            // .meta file exists and is not a ZIP, use it directly
            meta_path = path_get(rom_info_meta_path);
        }
    }

    // 2. If no external metadata, check for embedded metadata in ROM
    if (!meta_path) {
        mini_t *embedded_meta = try_load_meta_from_rom_embedded(path_get(path));
        if (embedded_meta) {
            // Successfully loaded embedded metadata, use it directly
            rom_info->meta.name = strdup(mini_get_string(embedded_meta, "meta", "name", ""));
            rom_info->meta.author = strdup(mini_get_string(embedded_meta, "meta", "author", ""));
            rom_info->meta.release_date = strdup(mini_get_string(embedded_meta, "meta", "release-date", ""));
            rom_info->meta.osi_license = strdup(mini_get_string(embedded_meta, "meta", "osi-license", ""));
            rom_info->meta.website = strdup(mini_get_string(embedded_meta, "meta", "website", ""));
            rom_info->meta.age_rating = mini_get_int(embedded_meta, "meta", "age-rating", 0);
            rom_info->meta.short_description = strdup(mini_get_string(embedded_meta, "meta", "short-desc", ""));
            mini_free(embedded_meta);
            path_free(rom_info_meta_path);
            return;
        }
        // If embedded extraction fails, the fallback will try external sources
    }

    // 3. If we still don't have a meta path, try metadata.ini as fallback
    if (!meta_path) {
        // Create a fresh path clone to avoid conflicts with previous modifications
        path_t *ini_path = path_clone(path);
        path_ext_replace(ini_path, "metadata.ini");

        if (file_exists(path_get(ini_path))) {
            meta_path = path_get(ini_path);
            path_free(rom_info_meta_path);
            rom_info_meta_path = ini_path;
        } else {
            path_free(ini_path);
            // 4. Try to find the file in database using ROM game code
            // Database structure: metadata/N/B/7/T/metadata.ini or menu/metadata/N/B/7/T/metadata.ini
            // Navigate back to storage root and look for metadata directory
            
            // Create a string for the game code (null-terminated)
            char game_code_str[5] = {0};
            strncpy(game_code_str, rom_info->game_code, 4);

            bool found = false;
            path_t *db_path = NULL;

            // Try multiple paths - navigate back up to 10 levels to find storage root
            // ROMs can be at various depths: roms/game.z64, N64/game.z64, NTSC/USA/A_ROMs/game.z64, etc.
            for (int levels = 1; levels <= 10 && !found; levels++) {
                db_path = path_clone(path);
                
                // Pop the filename and go back 'levels' directories
                path_pop(db_path);  // Remove filename
                for (int i = 1; i < levels; i++) {
                    path_pop(db_path);
                }
                
                // Try two patterns: menu/metadata and just metadata
                for (int p = 0; p < 2 && !found; p++) {
                    path_t *test_path = path_clone(db_path);
                    
                    if (p == 0) {
                        path_push(test_path, "menu");
                        path_push(test_path, "metadata");
                    } else {
                        path_push(test_path, "metadata");
                    }
                    
                    // Push each character of the game code as a separate directory
                    char char_path[2] = {0, 0};
                    for (int i = 0; i < 4; i++) {
                        char_path[0] = game_code_str[i];
                        path_push(test_path, char_path);
                    }
                    
                    path_push(test_path, "metadata.ini");

                    if (file_exists(path_get(test_path))) {
                        db_path = test_path;
                        meta_path = path_get(db_path);
                        path_free(rom_info_meta_path);
                        rom_info_meta_path = db_path;
                        found = true;
                    } else {
                        path_free(test_path);
                    }
                }
                
                if (!found) {
                    path_free(db_path);
                }
            }

            if (!found) {
                // Database file does not exist at any level, return with defaults
                path_free(rom_info_meta_path);
                return;
            }
        }
    }

    mini_t *rom_meta_ini = mini_load(meta_path);

    if (rom_meta_ini) {
        rom_info->meta.name = strdup(mini_get_string(rom_meta_ini, "meta", "name", ""));
        rom_info->meta.author = strdup(mini_get_string(rom_meta_ini, "meta", "author", ""));
        rom_info->meta.release_date = strdup(mini_get_string(rom_meta_ini, "meta", "release-date", ""));
        rom_info->meta.osi_license = strdup(mini_get_string(rom_meta_ini, "meta", "osi-license", ""));
        rom_info->meta.website = strdup(mini_get_string(rom_meta_ini, "meta", "website", ""));
        rom_info->meta.age_rating = mini_get_int(rom_meta_ini, "meta", "age-rating", 0);
        rom_info->meta.short_description = strdup(mini_get_string(rom_meta_ini, "meta", "short-desc", ""));

        mini_free(rom_meta_ini);
    }

    path_free(rom_info_meta_path);
}
