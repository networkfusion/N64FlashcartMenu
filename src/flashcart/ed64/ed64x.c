// #include <stdio.h>
// #include <malloc.h>
// #include <string.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <fatfs/ff.h>
#include <libdragon.h>

#include "utils/fs.h"
#include "utils/utils.h"

#include "../flashcart_utils.h"
#include "ed64x_ll.h"
#include "ed64x.h"

typedef enum {
    // potentially handle if the firmware supports it...
    // ED64_V1_0 = 110,
    // ED64_V2_0 = 320,
    // ED64_V2_5 = 325,
    // ED64_V3_0 = 330,
    ED64_X5_0 = 550,
    ED64_X7_0 = 570,
    ED64_UKNOWN = 0,
} ed64x_device_variant_t;

/* ED64 save location base address  */
#define SRAM_ADDRESS (0xA8000000)
/* ED64 ROM location base address  */
#define ROM_ADDRESS  (0xB0000000)

/* Test FPGA */
static unsigned char ed64x_test_fpga (void) {

    unsigned char resp = 0;

    BiBootCfgSet(0x10);
    if (!BiBootCfgGet(0x10)) {resp = 1;}
    BiBootCfgClr(0x10);
    if (BiBootCfgGet(0x10)) {resp = 1;}

    return resp;
}

/* Test SDRAM */
static unsigned char ed64x_test_sdram (void) {

    unsigned long i;
    unsigned short buff[0x10000];

    for (i = 0; i < 0x10000; i++) {
        buff[i] = i;
    }
    BiCartRomWr(buff, 0, 0x20000);
    BiCartRomRd(buff, 0, 0x20000);
    for (i = 0; i < 0x10000; i++) {
        if (buff[i] != i) return 1;
    }

    return 0;
}

static flashcart_err_t ed64x_init (void) {

    // unsigned short cart_serial_number;
    // unsigned char buff[0x40];

    unsigned char mcn_wr_response = 0;
    unsigned char fpga_test_response = 0;
    unsigned char sdram_test_response = 0;

    // extern unsigned char mcn_data[];
    // extern unsigned long mcn_data_len;

    char *mcn_path = "rom:/firmware/ed64x/mcn_data.rbf";
    char *iom_path = "rom:/firmware/ed64x/iom_data.rbf";
    
    bi_init();
    // BiBootRomRd(buff, 0x3FFC0, 0x40);
    // cart_serial_number = (short)buff[10] << 8 | buff[11];

    if (file_exists(mcn_path)) {
        // FIXME: load the content of the mcn_path into the array and set its length!
        
        unsigned long mcn_data_len = 122156; // filesize(fp); // should be settable for future ... dfs_size(fh);
        unsigned char mcn_data[mcn_data_len];
        
        FILE *fp = fopen(mcn_path, "r");
        fread(&mcn_data, 1, mcn_data_len, fp);
	    fclose(fp);
        
        if (!BiBootCfgGet(BI_BCFG_BOOTMOD)) {
            mcn_wr_response = BiMCNWr(mcn_data, sizeof(mcn_data));
            if (mcn_wr_response) {assertf(false, "mcn load ed64x fail");} //return mcn_wr_response;}
            bi_init(); // reload after fw change
        }

    }

    // assertf(false, "mcn load ed64x");

    // if (file_exists(iom_path)) {
    //     // FIXME: we probably need to handle the iom data here as well.
    //     unsigned long iom_data_len = 32220; // should be settable for future ... dfs_size(fh2);
    //     unsigned char iom_data[iom_data_len];
    //     FILE *fp = fopen(iom_path, "r");
    //     fread(&iom_data, 1, iom_data_len, fp);
	//     fclose(fp);

    //     unsigned char iom_wr_response = BiIOMWr(iom_data, sizeof(iom_data));
    //     if (iom_wr_response) {assertf(false, "iom load ed64x fail");}
    //     bi_init(); // reload after fw change
    // }

    sdram_test_response = ed64x_test_sdram();
    fpga_test_response = ed64x_test_fpga();
    if (sdram_test_response || fpga_test_response) { assertf(false, "fpga ed64x fail"); } //{return 0x55;} //FIXME: is this the correct error???

    return FLASHCART_OK;
}

static flashcart_err_t ed64x_deinit (void) {

    return FLASHCART_OK;
}

static ed64x_device_variant_t get_cart_model() {
    ed64x_device_variant_t variant = ED64_X7_0; // FIXME: check cart model from ll for better feature handling.
    return variant;
}

static bool ed64x_has_feature (flashcart_features_t feature) {
    bool is_model_x7 = (get_cart_model() == ED64_X7_0); 
    switch (feature) {
        case FLASHCART_FEATURE_RTC: return is_model_x7 ? true : false;
        case FLASHCART_FEATURE_USB: return is_model_x7 ? true : false;
        case FLASHCART_FEATURE_64DD: return false;
        case FLASHCART_FEATURE_AUTO_CIC: return true;
        case FLASHCART_FEATURE_AUTO_REGION: return true;
        default: return false;
    }
}

static flashcart_err_t ed64x_load_rom (char *rom_path, flashcart_progress_callback_t *progress) {
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

    size_t sdram_size = MiB(64);

    size_t chunk_size = KiB(128);
    for (int offset = 0; offset < sdram_size; offset += chunk_size) {
        size_t block_size = MIN(sdram_size - offset, chunk_size);
        if (f_read(&fil, (void *) (ROM_ADDRESS + offset), block_size, &br) != FR_OK) {
            f_close(&fil);
            return FLASHCART_ERR_LOAD;
        }
        if (progress) {
            progress(f_tell(&fil) / (float) (f_size(&fil)));
        }
    }
    if (f_tell(&fil) != rom_size) {
        f_close(&fil);
        return FLASHCART_ERR_LOAD;
    }

    if (f_close(&fil) != FR_OK) {
        return FLASHCART_ERR_LOAD;
    }

    return FLASHCART_OK;
}

static flashcart_err_t ed64x_load_file (char *file_path, uint32_t rom_offset, uint32_t file_offset) {
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

static flashcart_err_t ed64x_load_save (char *save_path) {


    return FLASHCART_OK;
}

static flashcart_err_t ed64x_set_save_type (flashcart_save_type_t save_type) {
    

    return FLASHCART_OK;
}

static flashcart_t flashcart_ed64x = {
    .init = ed64x_init,
    .deinit = ed64x_deinit,
    .has_feature = ed64x_has_feature,
    .load_rom = ed64x_load_rom,
    .load_file = ed64x_load_file,
    .load_save = ed64x_load_save,
    .load_64dd_ipl = NULL,
    .load_64dd_disk = NULL,
    .set_save_type = ed64x_set_save_type,
    .set_save_writeback = NULL,
};


flashcart_t *ed64x_get_flashcart (void) {
    return &flashcart_ed64x;
}
