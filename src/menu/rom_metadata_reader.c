#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <miniz.h>
#include <miniz_zip.h>
#include <errno.h>

#include "utils/fs.h"
#include "path.h"
#include "mini.c/src/mini.h"

/*
 * Try to extract "metadata.ini" from either an external .meta ZIP file (same
 * name as the ROM, extension .meta) or an embedded ZIP appended to the ROM
 * (bit 0 in byte 0x38 set). If found, write a temporary ini file next to the
 * ROM and return a loaded mini_t * (caller must mini_free it). Returns NULL
 * on failure.
 */
mini_t *metadata_load_from_meta_or_embedded(path_t *rom_path) {
    if (!path_has_value(rom_path)) {
        return NULL;
    }

    path_t *meta_path = path_clone(rom_path);
    path_ext_replace(meta_path, "meta");

    const char *zip_candidate = NULL;

    if (file_exists(path_get(meta_path))) {
        zip_candidate = path_get(meta_path);
    } else {
        /* Check embedded flag byte 0x38 */
        FILE *f = fopen(path_get(rom_path), "rb");
        if (f) {
            if (fseek(f, 0x38, SEEK_SET) == 0) {
                int b = fgetc(f);
                if (b != EOF) {
                    if (b & 0x01) {
                        /* Treat the ROM file itself as a ZIP-containing file */
                        zip_candidate = path_get(rom_path);
                    }
                }
            }
            fclose(f);
        }
    }

    mini_t *result = NULL;

    if (zip_candidate) {
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));

        if (mz_zip_reader_init_file(&zip, zip_candidate, 0)) {
            int fi = mz_zip_reader_locate_file(&zip, "metadata.ini", NULL, 0);
            if (fi >= 0) {
                size_t size = 0;
                void *data = mz_zip_reader_extract_to_heap(&zip, fi, &size, 0);
                if (data && size > 0) {
                    /* write to a temporary ini path next to ROM */
                    path_t *tmp = path_clone(rom_path);
                    char *basename = strdup(path_last_get(tmp));
                    path_pop(tmp); /* now tmp is directory */

                    char tmpname[256];
                    snprintf(tmpname, sizeof(tmpname), "%s.meta.ini", basename);
                    free(basename);

                    path_push(tmp, tmpname);

                    FILE *out = fopen(path_get(tmp), "wb");
                    if (out) {
                        fwrite(data, 1, size, out);
                        fclose(out);

                        /* Load with mini and then remove temp file */
                        result = mini_load(path_get(tmp));

                        /* cleanup temp file */
                        if (remove(path_get(tmp)) && errno != ENOENT) {
                            /* ignore removal errors */
                        }
                    }

                    path_free(tmp);
                    mz_free(data);
                }
            }

            mz_zip_reader_end(&zip);
        }
    }

    path_free(meta_path);
    return result;
}
