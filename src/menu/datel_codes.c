#include <ctype.h>
#include "datel_codes.h"
#include <string.h>
#include <libdragon.h> // only included for debugf
#include "utils/fs.h"
#include "utils/utils.h"


#define MAX_FILE_SIZE KiB(128)

/** @brief Text file structure */
typedef struct {
    char *contents; /**< File contents */
    size_t length; /**< File length */
} cheat_file_t;

static cheat_file_t *cheat_file_text;

cheat_file_code_t cheat_codes[MAX_CHEAT_CODES]; // Global array to hold cheat codes

/**
 * @brief Generate a cheats array containing enabled cheats as address/value pairs.
 *        The last two entries will always be zero.
 * 
 * @param cheats_in Input array of type cheat_file_code_t.
 * @param cheats_out Output array (must be at least (MAX_CHEAT_CODES * 2 + 2) in size).
 * @return Number of address/value pairs written (including the trailing zeros).
 */
size_t generate_enabled_cheats_array(cheat_file_code_t *cheats_in, uint32_t *cheats_out) {
    size_t count = 0;
    for (int i = 0; i < MAX_CHEAT_CODES; ++i) {
        if (cheats_in[i].enabled) {
            cheats_out[count++] = cheats_in[i].address;
            cheats_out[count++] = (uint32_t)cheats_in[i].value;
        }
    }
    // Ensure the last two entries are zero
    cheats_out[count++] = 0;
    cheats_out[count++] = 0;

    return count;
}

/** * @brief Get the cheat codes.
 * 
 * @return Pointer to the array of cheat codes.
 */
cheat_file_code_t *get_cheat_codes(void) {
    return cheat_codes;
}

/** * @brief Set the cheat codes.
 * 
 * @param cheats Pointer to the array of cheat codes.
 */
void set_cheat_codes(cheat_file_code_t *cheats) {
    if (cheats) {
        memcpy(cheat_codes, cheats, sizeof(cheat_file_code_t) * MAX_CHEAT_CODES);
    } else {
        memset(cheat_codes, 0, sizeof(cheat_file_code_t) * MAX_CHEAT_CODES);
    }
}

/** * @brief Populate a cheat code with a description.
 * 
 * @param code Pointer to the cheat code structure.
 * @param description The description string to set.
 */
void populate_cheat_code_description(cheat_file_code_t *code, const char *description) {
    if (code && description) {
        strncpy(code->description, description, sizeof(code->description) - 1);
        code->description[sizeof(code->description) - 1] = '\0'; // Ensure null-termination
    }
}

/** * @brief Parse a cheat code string and populate the cheat code structure.
 * 
 * @param code Pointer to the cheat code structure to populate.
 * @param code_str The cheat code string in the format "address value description".
 */
void parse_cheat_code_string(cheat_file_code_t *code, const char *code_str) {
    if (code && code_str) {
        unsigned int addr = 0, val = 0;
        char description[32] = {0};
        int parsed = sscanf(code_str, " %x %x %31[^\n]", &addr, &val, description);
        if (parsed == 3) {
            code->address = (uint32_t)addr;
            code->value   = (uint32_t)val;
            code->enabled = true;
            populate_cheat_code_description(code, description);
        } else if (parsed == 2) {
            code->address = (uint32_t)addr;
            code->value   = (uint32_t)val;
            code->enabled = true;
            code->description[0] = '\0';
        } else {
            debugf("Failed to parse cheat code string: %s\n", code_str);
            code->address = 0;
            code->value   = 0;
            code->enabled = false;
            code->description[0] = '\0'; // Clear description
        }
    } else {
        debugf("Invalid cheat code or code string provided.\n");
        if (code) {
            code->address = 0;
            code->value   = 0;
            code->enabled = false;
            code->description[0] = '\0'; // Clear description
        }
    }
}

/**
 * @brief Deinitialize the cheat file.
 */
static void deinit_cheat_file (void) {
    if (cheat_file_text) {
        if (cheat_file_text->contents) {
            free(cheat_file_text->contents);
        }
        free(cheat_file_text);
        cheat_file_text = NULL;
    }
}

cheat_file_load_err_t open_cheat_file(char *path) {
    if ((cheat_file_text = calloc(1, sizeof(cheat_file_t))) == NULL) {
        return CHEAT_FILE_LOAD_ERR_MEMORY_ALLOC_FAIL;
    }

    file_read_text_err_t read_error = FILE_READ_TEXT_OK;
    if (!file_try_read_text_ex(path, MAX_FILE_SIZE, &cheat_file_text->contents, &cheat_file_text->length, &read_error)) {
        cheat_file_load_err_t mapped_error = CHEAT_FILE_LOAD_ERR_UNKNOWN;
        switch (read_error) {
            case FILE_READ_TEXT_ERR_OPEN:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_OPEN_FAIL;
                break;
            case FILE_READ_TEXT_ERR_SEEK:
            case FILE_READ_TEXT_ERR_SIZE:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_STAT_FAIL;
                break;
            case FILE_READ_TEXT_ERR_EMPTY:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_EMPTY;
                break;
            case FILE_READ_TEXT_ERR_TOO_BIG:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_TOO_BIG;
                break;
            case FILE_READ_TEXT_ERR_ALLOC:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_CONTENTS_ALLOC_FAIL;
                break;
            case FILE_READ_TEXT_ERR_READ:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_READ_FAIL;
                break;
            case FILE_READ_TEXT_ERR_CLOSE:
                mapped_error = CHEAT_FILE_LOAD_ERR_FILE_CLOSE_FAIL;
                break;
            case FILE_READ_TEXT_ERR_INVALID_ARGS:
            case FILE_READ_TEXT_OK:
            default:
                mapped_error = CHEAT_FILE_LOAD_ERR_UNKNOWN;
                break;
        }
        deinit_cheat_file();
        return mapped_error;
    }

    return CHEAT_FILE_LOAD_OK;
}

void cheat_file_open_res_debug(cheat_file_load_err_t res) {
    switch (res) {
        case CHEAT_FILE_LOAD_OK:
            debugf("Cheat file loaded successfully.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_MEMORY_ALLOC_FAIL:
            debugf("Cheat file load error: Memory allocation failed.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_OPEN_FAIL:
            debugf("Cheat file load error: File open failed.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_STAT_FAIL:
            debugf("Cheat file load error: File stat failed.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_EMPTY:
            debugf("Cheat file load error: File is empty.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_TOO_BIG:
            debugf("Cheat file load error: File is too big.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_CONTENTS_ALLOC_FAIL:
            debugf("Cheat file load error: File contents allocation failed.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_READ_FAIL:
            debugf("Cheat file load error: File read failed.\n");
            break;
        case CHEAT_FILE_LOAD_ERR_FILE_CLOSE_FAIL:
            debugf("Cheat file load error: File close failed.\n");
            break;
        default:
            debugf("Cheat file load error: Unknown error.\n");
    }
}

void load_cheats_from_file(char *path) {

    debugf("Cheat Editor: Loading cheats from path %s.\n", path);

    set_cheat_codes(NULL);
    cheat_file_load_err_t res_file_open = open_cheat_file(path);

    if (res_file_open == CHEAT_FILE_LOAD_OK) {
        debugf("Cheat Editor: Cheat file loaded successfully.\n");

        // Parse each line in the file as a cheat code string
        int code_count = 0;
        char *line = cheat_file_text->contents;
        char *saveptr = NULL;
        char *token = strtok_r(line, "\n", &saveptr);

        while (token && code_count < MAX_CHEAT_CODES) {
            // Trim leading whitespace
            char *p = token;
            while (*p && isspace((unsigned char)*p)) p++;
            bool disabled = false;
            // ": " prefix means disabled code; still parse the code after the marker
            if (*p == ':') {
                disabled = true;
                p++;
                while (*p && isspace((unsigned char)*p)) p++;
            }
            // Skip empty or comment-only lines (#, ;, or //)
            if (*p && !(p[0] == '#' || p[0] == ';' || (p[0] == '/' && p[1] == '/'))) {
                parse_cheat_code_string(&cheat_codes[code_count], p);
                bool parsed_ok = (cheat_codes[code_count].address != 0)
                              || (cheat_codes[code_count].value   != 0);
                if (parsed_ok) {
                    if (disabled) 
                        cheat_codes[code_count].enabled = false;
                    code_count++;
                }
            }
            token = strtok_r(NULL, "\n", &saveptr);
        }

        // Zero out any remaining cheat codes
        for (int i = code_count; i < MAX_CHEAT_CODES; ++i) {
            memset(&cheat_codes[i], 0, sizeof(cheat_file_code_t));
        }

        set_cheat_codes(cheat_codes);

        deinit_cheat_file();

        #if DEBUG
        // --DEBUG CODE
        uint32_t cheats[MAX_CHEAT_CODE_ARRAYLIST_SIZE];
        size_t cheat_item_count = generate_enabled_cheats_array(get_cheat_codes(), cheats);
        debugf("Cheat Editor: Parsed and generated %u cheat items from file.\n", cheat_item_count);
        // --END DEBUG CODE
        #endif

    }
    else {
        cheat_file_open_res_debug(res_file_open);
        set_cheat_codes(NULL);
        deinit_cheat_file();
    }
}

void save_cheats_to_file(char *path) {

    FILE *f = fopen(path, "w");
    if (!f) {
        debugf("Cheat Editor: Failed to open file for writing: %s\n", path);
        return;
    }

    for (int i = 0; i < MAX_CHEAT_CODES; ++i) {
        cheat_file_code_t *code = &cheat_codes[i];
        if (code->address != 0) { //code->enabled &&
            if (!code->enabled) {
                fprintf(f, ": ");
            } 
            if (code->description[0] != '\0') {
                fprintf(f, "%08lX %04X %s\n", code->address, code->value, code->description);
            } else {
                fprintf(f, "%08lX %04X\n", code->address, code->value);
            }
        }
    }
    

    if (fclose(f) != 0) {
        debugf("Cheat Editor: Failed to close file: %s\n", path);
        return;
    }
    debugf("Cheat Editor: Cheats saved to %s.\n", path);

}
