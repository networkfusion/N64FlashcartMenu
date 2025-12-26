#include <libdragon.h>
#include <mini.c/src/mini.h>

#include "ed64_vseries_state.h"
#include "utils/fs.h"

#ifndef ED64_VSERIES_STATE_FILE_PATH
#define ED64_VSERIES_STATE_FILE_PATH  "sd:/menu/ed64_vseries_state.ini" // TODO: just save to config.ini?
#endif

static ed64_vseries_pseudo_writeback_t init = {
    .last_save_path = "",
    .last_save_type = 0,
    .is_expecting_save_writeback = false,
};


void ed64_vseries_state_load (ed64_vseries_pseudo_writeback_t *state) {
    if (!file_exists(ED64_VSERIES_STATE_FILE_PATH)) {
        ed64_vseries_state_save(&init);
    }

    mini_t *ini = mini_try_load(ED64_VSERIES_STATE_FILE_PATH);

    state->last_save_path = strdup(mini_get_string(ini, "ED64_STATE", "last_save_path", init.last_save_path));
    state->last_save_type = mini_get_int(ini, "ED64_STATE", "last_save_type", init.last_save_type);
    state->is_expecting_save_writeback = mini_get_bool(ini, "ED64_STATE", "is_expecting_save_writeback", init.is_expecting_save_writeback);

    mini_free(ini);
}

void ed64_vseries_state_save (ed64_vseries_pseudo_writeback_t *state) {
    mini_t *ini = mini_create(ED64_VSERIES_STATE_FILE_PATH);

    mini_set_string(ini, "ED64_STATE", "last_save_path", state->last_save_path);
    mini_set_int(ini, "ED64_STATE", "last_save_type", state->last_save_type);
    mini_set_bool(ini, "ED64_STATE", "is_expecting_save_writeback", state->is_expecting_save_writeback);

    mini_save(ini, MINI_FLAGS_SKIP_EMPTY_GROUPS);

    mini_free(ini);
}
