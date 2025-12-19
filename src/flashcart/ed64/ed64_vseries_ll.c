/**
 * @file ed64_vseries_ll.c
 * @brief Low-level functions for ED64 Vseries
 * @ingroup flashcart
 */

#include <libdragon.h>

#include "../flashcart_utils.h"
#include "ed64_vseries_ll.h"


bool ed64_vseries_ll_get_cpld_version (ed64_vseries_cpld_version_t *cpld_version) {
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
