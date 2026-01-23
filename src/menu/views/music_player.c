#include <stdlib.h>
#include <string.h>

#include "views.h"
#include "../sound.h"
#include "../mp3_player.h"
#include "../ui_components.h"

/* Playback mode enum local to the view */
typedef enum {
    MP3_MODE_SINGLE = 0,
    MP3_MODE_REPEAT_ONE,
    MP3_MODE_REPEAT_ALL,
    MP3_MODE_PLAY_FOLDER,
    MP3_MODE_SHUFFLE,
    MP3_MODE_COUNT
} mp3_play_mode_t;

static mp3_play_mode_t play_mode = MP3_MODE_SINGLE;

/* Playlist stored by the view (built from the current browser directory) */
static char **playlist_paths = NULL;
static size_t playlist_count = 0;
static size_t playlist_index = 0; /* index in playlist_paths for current track */
static int *shuffle_order = NULL;
static size_t shuffle_pos = 0;

static void free_playlist(void) {
    if (playlist_paths) {
        for (size_t i = 0; i < playlist_count; i++) {
            free(playlist_paths[i]);
        }
        free(playlist_paths);
        playlist_paths = NULL;
    }
    if (shuffle_order) {
        free(shuffle_order);
        shuffle_order = NULL;
    }
    playlist_count = 0;
    playlist_index = 0;
    shuffle_pos = 0;
}

static void build_playlist_from_browser(menu_t *menu) {
    free_playlist();

    if (!menu || !menu->browser.list || menu->browser.entries == 0) return;

    /* Count music entries in current browser listing */
    size_t count = 0;
    for (int i = 0; i < menu->browser.entries; i++) {
        if (menu->browser.list[i].type == ENTRY_TYPE_MUSIC) count++;
    }
    if (count == 0) return;

    playlist_paths = calloc(count, sizeof(char*));
    if (!playlist_paths) return;

    size_t idx = 0;
    for (int i = 0; i < menu->browser.entries; i++) {
        if (menu->browser.list[i].type == ENTRY_TYPE_MUSIC) {
            path_t *p = path_clone_push(menu->browser.directory, menu->browser.list[i].name);
            if (p) {
                playlist_paths[idx] = strdup(path_get(p));
                path_free(p);
                if (playlist_paths[idx]) idx++;
            }
        }
    }
    playlist_count = idx;

    /* default starting position: find current track's path and set playlist_index accordingly */
    if (menu->browser.entry) {
        char *current_path = NULL;
        path_t *current = path_clone_push(menu->browser.directory, menu->browser.entry->name);
        if (current) { current_path = path_get(current); }
        if (current_path) {
            for (size_t i = 0; i < playlist_count; i++) {
                if (strcmp(current_path, playlist_paths[i]) == 0) {
                    playlist_index = i;
                    break;
                }
            }
        }
        if (current) path_free(current);
    }

    /* prepare shuffle order if needed (deferred until shuffle mode is selected) */
}

static void prepare_shuffle_order(void) {
    if (shuffle_order) free(shuffle_order);
    if (playlist_count == 0) return;
    shuffle_order = malloc(sizeof(int) * playlist_count);
    if (!shuffle_order) return;
    for (size_t i = 0; i < playlist_count; i++) shuffle_order[i] = (int)i;
    /* Fisher-Yates */
    for (size_t i = playlist_count - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        int tmp = shuffle_order[i]; shuffle_order[i] = shuffle_order[j]; shuffle_order[j] = tmp;
    }
    /* find shuffle_pos matching current playlist_index so playback continues from current track */
    for (size_t i = 0; i < playlist_count; i++) {
        if (shuffle_order[i] == (int)playlist_index) { shuffle_pos = i; break; }
    }
}

static const char *mode_to_string(mp3_play_mode_t m) {
    switch (m) {
        case MP3_MODE_SINGLE: return "Single";
        case MP3_MODE_REPEAT_ONE: return "Repeat 1";
        case MP3_MODE_REPEAT_ALL: return "Repeat All";
        case MP3_MODE_PLAY_FOLDER: return "Folder";
        case MP3_MODE_SHUFFLE: return "Shuffle";
        default: return "Unknown";
    }
}

static void start_track_by_index(menu_t *menu, size_t idx) {
    if (idx >= playlist_count) return;
    mp3player_stop();
    mp3player_unload();
    mp3player_err_t err = mp3player_load(playlist_paths[idx]);
    if (err != MP3PLAYER_OK) {
        /* skip this file and try next */
        debugf("MP3: failed to load %s\n", playlist_paths[idx]);
        return;
    }
    mp3player_play();
}

static void start_next_track(menu_t *menu) {
    if (playlist_count == 0) return;
    size_t next_index = playlist_index;
    switch (play_mode) {
        case MP3_MODE_SINGLE:
            /* do nothing */
            return;
        case MP3_MODE_REPEAT_ONE:
            /* restart same track */
            start_track_by_index(menu, playlist_index);
            return;
        case MP3_MODE_PLAY_FOLDER:
            if (playlist_index + 1 < playlist_count) {
                playlist_index++;
                start_track_by_index(menu, playlist_index);
            } else {
                /* end of folder: stop */
                mp3player_stop();
            }
            return;
        case MP3_MODE_REPEAT_ALL:
            playlist_index = (playlist_index + 1) % playlist_count;
            start_track_by_index(menu, playlist_index);
            return;
        case MP3_MODE_SHUFFLE:
            if (!shuffle_order) prepare_shuffle_order();
            if (shuffle_pos + 1 < playlist_count) {
                shuffle_pos++;
                playlist_index = (size_t)shuffle_order[shuffle_pos];
                start_track_by_index(menu, playlist_index);
            } else {
                /* shuffle-once: finished */
                mp3player_stop();
            }
            return;
        default:
            return;
    }
}

static void start_prev_track(menu_t *menu) {
    if (playlist_count == 0) return;
    switch (play_mode) {
        case MP3_MODE_SINGLE:
            return;
        case MP3_MODE_REPEAT_ONE:
            start_track_by_index(menu, playlist_index);
            return;
        case MP3_MODE_PLAY_FOLDER:
        case MP3_MODE_REPEAT_ALL:
            if (playlist_index == 0) playlist_index = playlist_count - 1; else playlist_index--;
            start_track_by_index(menu, playlist_index);
            return;
        case MP3_MODE_SHUFFLE:
            if (!shuffle_order) prepare_shuffle_order();
            if (shuffle_pos > 0) {
                shuffle_pos--;
                playlist_index = (size_t)shuffle_order[shuffle_pos];
                start_track_by_index(menu, playlist_index);
            }
            return;
        default:
            return;
    }
}


static void process (menu_t *menu) {
    mp3player_err_t err;

    err = mp3player_process();
    if (err != MP3PLAYER_OK) {
        menu_show_error(menu, convert_error_message(err));
        return;
    }

    /* START button mapped to menu->actions.settings - cycle playback mode */
    if (menu->actions.settings) {
        play_mode = (mp3_play_mode_t) ((play_mode + 1) % MP3_MODE_COUNT);
        sound_play_effect(SFX_SETTING);
        if (play_mode == MP3_MODE_SHUFFLE) {
            prepare_shuffle_order();
        }
    }

    if (menu->actions.back) {
        sound_play_effect(SFX_EXIT);
        menu->next_mode = MENU_MODE_BROWSER;
        return;
    } else if (menu->actions.enter) {
        err = mp3player_toggle();
        if (err != MP3PLAYER_OK) {
            menu_show_error(menu, convert_error_message(err));
        }
        sound_play_effect(SFX_ENTER);
        return;
    } else if (menu->actions.go_left || menu->actions.go_right) {
        int seconds = menu->actions.go_fast ? SEEK_SECONDS_FAST : SEEK_SECONDS;
        err = mp3player_seek(menu->actions.go_left ? (-seconds) : seconds);
        if (err != MP3PLAYER_OK) {
            menu_show_error(menu, convert_error_message(err));
        }
        return;
    }

    if (mp3player_is_finished()) {
        /* advance based on play mode */
        start_next_track(menu);
    }
}

static void draw (menu_t *menu, surface_t *d) {
    rdpq_attach(d, NULL);

    ui_components_background_draw();

    ui_components_layout_draw();

    ui_components_seekbar_draw(mp3player_get_progress());

    ui_components_main_text_draw(
        STL_DEFAULT,
        ALIGN_CENTER, VALIGN_TOP,
        "MUSIC PLAYER\n"
        "\n"
        "%s",
        menu->browser.entry->name
    );

    char formatted_track_elapsed_length[64];

    format_elapsed_duration(
        formatted_track_elapsed_length,
        mp3player_get_duration() * mp3player_get_progress(),
        mp3player_get_duration()
    );

    ui_components_main_text_draw(
        STL_DEFAULT,
        ALIGN_LEFT, VALIGN_TOP,
        "\n"
        "\n"
        "\n"
        "\n"
        " Track elapsed / length:\n"
        "  %s\n"
        "\n"
        " Average bitrate:\n"
        "  %.0f kbps\n"
        "\n"
        " Samplerate:\n"
        "  %d Hz",
        formatted_track_elapsed_length,
        mp3player_get_bitrate() / 1000,
        mp3player_get_samplerate()
    );

    ui_components_actions_bar_text_draw(
        STL_DEFAULT,
        ALIGN_LEFT, VALIGN_TOP,
        "A: %s\n"
        "B: Exit\n",
        mp3player_is_playing() ? "Pause" : mp3player_is_finished() ? "Play again" : "Play"
    );

    ui_components_actions_bar_text_draw(
        STL_DEFAULT,
        ALIGN_CENTER, VALIGN_TOP,
        "◀ Rewind | Fast forward ▶\n"
    );

    /* Show playback mode and how to change it (START cycles modes) */
    ui_components_actions_bar_text_draw(
        STL_DEFAULT,
        ALIGN_RIGHT, VALIGN_TOP,
        "Mode: %s\n"
        "START: Cycle mode",
        mode_to_string(play_mode)
    );

    rdpq_detach_show();
}

static void deinit (void) {
    sound_init_default();
    mp3player_deinit();
    free_playlist();
}


void view_music_player_init (menu_t *menu) {
    mp3player_err_t err;

    err = mp3player_init();
    if (err != MP3PLAYER_OK) {
        menu_show_error(menu, convert_error_message(err));
        mp3player_deinit();
        return;
    }

    /* Build playlist from the current directory listing */
    build_playlist_from_browser(menu);

    path_t *path = path_clone_push(menu->browser.directory, menu->browser.entry->name);

    err = mp3player_load(path_get(path));
    if (err != MP3PLAYER_OK) {
        menu_show_error(menu, convert_error_message(err));
        mp3player_deinit();
    } else {
        sound_init_mp3_playback();
        mp3player_mute(false);
        err = mp3player_play();
        if (err != MP3PLAYER_OK) {
            menu_show_error(menu, convert_error_message(err));
            mp3player_deinit();
        }
    }

    if (path) path_free(path);
}
