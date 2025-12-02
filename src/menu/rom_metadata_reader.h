#ifndef METADATA_READER_H
#define METADATA_READER_H

#include "path.h"
#include <mini.c/src/mini.h>

/* Try to extract metadata.ini from external .meta ZIP or embedded ZIP and
 * return a loaded mini_t* (caller must free with mini_free). Returns NULL on
 * failure. */
mini_t *metadata_load_from_meta_or_embedded(path_t *rom_path);

#endif // METADATA_READER_H
