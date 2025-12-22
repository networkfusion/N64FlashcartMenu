/**
 * @file ed64_vseries_ll.c
 * @brief Low-level functions for ED64 Vseries
 * @ingroup flashcart
 */

#include <libdragon.h>

#include "../flashcart_utils.h"
#include "ed64_vseries_ll.h"

#define ED_V_SAV_EEP_ON           (1 << 0)
#define ED_V_SAV_SRM_ON           (1 << 1)
#define ED_V_SAV_EEP_SIZE_LARGE   (1 << 2)
#define ED_V_SAV_SRM_SIZE_LARGE   (1 << 3)
#define ED_V_SAV_RAM_BANK_ON      (1 << 7)
#define ED_V_SAV_RAM_BANK_APPLY   (1 << 15)


bool ed64_vseries_ll_get_cpld_version (uint16_t *cpld_version) {
    uint16_t ver;
    uint16_t cfg;
    cfg = io_read(ED_V_CFG_REG);
    io_write(ED_V_CFG_REG, 0x00);
    ver = io_read(ED_V_MAX_VER_REG);
    io_write(ED_V_CFG_REG, cfg);
    *cpld_version = ver; //  & 0xFFFF; //CPLD_VERSION_3_0;

    return true;
}

bool ed64_vseries_ll_get_fpga_version (uint16_t *fpga_version) {
    uint16_t ver;
    ver = io_read(ED_V_VER_REG);
    *fpga_version = ver;

    return true;
}

// void ed64_vseries_ll_v3_write(uint16_t address, uint8_t *data) {
//     io_write(ED_V_3_FL_ADDR_REG, address);
//     io_write(ED_V_3_FL_DATA_REG, data);
// }

bool ed64_vseries_ll_set_save_type(ed64_vseries_save_type_t type, bool use_ram_bank) {

    uint32_t save_cfg = 0;
    bool eeprom_on = false;
    bool sram_on = false;
    bool eeprom_size_large = false;
    bool sram_size_large = false;
    bool config_ram_bank_enable = !use_ram_bank;

    switch (type) {
        case SAVE_TYPE_EEPROM_16KBIT:
            eeprom_on = true;
            eeprom_size_large = true;
            break;
        case SAVE_TYPE_EEPROM_4KBIT:
            eeprom_on = true;
            break;
        case SAVE_TYPE_SRAM_256KBIT:
            sram_on = true;
            break;
        case SAVE_TYPE_SRAM_BANKED:
        case SAVE_TYPE_SRAM_1MBIT:
            sram_on = true;
            sram_size_large = true;
            break;
        case SAVE_TYPE_FLASHRAM_1MBIT:
            sram_on = false;
            sram_size_large = true;
            break;
        default:
            sram_on = false;
            sram_size_large = false;
            config_ram_bank_enable = true;
            break;
    }
    
    if (eeprom_on) { save_cfg |= ED_V_SAV_EEP_ON; }
    if (sram_on) { save_cfg |= ED_V_SAV_SRM_ON; }
    if (eeprom_size_large) { save_cfg |= ED_V_SAV_EEP_SIZE_LARGE; }
    if (sram_size_large) { save_cfg |= ED_V_SAV_SRM_SIZE_LARGE; }
    if (config_ram_bank_enable) { save_cfg |= ED_V_SAV_RAM_BANK_ON; }
    save_cfg |= ED_V_SAV_RAM_BANK_APPLY;

    io_write(ED_V_SAV_CFG_REG, save_cfg);

    // TODO: verify write
    return false; // false on success
}
