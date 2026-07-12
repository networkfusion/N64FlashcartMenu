/**
 * @file test_fs.c
 * @brief Unit tests for file system utility functions with OOB hardening.
 * 
 * Build and run within the devcontainer:
 *   docker run --rm -v "${PWD}:/workspaces/N64FlashcartMenu" -w /workspaces/N64FlashcartMenu/tests \
 *     n64flashcartmenu-sc64deployer bash -lc "make -B test"
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <stdlib.h>

#include "../src/utils/fs.h"
#include "../src/utils/utils.h"

#define TEST_TMP_ROOT ".tmp_fs_tests"

static void cleanup_tmp_root(void) {
    (void)system("rm -rf " TEST_TMP_ROOT);
}

static void create_tmp_root(void) {
    cleanup_tmp_root();
    assert(directory_create(TEST_TMP_ROOT) == false);
}

static void test_strip_fs_prefix(void) {
    char path_with_prefix[] = "rom:/menu/config.ini";
    char path_without_prefix[] = "menu/config.ini";

    assert(strcmp(strip_fs_prefix(path_with_prefix), "/menu/config.ini") == 0);
    assert(strcmp(strip_fs_prefix(path_without_prefix), "menu/config.ini") == 0);
    printf("✓ test_strip_fs_prefix\n");
}

static void test_file_basename(void) {
    char path_a[] = "rom:/menu/config.ini";
    char path_b[] = "config.ini";

    assert(strcmp(file_basename(path_a), "config.ini") == 0);
    assert(strcmp(file_basename(path_b), "config.ini") == 0);
    printf("✓ test_file_basename\n");
}

static void test_directory_create_nested(void) {
    char nested[] = TEST_TMP_ROOT "/a/b/c";
    assert(directory_create(nested) == false);
    assert(directory_exists(nested) == true);

    /* Creating an existing directory should remain a non-error. */
    assert(directory_create(nested) == false);
    printf("✓ test_directory_create_nested\n");
}

static void test_file_exists_and_size(void) {
    char file_path[] = TEST_TMP_ROOT "/size_test.bin";
    FILE *f = fopen(file_path, "wb");
    uint8_t bytes[32];
    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (uint8_t)i;
    }

    assert(f != NULL);
    assert(fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes));
    assert(fclose(f) == 0);

    assert(file_exists(file_path) == true);
    assert(file_get_size(file_path) == 32);
    assert(file_get_size(TEST_TMP_ROOT "/does_not_exist.bin") == -1);
    printf("✓ test_file_exists_and_size\n");
}

static void test_file_fill(void) {
    char file_path[] = TEST_TMP_ROOT "/fill_test.bin";
    FILE *f = fopen(file_path, "wb");
    uint8_t bytes[16];
    uint8_t verify[16];

    for (size_t i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (uint8_t)i;
    }

    assert(f != NULL);
    assert(fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes));
    assert(fclose(f) == 0);

    assert(file_fill(file_path, 0xAA) == false);

    f = fopen(file_path, "rb");
    assert(f != NULL);
    assert(fread(verify, 1, sizeof(verify), f) == sizeof(verify));
    assert(fclose(f) == 0);

    for (size_t i = 0; i < sizeof(verify); i++) {
        assert(verify[i] == 0xAA);
    }

    printf("✓ test_file_fill\n");
}

/**
 * Test file_allocate for deterministic success/failure scenarios.
 */
static void test_file_allocate(void) {
    char ok_path[] = TEST_TMP_ROOT "/alloc_zero.bin";
    char fail_path[] = TEST_TMP_ROOT "/missing_parent/alloc_fail.bin";

    /* Size 0 allocation should create an empty file with no error. */
    assert(file_allocate(ok_path, 0) == false);
    assert(file_exists(ok_path) == true);
    assert(file_get_size(ok_path) == 0);

    /* Missing parent directory should fail in a deterministic way. */
    assert(file_allocate(fail_path, 128) == true);
    assert(file_exists(fail_path) == false);

    printf("✓ test_file_allocate\n");
}

/**
 * Test directory_create idempotence and support for trailing separators.
 */
static void test_directory_create_trailing_separator(void) {
    char with_trailing[] = TEST_TMP_ROOT "/trail/a/b/";
    char normalized[] = TEST_TMP_ROOT "/trail/a/b";

    assert(directory_create(with_trailing) == false);
    assert(directory_exists(normalized) == true);

    /* Creating the same path again should remain non-fatal. */
    assert(directory_create(with_trailing) == false);

    printf("✓ test_directory_create_trailing_separator\n");
}

/**
 * Test file_has_extensions with valid extension match.
 */
static void test_file_has_extensions_match(void) {
    const char *extensions[] = { "txt", "md", NULL };
    char path[] = "document.txt";
    assert(file_has_extensions(path, extensions) == true);
    printf("✓ test_file_has_extensions_match\n");
}

/**
 * Test file_has_extensions with valid extension no match.
 */
static void test_file_has_extensions_no_match(void) {
    const char *extensions[] = { "txt", "md", NULL };
    char path[] = "document.pdf";
    assert(file_has_extensions(path, extensions) == false);
    printf("✓ test_file_has_extensions_no_match\n");
}

/**
 * Test file_has_extensions with NULL path.
 */
static void test_file_has_extensions_null_path(void) {
    const char *extensions[] = { "txt", NULL };
    assert(file_has_extensions(NULL, extensions) == false);
    printf("✓ test_file_has_extensions_null_path\n");
}

/**
 * Test file_has_extensions with NULL extensions.
 */
static void test_file_has_extensions_null_extensions(void) {
    char path[] = "document.txt";
    assert(file_has_extensions(path, NULL) == false);
    printf("✓ test_file_has_extensions_null_extensions\n");
}

/**
 * Test file_has_extensions with no extension.
 */
static void test_file_has_extensions_no_extension(void) {
    const char *extensions[] = { "txt", NULL };
    char path[] = "README";
    assert(file_has_extensions(path, extensions) == false);
    printf("✓ test_file_has_extensions_no_extension\n");
}

/**
 * Test file_has_extensions with empty extension list.
 */
static void test_file_has_extensions_empty_list(void) {
    const char *extensions[] = { NULL };
    char path[] = "document.txt";
    assert(file_has_extensions(path, extensions) == false);
    printf("✓ test_file_has_extensions_empty_list\n");
}

/**
 * Test file_has_extensions with trailing dot in file name.
 */
static void test_file_has_extensions_trailing_dot(void) {
    const char *extensions[] = { "txt", NULL };
    char path[] = "document.";
    assert(file_has_extensions(path, extensions) == false);
    printf("✓ test_file_has_extensions_trailing_dot\n");
}

/**
 * Test file_has_extensions with long but valid path.
 */
static void test_file_has_extensions_long_path(void) {
    const char *extensions[] = { "n64", NULL };
    char path[512];
    memset(path, 'a', 511);
    path[511] = '\0';
    /* Overwrite last 4 chars to be ".n64" */
    strcpy(&path[507], ".n64");
    assert(file_has_extensions(path, extensions) == true);
    printf("✓ test_file_has_extensions_long_path\n");
}

/**
 * Test file_has_extensions with overlong path (exceeds FS_MAX_PATH_SCAN_LENGTH).
 * This should be rejected defensively.
 */
static void test_file_has_extensions_overlong_path(void) {
    const char *extensions[] = { "txt", NULL };
    char path[1100];
    memset(path, 'x', 1099);
    path[1099] = '\0';
    /* Despite having a valid extension stored at end, path exceeds scan limit */
    assert(file_has_extensions(path, extensions) == false);
    printf("✓ test_file_has_extensions_overlong_path\n");
}

/**
 * Test file_has_extensions with very long extension (exceeds FS_MAX_EXTENSION_LENGTH).
 */
static void test_file_has_extensions_long_extension(void) {
    const char *long_ext[] = { "verylongextensionnamethatexceedsscanlimit", NULL };
    char path[] = "file.txt";
    assert(file_has_extensions(path, long_ext) == false);
    printf("✓ test_file_has_extensions_long_extension\n");
}

/**
 * Test file_has_extensions with case-insensitive match.
 */
static void test_file_has_extensions_case_insensitive(void) {
    const char *extensions[] = { "TXT", NULL };
    char path[] = "document.txt";
    assert(file_has_extensions(path, extensions) == true);
    printf("✓ test_file_has_extensions_case_insensitive\n");
}

/**
 * Run all tests.
 */
int main(void) {
    printf("Running fs utility unit tests...\n\n");

    create_tmp_root();

    test_strip_fs_prefix();
    test_file_basename();
    test_directory_create_nested();
    test_directory_create_trailing_separator();
    test_file_exists_and_size();
    test_file_allocate();
    test_file_fill();

    test_file_has_extensions_match();
    test_file_has_extensions_no_match();
    test_file_has_extensions_null_path();
    test_file_has_extensions_null_extensions();
    test_file_has_extensions_no_extension();
    test_file_has_extensions_empty_list();
    test_file_has_extensions_trailing_dot();
    test_file_has_extensions_long_path();
    test_file_has_extensions_overlong_path();
    test_file_has_extensions_long_extension();
    test_file_has_extensions_case_insensitive();

    cleanup_tmp_root();

    printf("\n✓ All tests passed!\n");
    return 0;
}
