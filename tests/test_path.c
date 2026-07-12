/**
 * @file test_path.c
 * @brief Host-side unit tests for menu path helpers.
 *
 * Build and run within the devcontainer:
 *   docker run --rm -v "${PWD}:/workspaces/N64FlashcartMenu" -w /workspaces/N64FlashcartMenu/tests \
 *     n64flashcartmenu-sc64deployer bash -lc "make -B test"
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/menu/path.h"

/**
 * Ensure root normalization is stable regardless of trailing slash in base path.
 */
static void test_path_init_and_root(void) {
    path_t *a = path_init("rom:", "menu");
    path_t *b = path_init("rom:/", "menu");

    assert(strcmp(path_get(a), "rom:/menu") == 0);
    assert(strcmp(path_get(b), "rom:/menu") == 0);
    assert(path_is_root(a) == false);

    path_free(a);
    path_free(b);
    printf("✓ test_path_init_and_root\n");
}

/**
 * Validate push/pop navigation and root underflow protection.
 */
static void test_path_push_pop(void) {
    path_t *p = path_init("rom:/", "menu");
    path_push(p, "config");
    assert(strcmp(path_get(p), "rom:/menu/config") == 0);

    path_pop(p);
    assert(strcmp(path_get(p), "rom:/menu") == 0);

    path_pop(p);
    assert(path_is_root(p) == true);
    assert(strcmp(path_get(p), "rom:/") == 0);

    /* Popping root should be a no-op. */
    path_pop(p);
    assert(strcmp(path_get(p), "rom:/") == 0);

    path_free(p);
    printf("✓ test_path_push_pop\n");
}

/**
 * Validate cloning behavior and path equality checks.
 */
static void test_path_clone_and_match(void) {
    path_t *p = path_init("rom:/", "menu");
    path_push(p, "config.ini");

    path_t *clone = path_clone(p);
    assert(path_are_match(p, clone) == true);

    path_t *clone_push = path_clone_push(p, "backup");
    assert(path_are_match(p, clone_push) == false);

    path_free(p);
    path_free(clone);
    path_free(clone_push);
    printf("✓ test_path_clone_and_match\n");
}

/**
 * Validate subdir insertion between parent path and filename leaf.
 */
static void test_path_push_subdir(void) {
    path_t *p = path_init("rom:/", "menu/file.txt");
    path_push_subdir(p, "archived");

    assert(strcmp(path_get(p), "rom:/menu/archived/file.txt") == 0);

    path_free(p);
    printf("✓ test_path_push_subdir\n");
}

/**
 * Validate extension getter, remover, and replacement operations.
 */
static void test_path_extensions(void) {
    path_t *p = path_init("rom:/", "menu/file.txt");

    assert(strcmp(path_ext_get(p), "txt") == 0);

    path_ext_remove(p);
    assert(path_ext_get(p) == NULL);
    assert(strcmp(path_get(p), "rom:/menu/file") == 0);

    path_ext_replace(p, "cfg");
    assert(strcmp(path_ext_get(p), "cfg") == 0);
    assert(strcmp(path_get(p), "rom:/menu/file.cfg") == 0);

    path_free(p);
    printf("✓ test_path_extensions\n");
}

/**
 * Validate truthiness checks for empty, populated, and NULL paths.
 */
static void test_path_has_value(void) {
    path_t *empty = path_create(NULL);
    path_t *value = path_create("rom:/menu");

    assert(path_has_value(empty) == false);
    assert(path_has_value(value) == true);
    assert(path_has_value(NULL) == false);

    path_free(empty);
    path_free(value);
    printf("✓ test_path_has_value\n");
}

/**
 * Validate last-path extraction and leading-slash normalization in path_push.
 */
static void test_path_last_get_and_leading_slash_push(void) {
    path_t *p = path_init("rom:/", "menu");
    path_push(p, "/games");
    path_push(p, "zelda.rom");

    assert(strcmp(path_get(p), "rom:/menu/games/zelda.rom") == 0);
    assert(strcmp(path_last_get(p), "zelda.rom") == 0);

    path_free(p);
    printf("✓ test_path_last_get_and_leading_slash_push\n");
}

/**
 * Validate NULL-aware equality behavior in path_are_match.
 */
static void test_path_are_match_with_null_and_empty(void) {
    path_t *empty = path_create(NULL);
    path_t *filled = path_create("rom:/menu");

    assert(path_are_match(NULL, NULL) == true);
    assert(path_are_match(NULL, empty) == true);
    assert(path_are_match(empty, NULL) == true);
    assert(path_are_match(empty, filled) == false);
    assert(path_are_match(filled, empty) == false);

    path_free(empty);
    path_free(filled);
    printf("✓ test_path_are_match_with_null_and_empty\n");
}

/**
 * Run all path tests.
 */
int main(void) {
    printf("Running menu path unit tests...\n\n");

    test_path_init_and_root();
    test_path_push_pop();
    test_path_clone_and_match();
    test_path_push_subdir();
    test_path_extensions();
    test_path_has_value();
    test_path_last_get_and_leading_slash_push();
    test_path_are_match_with_null_and_empty();

    printf("\n✓ All path tests passed!\n");
    return 0;
}
