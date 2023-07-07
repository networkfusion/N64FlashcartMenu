#ifndef FLASHCART_ED64_INTERNAL_H__
#define FLASHCART_ED64_INTERNAL_H__


#include <stddef.h>
#include <stdint.h>


typedef struct {
    uint8_t BUFFER[8192];
    uint8_t EEPROM[2048];
    uint8_t DD_SECTOR[256];
    uint8_t FLASHRAM[128];
} ed64_buffers_t;

#define ED64_BUFFERS_BASE       (0x1FFE0000UL)
#define ED64_BUFFERS            ((ed64_buffers_t *) ED64_BUFFERS_BASE)


typedef enum {
    ED64_OK,
    ED64_ERROR_BAD_ARGUMENT,
    ED64_ERROR_BAD_ADDRESS,
    ED64_ERROR_BAD_CONFIG_ID,
    ED64_ERROR_TIMEOUT,
    ED64_ERROR_SD_CARD,
    ED64_ERROR_UNKNOWN_CMD = -1
} ed64_error_t;

typedef enum {
    CFG_BOOTLOADER_SWITCH,
    CFG_ROM_WRITE_ENABLE,
    CFG_ROM_SHADOW_ENABLE,
    CFG_DD_MODE,
    CFG_ISV_ENABLE,
    CFG_BOOT_MODE,
    CFG_SAVE_TYPE,
    CFG_CIC_SEED,
    CFG_TV_TYPE,
    CFG_DD_SD_ENABLE,
    CFG_DD_DRIVE_TYPE,
    CFG_DD_DISK_STATE,
    CFG_BUTTON_STATE,
    CFG_BUTTON_MODE,
    CFG_ROM_EXTENDED_ENABLE,
} ed64_cfg_t;

typedef enum {
    SAVE_TYPE_NONE,
    SAVE_TYPE_EEPROM_4K,
    SAVE_TYPE_EEPROM_16K,
    SAVE_TYPE_SRAM,
    SAVE_TYPE_FLASHRAM,
    SAVE_TYPE_SRAM_BANKED,
} ed64_save_type_t;


void ed64_unlock (void);
void ed64_lock (void);
bool ed64_check_presence (void);
void ed64_read_data (void *src, void *dst, size_t length);
void ed64_write_data (void *src, void *dst, size_t length);
ed64_error_t ed64_get_version (uint16_t *major, uint16_t *minor);
ed64_error_t ed64_get_config (ed64_cfg_t cfg, void *value);
ed64_error_t ed64_set_config (ed64_cfg_t cfg, uint32_t value);
ed64_error_t ed64_writeback_enable (void *address);
ed64_error_t ed64_flash_program (void *address, size_t length);
ed64_error_t ed64_flash_wait_busy (void);
ed64_error_t ed64_flash_get_erase_block_size (size_t *erase_block_size);
ed64_error_t ed64_flash_erase_block (void *address);


#endif
