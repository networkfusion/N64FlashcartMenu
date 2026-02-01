/**
 * @file rom_metadata.h
 * @brief ROM Metadata extraction and loading
 * @ingroup menu
 *
 * This module handles ROM metadata loading from multiple sources following the
 * N64Brew ROM Metadata specification (https://n64brew.dev/wiki/ROM_Metadata).
 *
 * Metadata is loaded in the following priority order:
 * 1. External .meta file (ZIP next to ROM) - highest priority
 * 2. Embedded metadata in ROM (ZIP appended to ROM)
 * 3. External metadata.ini file (plain INI in same directory)
 * 4. Centralized database (metadata/GAMECODE/metadata.ini)
 * 5. Default values - lowest priority
 */

#ifndef ROM_METADATA_H
#define ROM_METADATA_H

#include "rom_info.h"
#include "path.h"

/**
 * @brief Load ROM metadata from external or embedded sources
 *
 * Attempts to load ROM metadata using the following strategy:
 * - Checks for external .meta file (ZIP archive with metadata.ini)
 * - Checks for embedded metadata in ROM (indicated by byte 0x38 bit 0)
 * - Checks for external metadata.ini in same directory
 * - Checks for database entry (metadata/GAMECODE/metadata.ini)
 * - Falls back to default values if no metadata found
 *
 * @param path Path to the ROM file
 * @param rom_info Pointer to rom_info_t structure to populate
 *
 * @see https://n64brew.dev/wiki/ROM_Metadata
 */
void rom_metadata_load(path_t *path, rom_info_t *rom_info);

#endif // ROM_METADATA_H
