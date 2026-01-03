/**
 * @file ed64_vseries_ll.c
 * @brief Low-level functions for ED64 Vseries
 * @ingroup flashcart
 */

#include <libdragon.h>
#include <libcart/cart.h>
#include "../flashcart_utils.h"
#include "ed64_vseries_ll.h"
#include "ed64_vseries_regs.h"


// void edv_3_init() {

//     uint8_t buff[1024];
//     uint16_t cfg = ed64_register_read(REG_CFG);

//     ed64_register_write(REG_CFG, 0);
//     ed64_register_write(REG_CFG_CNT, 161);
//     ed64_io_reg_v3(0x55, 0x98);
//     ed64_dma_read_rom(buff, 0, 2);
//     ed64_io_reg_v3(0x55, 0xF0);
//     ed64_dma_read_rom(buff, 0, 2);
//     ed64_dma_read_rom(buff, 1024, 2);
//     ed64_dma_read_rom(buff, 1024 + 256 - 2, 2);
//     ed64_register_write(REG_CFG_CNT, 1);

//     ed64_register_write(REG_CFG, cfg);
// }


// uint8_t ed64_vseries_ll_dma_busy() {
//     // uint32_t resp;
//     // while ((resp = io_read(ED_STATUS_REG)) & ED_STATE_DMA_BUSY) {
//     //     if (resp & ED_STATE_DMA_TOUT)
//     //     {
//     //         return resp & ED_STATE_DMA_TOUT;
//     //     }
//     // }
//     // return 0;
//     while ((io_read(ED_STATUS_REG) & ED_STATE_DMA_BUSY) != 0);
//     return io_read(ED_STATUS_REG) & ED_STATE_DMA_TOUT;

// }

// uint8_t ed64_vseries_ll_usb_read_busy() {
//     return io_read(ED_STATUS_REG) & ED_STATE_RXF;
// }

// uint8_t ed64_vseries_ll_usb_write_busy() {
//     return io_read(ED_STATUS_REG) & ED_STATE_TXE;
// }


// uint8_t ed64_vseries_ll_usb_read(uint32_t address, uint32_t length) {

//     address /= 4;
//     while (ed64_vseries_ll_usb_read_busy() != 0);

//     io_write(ED_DMA_LEN_REG, length - 1);
//     io_write(ED_DMA_ADDR_REG, address);
//     io_write(ED_DMA_CFG_REG, ED_DMA_FIFO_TO_RAM);

//     if (ed64_vseries_ll_dma_busy() != 0)return 1; //EVD_ERROR_FIFO_TIMEOUT;

//     return 0;
// }

// uint8_t ed64_vseries_ll_usb_write(uint32_t address, uint32_t length) {

//     address /= 4;
//     while (ed64_vseries_ll_usb_write_busy() != 0);

//     io_write(ED_DMA_LEN_REG, length - 1);
//     io_write(ED_DMA_ADDR_REG, address);
//     io_write(ED_DMA_CFG_REG, ED_DMA_RAM_TO_FIFO);

//     if (ed64_vseries_ll_dma_busy() != 0)return 1; //EVD_ERROR_FIFO_TIMEOUT;

//     return 0;
// }

bool ed64_vseries_ll_get_cpld_version (uint16_t *cpld_version) {
    uint16_t ver;
    uint16_t cfg;
    cfg = io_read(ED_CFG_REG);
    io_write(ED_CFG_REG, 0x00);
    ver = io_read(ED_MAX_VER_REG);
    io_write(ED_CFG_REG, cfg);
    *cpld_version = ver; //  & 0xFFFF; //CPLD_VERSION_3_0;

    return true;
}

bool ed64_vseries_ll_get_fpga_version (uint16_t *fpga_version) {
    uint16_t ver;
    ver = io_read(ED_VER_REG);
    *fpga_version = ver;

    return true;
}

// void ed64_vseries_ll_enable_gpio(void)
// {
//     uint16_t cfg = io_read(ED_CFG_REG);
//     cfg |= ED_CFG_MODE_GPIO_ON;
//     io_write(ED_CFG_REG, cfg);
// }

// void ed64_vseries_ll_disable_gpio(void)
// {
//     uint16_t cfg = io_read(ED_CFG_REG);
//     cfg &= ~ED_CFG_MODE_GPIO_ON;
//     io_write(ED_CFG_REG, cfg);
// }

void ed64_vseries_ll_v3_enable_rtc (void) {
    uint16_t cfg = io_read(ED_CFG_REG);
    // cfg &= ~ED_CFG_MODE_GPIO_ON;
    cfg |= ED_CFG_MODE_RTC_ON;
    io_write(ED_CFG_REG, cfg);
}

// void ed64_vseries_ll_v3_write(uint16_t address, uint16_t data) {
//     io_write(ED_V3_FLA_ADDR_REG, address);
//     io_write(ED_V3_FLA_DATA_REG, data);
// }

bool ed64_vseries_ll_set_save_type(ed64_vseries_save_type_t type, bool use_config_ram_bank) {

    bool config_ram_bank_enable = !use_config_ram_bank;
    uint32_t save_cfg = 0; // io_read(ED_SAV_CFG_REG);

    switch (type) {
        case SAVE_TYPE_EEPROM_4KBIT:
            save_cfg |= ED_SAV_EEP_ON;
            // save_cfg |= ED_SAV_EEP_SMALL;
            // save_cfg |= ED_SAV_SRM_OFF;
            // save_cfg |= ED_SAV_SRM_SMALL;
            break;
        case SAVE_TYPE_EEPROM_16KBIT:
            save_cfg |= ED_SAV_EEP_ON;
            save_cfg |= ED_SAV_EEP_LARGE;
            // save_cfg |= ED_SAV_SRM_OFF;
            // save_cfg |= ED_SAV_SRM_SMALL;
            break;
        case SAVE_TYPE_SRAM_256KBIT:
            // save_cfg |= ED_SAV_EEP_OFF;
            // save_cfg |= ED_SAV_EEP_SMALL;
            save_cfg |= ED_SAV_SRM_ON;
            save_cfg |= ED_SAV_SRM_SMALL;
            break;
        case SAVE_TYPE_SRAM_BANKED:
        case SAVE_TYPE_SRAM_1MBIT:
            // save_cfg |= ED_SAV_EEP_OFF;
            // save_cfg |= ED_SAV_EEP_SMALL;
            save_cfg |= ED_SAV_SRM_ON;
            save_cfg |= ED_SAV_SRM_LARGE;
            break;
        case SAVE_TYPE_FLASHRAM_1MBIT:
            // save_cfg |= ED_SAV_EEP_OFF;
            // save_cfg |= ED_SAV_EEP_SMALL;
            save_cfg |= ED_SAV_SRM_OFF;
            save_cfg |= ED_SAV_SRM_LARGE;
            break;
        case SAVE_TYPE_NONE:
        default:
            // save_cfg |= ED_SAV_EEP_OFF;
            // save_cfg |= ~ED_SAV_EEP_SMALL;
            // save_cfg |= ~ED_SAV_SRM_ON;
            // save_cfg |= ~ED_SAV_SRM_LARGE;
            // save_cfg |= ED_SAV_RAM_BANK_ON;
            break;
    }
    
    // if (config_ram_bank_enable) { save_cfg |= ED_SAV_RAM_BANK_ON; }

    // save_cfg |= ED_SAV_RAM_BANK_APPLY;

    io_write(ED_SAV_CFG_REG, save_cfg);

    // TODO: verify write
    return false; // false on success
}

// void ed64_vseries_ll_update_firmware(uint8_t *firmware_data) {

//     debugf("Starting firmware update...\n");

//     //uint16_t cfg = io_read(ED_CFG_REG);
//     io_write(ED_CFG_REG, ED_CFG_SDRAM_OFF); // disable sram during firmware update

//     io_write(ED_CFG_CNT_REG, 0);
//     wait_ms(10);
//     io_write(ED_CFG_CNT_REG, 1);
//     wait_ms(10);

//     uint32_t i = 0;
//     uint16_t f_ctr = 0;
//     for (;;) {

//         io_write(ED_CFG_DAT_REG, *(uint16_t *) & firmware_data[i]);
//         while ((io_read(ED_CFG_CNT_REG) & 8) != 0);

//         f_ctr = firmware_data[i++] == 0xFF ? f_ctr + 1 : 0;
//         if (f_ctr >= 47)break;
//         f_ctr = firmware_data[i++] == 0xFF ? f_ctr + 1 : 0;
//         if (f_ctr >= 47)break;
//     }


//     while ((io_read(ED_CFG_CNT_REG) & 4) == 0) {
//         io_write(ED_CFG_DAT_REG, 0xFFFF);
//         while ((io_read(ED_CFG_CNT_REG) & 8) != 0);
//     }

//     wait_ms(20);

//     io_write(ED_CFG_REG, ED_CFG_SDRAM_ON); //re-enable sram

//     ed_init();

//     debugf("Firmware update completed.\n");
// }

// uint16_t ed64_vseries_ll_message_read(void) {
//     return io_read(ED_MSG_REG);
// }

// void ed64_vseries_ll_message_write(uint16_t data) {
//     io_write(ED_MSG_REG, data);
// }

// void ed64_vseries_ll_dd_cc_ram_oe(void) {

//     uint16_t cfg = io_read(ED_CFG_REG);
//     cfg &= ~ED_CFG_DD_CC_WE;
//     cfg |= ED_CFG_DD_CC_ON;
//     io_write(ED_CFG_REG, cfg);
// }

// void ed64_vseries_ll_dd_cc_ram_we(void) {

//     uint16_t cfg = io_read(ED_CFG_REG);
//     cfg |= ED_CFG_DD_CC_ON | ED_CFG_DD_CC_WE;
//     io_write(ED_CFG_REG, cfg);
// }

// void ed64_vseries_ll_dd_cc_ram_off(void) {

//     uint16_t cfg = io_read(ED_CFG_REG);
//     cfg &= ~(ED_CFG_DD_CC_ON | ED_CFG_DD_CC_WE);
//     io_write(ED_CFG_REG, cfg);
// }

// void ed64_vseries_ll_dd_cc_ram_clr(void) {

//     uint16_t cfg = io_read(ED_CFG_REG);
//     cfg |= ED_CFG_DD_CC_WE;
//     cfg &= ~ED_CFG_DD_CC_ON;
//     io_write(ED_CFG_REG, cfg);
//     wait_ms(100);
// }

// bool ed64_vseries_ll_dd_ram_supported(void) {

//     return (io_read(ED_STATUS_REG) >> 15) & 1;
// }
