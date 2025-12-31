#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <fatfs/ff.h>
#include <libdragon.h>
//#include <libcart/cart.h>

#include "utils/fs.h"
#include "utils/utils.h"

#include "../flashcart_utils.h"
#include "ed64_vseries_ll.h"
#include "ed64_vseries.h"
#include "ed64_vseries_state.h"

typedef enum {
    ED64_V1_0 = 110,
    ED64_V2_0 = 320,
    ED64_V2_5 = 325,
    ED64_V3_0 = 330,
} ed64_vseries_device_variant_t;

/* ED64 save location base address  */
#define SRAM_FLASHRAM_ADDRESS    (0xA8000000)
/* ED64 ROM location base address  */
#define ROM_ADDRESS              (0xB0000000)
#define EEPROM_ADDRESS           (0x1FFE2000)


//#define SAV_CONFIG_DATA_DEFAULT_SIZE    (8192) // 8KB default config data size to store the last used ROM + expecting save writeback + save type

static ed64_vseries_pseudo_writeback_t current_state;
static ed64_vseries_save_type_t current_save_type = SAVE_TYPE_NONE;

//extern int ed_exit (void);

static ed64_vseries_device_variant_t get_cart_model() {
    ed64_vseries_device_variant_t variant;
    uint16_t cpld_version;
    ed64_vseries_ll_get_cpld_version(&cpld_version);

    if ((cpld_version & 0xF000) == CPLD_VERSION_3_0) {
        return ED64_V3_0;
    } else if ((cpld_version & 0xF000) == CPLD_VERSION_2_5) {
        return ED64_V2_5;
    } else if ((cpld_version & 0xF000) == CPLD_VERSION_2_0) {
        return ED64_V2_0;
    } else {
        return ED64_V1_0;
    }
    return variant;
}

static flashcart_firmware_version_t ed64_vseries_get_firmware_version (void) {
    flashcart_firmware_version_t version_info;
    uint16_t cpld_version;
    ed64_vseries_ll_get_cpld_version(&cpld_version);

    if ((cpld_version & 0xF000) == CPLD_VERSION_3_0) {
        version_info.major = 3;
        version_info.minor = 0;
    } else if ((cpld_version & 0xF000) == CPLD_VERSION_2_5) {
        version_info.major = 2;
        version_info.minor = 5;
    } else if ((cpld_version & 0xF000) == CPLD_VERSION_2_0) {
        version_info.major = 2;
        version_info.minor = 0;
    } else {
        version_info.major = 0;
    }

    uint16_t fpga_version;
    ed64_vseries_ll_get_fpga_version(&fpga_version);
    version_info.revision = fpga_version;

    return version_info;
}

static flashcart_err_t ed64_vseries_pseudo_save_writeback(void) {
    if (current_state.is_expecting_save_writeback) {
        ed64_vseries_ll_set_save_type((ed64_vseries_save_type_t) current_state.last_save_type, false);
        
        // Now save the content back to the SD card!
        // Using the address and size based on save type.
        // based on the flashcart address map.
        //void *address = NULL;
        
        // FIL fil;
        // UINT bw;

        switch ((ed64_vseries_save_type_t) current_state.last_save_type) {
            case SAVE_TYPE_EEPROM_4KBIT:
                // int result4k = eepfs_init((void *) (EEPROM_ADDRESS), 1);
                // eepfs_read((void *) (EEPROM_ADDRESS), strip_fs_prefix(current_state.last_save_path), 64);
                break;
            case SAVE_TYPE_EEPROM_16KBIT:
                // int result16k = eepfs_init((void *) (EEPROM_ADDRESS), 1);
                // eepfs_read((void *) (EEPROM_ADDRESS), strip_fs_prefix(current_state.last_save_path), 256);
                break;
            case SAVE_TYPE_SRAM_256KBIT:
            case SAVE_TYPE_FLASHRAM_1MBIT:
            case SAVE_TYPE_SRAM_BANKED:
            case SAVE_TYPE_SRAM_1MBIT:
                //address = (void *) (SRAM_FLASHRAM_ADDRESS);
                break;
            case SAVE_TYPE_NONE:
            default:
                return FLASHCART_ERR_ARGS;
        }

        // if (f_open(&fil, strip_fs_prefix(current_state.last_save_path), FA_WRITE) != FR_OK) {
        //     return FLASHCART_ERR_LOAD;
        // }
        // size_t save_size = f_size(&fil);
        

        // FRESULT res = f_write(&fil, (void *) address, save_size, &bw);
        // if (res != FR_OK) {
        //     f_close(&fil);
        //     return FLASHCART_ERR_LOAD;
        // }

        // if (f_close(&fil) != FR_OK) {
        //     return FLASHCART_ERR_LOAD;
        // }

    }

    // make sure next boot doesnt trigger the check changing its state.
    current_state.is_expecting_save_writeback = false;
    //current_state.last_save_path = "";
    //current_state.last_save_type = SAVE_TYPE_NONE;
    ed64_vseries_state_save(&current_state);

    ed64_vseries_ll_set_save_type(SAVE_TYPE_NONE, false);

    return FLASHCART_OK;
}

static flashcart_err_t ed64_vseries_firmware_update_check_apply(void) {

    uint16_t cpld_version;
    uint16_t fpga_version;
    ed64_vseries_ll_get_fpga_version(&fpga_version);
    ed64_vseries_ll_get_cpld_version(&cpld_version);

    // Update firmware if needed (770 - 773 for v3)
    bool update_v3_available = fpga_version >= 0x300 && fpga_version <= 0x303;

    // 0x232 default v2.50
    bool update_v2_5_available = fpga_version >= 0x232 && fpga_version <= 0x233;

    // 0x214 == firm v2.20
    bool update_v2_available = fpga_version == 0x214;

    if ((cpld_version & 0xF000) == 0x3000 && update_v3_available) {
        debugf("ED64 V3 detected, updating firmware...\n");
        // char *firmware_path = "ed64-v3-fpga.rbf";

        // if (file_exists(firmware_path)) {
        //     FILE *fp = fopen(firmware_path, "rb");
        //     size_t file_size = ftell(fp);
        //     uint8_t *firmware_data = malloc( file_size );
        //     fread( firmware_data, 1, file_size, fp );
        //     fclose( fp );
        //     if (firmware_data) {
        //         debugf("Firmware file found, applying update...\n");
        //         ed64_vseries_ll_update_firmware(firmware_data);
        //         free(firmware_data);
        //         // ed_init();
        //     } else {
        //         debugf("Failed to load firmware file from %s\n", firmware_path);
        //     }
        // }
        // int32_t fpf = dfs_open(firmware_path);
        // uint8_t *firmware = malloc(dfs_size(fpf));
        // dfs_read(firmware, 1, dfs_size(fpf), fpf);
        // dfs_close(fpf);
        // //ed64_vseries_ll_update_firmware(firmware);
        // free(firmware);
        // //ed_init();
    }
    else if ((cpld_version & 0xF000) == 0x2000 && update_v2_5_available) {
        debugf("ED64 V2.5 detected, updating firmware...\n");
        // ed64_vseries_ll_update_firmware(firmware_v25);
        // ed_init();
    }
    else if ((cpld_version & 0xF000) == 0x0000 && update_v2_available) {
        debugf("ED64 V2.0 or below detected, updating firmware...\n");
        // ed64_vseries_ll_update_firmware(firmware_v20);
        // ed_init();
    }
    else {
        debugf("Unknown ED64 Vseries detected, or no update required.\n");
    }
    return FLASHCART_OK;
}

static flashcart_err_t ed64_vseries_init (void) {

    // Probably need to re-initialize after firmware update
    ed64_vseries_firmware_update_check_apply();

     // Enable RTC if V3
    if (get_cart_model() == ED64_V3_0) {
        debugf("ED64 V3 detected, enabling RTC mode...\n");
        //ed64_vseries_ll_enable_gpio();
        ed64_vseries_ll_v3_enable_rtc();
    }

    // although limited, we can implement a pseudo writeback system by storing the last used save path and type to SRAM bank as config data
    // then on init we can load that data back using the true writeback system?
    ed64_vseries_state_load(&current_state); // On a V3 we could use ed64_vseries_ll_set_save_type(SAVE_TYPE_SRAM_BANKED, true);

    ed64_vseries_pseudo_save_writeback();


    return FLASHCART_OK;
}

static flashcart_err_t ed64_vseries_deinit (void) {
    // // For the moment, just use libCart exit.
    // ed_exit();
    return FLASHCART_OK;
}

static bool ed64_vseries_has_feature (flashcart_features_t feature) {
    bool is_model_v3 = (get_cart_model() == ED64_V3_0); 
    switch (feature) {
        case FLASHCART_FEATURE_RTC: return is_model_v3 ? true : false;
        case FLASHCART_FEATURE_USB: return is_model_v3 ? true : false;
        case FLASHCART_FEATURE_AUTO_CIC: return is_model_v3 ? true : false;
        default: return false;
    }
}

static flashcart_err_t ed64_vseries_load_rom (char *rom_path, flashcart_progress_callback_t *progress) {
    FIL fil;
    UINT br;

    if (f_open(&fil, strip_fs_prefix(rom_path), FA_READ) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    fatfs_fix_file_size(&fil);

    size_t rom_size = f_size(&fil);

    if (rom_size > MiB(64)) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    size_t sdram_size = cart_size; //rom_size; // Adjust sdram_size based on save type and version if needed libdragon libcart provides it?

    size_t chunk_size = KiB(128);
    for (unsigned int offset = 0; offset < sdram_size; offset += chunk_size) {
        size_t block_size = MIN(sdram_size - offset, chunk_size);
        if (f_read(&fil, (void *) (ROM_ADDRESS + offset), block_size, &br) != FR_OK) {
            f_close(&fil);
            return FLASHCART_ERR_LOAD;
        }
        if (progress) {
            progress(f_tell(&fil) / (float) (f_size(&fil)));
        }
    }
    if (f_tell(&fil) != sdram_size) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    return FLASHCART_OK;
}

static flashcart_err_t ed64_vseries_load_file (char *file_path, uint32_t rom_offset, uint32_t file_offset) {
    FIL fil;
    UINT br;

    if (f_open(&fil, strip_fs_prefix(file_path), FA_READ) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    fatfs_fix_file_size(&fil);

    size_t file_size = f_size(&fil) - file_offset;

    if (file_size > (MiB(64) - rom_offset)) {
        f_close(&fil);
        return FLASHCART_ERR_ARGS;
    }

    if (f_lseek(&fil, file_offset) != FR_OK) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_read(&fil, (void *) (ROM_ADDRESS + rom_offset), file_size, &br) != FR_OK) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }
    if (br != file_size) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    return FLASHCART_OK;
}

static flashcart_err_t ed64_vseries_load_save (char *save_path) {
    
    void *address = NULL;

    ed64_vseries_save_type_t type = current_save_type;

    switch (type) {
        case SAVE_TYPE_EEPROM_4KBIT:
        case SAVE_TYPE_EEPROM_16KBIT:
            address = (void *) (EEPROM_ADDRESS);
            break;
        case SAVE_TYPE_SRAM_256KBIT:
        case SAVE_TYPE_FLASHRAM_1MBIT:
        case SAVE_TYPE_SRAM_BANKED:
        case SAVE_TYPE_SRAM_1MBIT:
            address = (void *) (SRAM_FLASHRAM_ADDRESS);
            break;
        case SAVE_TYPE_NONE:
        default:
            return FLASHCART_ERR_ARGS;
    }

    FIL fil;
    UINT br;

    if (f_open(&fil, strip_fs_prefix(save_path), FA_READ) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    size_t save_size = f_size(&fil);

    if (f_read(&fil, address, save_size, &br) != FR_OK) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    if (br != save_size) {
        return FLASHCART_ERR_LOAD;
    }

    // The ED64 Vseries doesn't have real writeback support, but we can at least store the last used save path and type
    // although limited, we can implement a pseudo writeback system by storing the last used save path and type to SRAM bank as config data
    // then on init we can load that data back using the true writeback system?
    current_state.last_save_path = save_path;
    current_state.last_save_type = type;
    current_state.is_expecting_save_writeback = true;
    ed64_vseries_state_save(&current_state);

    return FLASHCART_OK;
}

static flashcart_err_t ed64_vseries_set_save_type (flashcart_save_type_t save_type) {
    ed64_vseries_save_type_t type;

    switch (save_type) {
        case FLASHCART_SAVE_TYPE_NONE:
            type = SAVE_TYPE_NONE;
            break;
        case FLASHCART_SAVE_TYPE_EEPROM_4KBIT:
            type = SAVE_TYPE_EEPROM_4KBIT;
            break;
        case FLASHCART_SAVE_TYPE_EEPROM_16KBIT:
            type = SAVE_TYPE_EEPROM_16KBIT;
            break;
        case FLASHCART_SAVE_TYPE_SRAM_256KBIT:
            type = SAVE_TYPE_SRAM_256KBIT;
            break;
        case FLASHCART_SAVE_TYPE_SRAM_BANKED:
            type = SAVE_TYPE_SRAM_BANKED;
            break;
        case FLASHCART_SAVE_TYPE_SRAM_1MBIT:
            type = SAVE_TYPE_SRAM_1MBIT;
            break;
        case FLASHCART_SAVE_TYPE_FLASHRAM_1MBIT:
            type = SAVE_TYPE_FLASHRAM_1MBIT;
            break;
        default:
            return FLASHCART_ERR_ARGS;
    }

    // TODO: might be possible on carts that support sram 1MBit or banked sram to use config ram bank to store save data
    // if (ed64_v_ll_enable_save_writeback(false)) {
    //     return FLASHCART_ERR_INT;
    // }

    if (ed64_vseries_ll_set_save_type(type, false)) {
        return FLASHCART_ERR_INT;
    }

    current_save_type = type;

    return FLASHCART_OK;
}

static flashcart_t flashcart_ed64_vseries = {
    .init = ed64_vseries_init,
    .deinit = ed64_vseries_deinit,
    .has_feature = ed64_vseries_has_feature,
    .get_firmware_version = ed64_vseries_get_firmware_version,
    .load_rom = ed64_vseries_load_rom,
    .load_file = ed64_vseries_load_file,
    .load_save = ed64_vseries_load_save,
    .load_64dd_ipl = NULL,
    .load_64dd_disk = NULL,
    .set_save_type = ed64_vseries_set_save_type,
    .set_save_writeback = NULL,
    .set_next_boot_mode = NULL,
};


flashcart_t *ed64_vseries_get_flashcart (void) {
    return &flashcart_ed64_vseries;
}
