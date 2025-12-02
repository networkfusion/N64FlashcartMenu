#ifndef ROM_METADATA_READER_H
#define ROM_METADATA_READER_H

#include "path.h"
#include <mini.c/src/mini.h>

/* Try to extract metadata.ini from external .meta ZIP or embedded ZIP and
 * return a loaded mini_t* (caller must free with mini_free). Returns NULL on
 * failure. */
mini_t *rom_metadata_load_from_ini_file(path_t *rom_path);

/*
 * Extract a file from the metadata ZIP (external .meta or embedded) into a
 * heap buffer. Returns pointer to heap data (caller must free with mz_free),
 * and sets *out_size if not NULL. Returns NULL on failure.
 */
void *rom_metadata_extract_file_to_heap(path_t *rom_path, const char *filename, size_t *out_size);

#endif // ROM_METADATA_READER_H
