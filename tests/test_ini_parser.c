/**
 * @file test_ini_parser.c
 * @brief Host-side unit tests for INI parser behavior.
 *
 * These tests focus on parser semantics that can be validated without N64
 * runtime services: parsing, getters/setters, deletion, and save/load round-trip.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/menu/ini_parser.h"

#define TEST_INI_FILE ".tmp_ini_parser_test.ini"

/**
 * Ensure no leftover test file survives across runs.
 */
static void cleanup_tmp_file(void) {
    remove(TEST_INI_FILE);
}

/**
 * Validate parse_buffer with global keys, comments, sections, and trimming.
 */
static void test_parse_buffer_basics(void) {
    const char ini_text[] =
        "title = Main Menu\n"
        "; comment\n"
        "[audio]\n"
        "enabled = true\n"
        "volume = 42\n"
        "name =  Stereo  \n"
        "# another comment\n";

    ini_t *ini = ini_parse_buffer(ini_text, sizeof(ini_text) - 1);
    assert(ini != NULL);

    assert(strcmp(ini_get_string(ini, "", "title", "missing"), "Main Menu") == 0);
    assert(strcmp(ini_get_string(ini, "audio", "name", "missing"), "Stereo") == 0);
    assert(ini_get_bool(ini, "audio", "enabled", false) == true);
    assert(ini_get_int(ini, "audio", "volume", -1) == 42);

    ini_free(ini);
    printf("✓ test_parse_buffer_basics\n");
}

/**
 * Validate quoted parsing and escaping for quote and backslash characters.
 */
static void test_parse_quoted_values(void) {
    const char ini_text[] =
        "[paths]\n"
        "quoted = \"C:\\\\Games\\\\N64\"\n"
        "phrase = \"hello \\\"world\\\"\"\n";

    ini_t *ini = ini_parse_buffer(ini_text, sizeof(ini_text) - 1);
    assert(ini != NULL);

    assert(strcmp(ini_get_string(ini, "paths", "quoted", "missing"), "C:\\Games\\N64") == 0);
    assert(strcmp(ini_get_string(ini, "paths", "phrase", "missing"), "hello \"world\"") == 0);

    ini_free(ini);
    printf("✓ test_parse_quoted_values\n");
}

/**
 * Validate comment handling in quoted vs unquoted values.
 */
static void test_comment_semantics(void) {
    const char ini_text[] =
        "[values]\n"
        "trimmed = alpha ; trailing comment\n"
        "trimmed_hash = beta # trailing hash comment\n"
        "quoted = \"gamma ; not a comment # still value\"\n";

    ini_t *ini = ini_parse_buffer(ini_text, sizeof(ini_text) - 1);
    assert(ini != NULL);

    assert(strcmp(ini_get_string(ini, "values", "trimmed", "missing"), "alpha") == 0);
    assert(strcmp(ini_get_string(ini, "values", "trimmed_hash", "missing"), "beta") == 0);
    assert(strcmp(ini_get_string(ini, "values", "quoted", "missing"),
                  "gamma ; not a comment # still value") == 0);

    ini_free(ini);
    printf("✓ test_comment_semantics\n");
}

/**
 * Validate getters return defaults for invalid or missing values.
 */
static void test_getter_defaults_and_type_parsing(void) {
    const char ini_text[] =
        "[types]\n"
        "ok_int = 123\n"
        "bad_int = 12x\n"
        "truthy = on\n"
        "falsy = no\n"
        "bad_bool = maybe\n";

    ini_t *ini = ini_parse_buffer(ini_text, sizeof(ini_text) - 1);
    assert(ini != NULL);

    assert(ini_get_int(ini, "types", "ok_int", -1) == 123);
    assert(ini_get_int(ini, "types", "bad_int", 77) == 77);
    assert(ini_get_int(ini, "types", "missing", 33) == 33);

    assert(ini_get_bool(ini, "types", "truthy", false) == true);
    assert(ini_get_bool(ini, "types", "falsy", true) == false);
    assert(ini_get_bool(ini, "types", "bad_bool", true) == true);
    assert(ini_get_bool(ini, "types", "missing", false) == false);

    ini_free(ini);
    printf("✓ test_getter_defaults_and_type_parsing\n");
}

/**
 * Validate parse defaults for NULL/empty inputs and malformed section lines.
 */
static void test_parse_null_empty_and_malformed_lines(void) {
    ini_t *null_input = ini_parse_buffer(NULL, 0);
    assert(null_input != NULL);
    assert(ini_is_empty(null_input) == true);
    ini_free(null_input);

    const char empty_input[] = "";
    ini_t *empty = ini_parse_buffer(empty_input, 0);
    assert(empty != NULL);
    assert(ini_is_empty(empty) == true);
    ini_free(empty);

    const char malformed[] =
        "[valid]\n"
        "name = value\n"
        "[broken\n"
        "still = parsed\n";
    ini_t *parsed = ini_parse_buffer(malformed, sizeof(malformed) - 1);
    assert(parsed != NULL);
    assert(strcmp(ini_get_string(parsed, "valid", "name", "missing"), "value") == 0);
    assert(strcmp(ini_get_string(parsed, "valid", "still", "missing"), "parsed") == 0);
    ini_free(parsed);

    printf("✓ test_parse_null_empty_and_malformed_lines\n");
}

/**
 * Validate numeric parsing boundaries and out-of-range fallback behavior.
 */
static void test_int_overflow_defaults(void) {
    const char ini_text[] =
        "[limits]\n"
        "too_big = 999999999999999999999\n"
        "too_small = -999999999999999999999\n";

    ini_t *ini = ini_parse_buffer(ini_text, sizeof(ini_text) - 1);
    assert(ini != NULL);

    assert(ini_get_int(ini, "limits", "too_big", 11) == 11);
    assert(ini_get_int(ini, "limits", "too_small", 22) == 22);

    ini_free(ini);
    printf("✓ test_int_overflow_defaults\n");
}

/**
 * Validate set/get/delete semantics and empty-state tracking.
 */
static void test_set_delete_and_empty_state(void) {
    ini_t *ini = ini_create();
    assert(ini != NULL);
    assert(ini_is_empty(ini) == true);

    ini_set_string(ini, "menu", "title", "Flashcart Menu");
    ini_set_int(ini, "menu", "timeout", 5);
    ini_set_bool(ini, "menu", "autosave", true);

    assert(ini_is_empty(ini) == false);
    assert(strcmp(ini_get_string(ini, "menu", "title", "missing"), "Flashcart Menu") == 0);
    assert(ini_get_int(ini, "menu", "timeout", -1) == 5);
    assert(ini_get_bool(ini, "menu", "autosave", false) == true);

    ini_delete_key(ini, "menu", "title");
    ini_delete_key(ini, "menu", "timeout");
    ini_delete_key(ini, "menu", "autosave");

    assert(strcmp(ini_get_string(ini, "menu", "title", "default"), "default") == 0);
    assert(ini_is_empty(ini) == true);

    ini_free(ini);
    printf("✓ test_set_delete_and_empty_state\n");
}

/**
 * Validate save/load round-trip including quoting requirements.
 */
static void test_save_load_round_trip(void) {
    ini_t *ini = ini_create();
    assert(ini != NULL);

    ini_set_string(ini, "", "global_name", "N64 Menu");
    ini_set_string(ini, "video", "profile", "CRT #1");
    ini_set_string(ini, "video", "notes", "needs \"scanlines\"");

    cleanup_tmp_file();
    assert(ini_save(ini, TEST_INI_FILE) == true);
    ini_free(ini);

    ini_t *loaded = ini_load(TEST_INI_FILE);
    assert(loaded != NULL);

    assert(strcmp(ini_get_string(loaded, "", "global_name", "missing"), "N64 Menu") == 0);
    assert(strcmp(ini_get_string(loaded, "video", "profile", "missing"), "CRT #1") == 0);
    assert(strcmp(ini_get_string(loaded, "video", "notes", "missing"), "needs \"scanlines\"") == 0);

    ini_free(loaded);
    cleanup_tmp_file();
    printf("✓ test_save_load_round_trip\n");
}

/**
 * Validate error-tolerant load path that always returns a usable object.
 */
static void test_try_load_missing_file(void) {
    cleanup_tmp_file();

    ini_t *ini = ini_try_load(TEST_INI_FILE);
    assert(ini != NULL);
    assert(ini_is_empty(ini) == true);

    ini_free(ini);
    printf("✓ test_try_load_missing_file\n");
}

/**
 * Validate that ini_load(NULL) is safe and returns an empty object.
 */
static void test_load_null_path_returns_empty(void) {
    ini_t *ini = ini_load(NULL);
    assert(ini != NULL);
    assert(ini_is_empty(ini) == true);

    ini_free(ini);
    printf("✓ test_load_null_path_returns_empty\n");
}

int main(void) {
    printf("Running INI parser unit tests...\n\n");

    test_parse_buffer_basics();
    test_parse_quoted_values();
    test_comment_semantics();
    test_getter_defaults_and_type_parsing();
    test_parse_null_empty_and_malformed_lines();
    test_int_overflow_defaults();
    test_set_delete_and_empty_state();
    test_save_load_round_trip();
    test_try_load_missing_file();
    test_load_null_path_returns_empty();

    printf("\n✓ All INI parser tests passed!\n");
    return 0;
}
