/**
 * @file ed64_vseries_ll.c
 * @brief Low-level functions for ED64 Vseries
 * @ingroup flashcart
 */

#include <libdragon.h>

#include "../flashcart_utils.h"
#include "ed64_vseries_ll.h"

#define ED_V_DMA_SD_TO_RAM 1
#define ED_V_DMA_RAM_TO_SD 2
#define ED_V_DMA_FIFO_TO_RAM 3 //USB
#define ED_V_DMA_RAM_TO_FIFO 4 //USB

#define ED_V_SAV_EEP_ON           (1 << 0)
#define ED_V_SAV_SRM_ON           (1 << 1)
#define ED_V_SAV_EEP_SIZE_LARGE   (1 << 2)
#define ED_V_SAV_SRM_SIZE_LARGE   (1 << 3)
#define ED_V_SAV_RAM_BANK_ON      (1 << 7)
#define ED_V_SAV_RAM_BANK_APPLY   (1 << 15)

#define ED_V_STATE_DMA_BUSY       (1 << 0) // 1
#define ED_V_STATE_DMA_TOUT       (1 << 1) // 2
#define ED_V_STATE_USB_TXE        (1 << 2) // 4
#define ED_V_STATE_USB_RXF        (1 << 3) // 8
#define ED_V_STATE_SPI            (1 << 4) // 16

#define ED_V_CFG_SDRAM_OFF        (0 << 0) // 0
#define ED_V_CFG_SDRAM_ON         (1 << 0) // 1
#define ED_V_CFG_BYTESWAP         (1 << 1) // 2
#define ED_V_CFG_WR_MOD           (1 << 2) // 4
#define ED_V_CFG_WR_ADDR_MASK     (1 << 3) // 8
// 16 reserved
#define ED_V_CFG_MODE_RTC_OFF     (0 << 5)
#define ED_V_CFG_MODE_RTC_ON      (1 << 5) // 32
// 64 reserved
#define ED_V_CFG_MODE_GPIO_OFF    (0 << 6)
#define ED_V_CFG_MODE_GPIO_ON     (1 << 6) // 96 - this is strange... TODO: how to correctly use?
// 128 reserved
#define ED_V_CFG_DD_CC_ON         (1 << 8) // 256 // handle cart rom dd-cart conversion rom
#define ED_V_CFG_DD_CC_WE         (1 << 9) // 512


uint8_t ed64_vseries_ll_dma_busy() {
    // uint32_t resp;
    // while ((resp = io_read(ED_V_STATUS_REG)) & ED_V_STATE_DMA_BUSY) {
    //     if (resp & ED_V_STATE_DMA_TOUT)
    //     {
    //         return resp & ED_V_STATE_DMA_TOUT;
    //     }
    // }
    // return 0;
    while ((io_read(ED_V_STATUS_REG) & ED_V_STATE_DMA_BUSY) != 0);
    return io_read(ED_V_STATUS_REG) & ED_V_STATE_DMA_TOUT;

}

uint8_t ed64_vseries_ll_usb_read_busy() {
    return io_read(ED_V_STATUS_REG) & ED_V_STATE_USB_RXF;
}

uint8_t ed64_vseries_ll_usb_write_busy() {
    return io_read(ED_V_STATUS_REG) & ED_V_STATE_USB_TXE;
}


uint8_t ed64_vseries_ll_usb_read(uint32_t address, uint32_t length) {

    address /= 4;
    while (ed64_vseries_ll_usb_read_busy() != 0);

    io_write(ED_V_DMA_LEN_REG, length - 1);
    io_write(ED_V_DMA_ADDR_REG, address);
    io_write(ED_V_DMA_CFG_REG, ED_V_DMA_FIFO_TO_RAM);

    if (ed64_vseries_ll_dma_busy() != 0)return 1; //EVD_ERROR_FIFO_TIMEOUT;

    return 0;
}

uint8_t ed64_vseries_ll_usb_write(uint32_t address, uint32_t length) {

    address /= 4;
    while (ed64_vseries_ll_usb_write_busy() != 0);

    io_write(ED_V_DMA_LEN_REG, length - 1);
    io_write(ED_V_DMA_ADDR_REG, address);
    io_write(ED_V_DMA_CFG_REG, ED_V_DMA_RAM_TO_FIFO);

    if (ed64_vseries_ll_dma_busy() != 0)return 1; //EVD_ERROR_FIFO_TIMEOUT;

    return 0;
}

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

void ed64_vseries_ll_enable_gpio(void)
{
    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg |= ED_V_CFG_MODE_GPIO_ON;
    io_write(ED_V_CFG_REG, cfg);
}

void ed64_vseries_ll_disable_gpio(void)
{
    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg &= ~ED_V_CFG_MODE_GPIO_ON;
    io_write(ED_V_CFG_REG, cfg);
}

void ed64_vseries_ll_v3_enable_rtc (void) {
    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg &= ~ED_V_CFG_MODE_GPIO_ON;
    cfg |= ED_V_CFG_MODE_RTC_ON;
    io_write(ED_V_CFG_REG, cfg);
}

void ed64_vseries_ll_v3_write(uint16_t address, uint16_t data) {
    io_write(ED_V_3_FLA_ADDR_REG, address);
    io_write(ED_V_3_FLA_DATA_REG, data);
}

bool ed64_vseries_ll_set_save_type(ed64_vseries_save_type_t type, bool use_config_ram_bank) {

    bool config_ram_bank_enable = !use_config_ram_bank;
    uint16_t save_cfg = 0; // io_read(ED_SAV_CFG_REG);

    switch (type) {
        case SAVE_TYPE_EEPROM_4KBIT:
            save_cfg |= ED_V_SAV_EEP_ON;
            break;
        case SAVE_TYPE_EEPROM_16KBIT:
            save_cfg |= ED_V_SAV_EEP_ON;
            save_cfg |= ED_V_SAV_EEP_SIZE_LARGE;
            break;
        case SAVE_TYPE_SRAM_256KBIT:
            save_cfg |= ED_V_SAV_SRM_ON;
            break;
        case SAVE_TYPE_SRAM_BANKED:
        case SAVE_TYPE_SRAM_1MBIT:
            save_cfg |= ED_V_SAV_SRM_ON;
            save_cfg |= ED_V_SAV_SRM_SIZE_LARGE;
            break;
        case SAVE_TYPE_FLASHRAM_1MBIT:
            save_cfg |= ED_V_SAV_SRM_SIZE_LARGE;
            break;
        case SAVE_TYPE_NONE:
        default:
            save_cfg |= ~ED_V_SAV_SRM_ON;
            save_cfg |= ~ED_V_SAV_SRM_SIZE_LARGE;
            save_cfg |= ED_V_SAV_RAM_BANK_ON;
            break;
    }
    
    if (config_ram_bank_enable) { save_cfg |= ED_V_SAV_RAM_BANK_ON; }

    save_cfg |= ED_V_SAV_RAM_BANK_APPLY;

    io_write(ED_V_SAV_CFG_REG, save_cfg);

    // TODO: verify write
    return false; // false on success
}

void ed64_vseries_ll_update_firmware(uint8_t *firmware_data) {

    debugf("Starting firmware update...\n");

    //uint16_t cfg = io_read(ED_V_CFG_REG);
    io_write(ED_V_CFG_REG, ED_V_CFG_SDRAM_OFF); // disable sram during firmware update

    io_write(ED_V_CFG_CNT_REG, 0);
    wait_ms(10);
    io_write(ED_V_CFG_CNT_REG, 1);
    wait_ms(10);

    uint32_t i = 0;
    uint16_t f_ctr = 0;
    for (;;) {

        io_write(ED_V_CFG_DAT_REG, *(uint16_t *) & firmware_data[i]);
        while ((io_read(ED_V_CFG_CNT_REG) & 8) != 0);

        f_ctr = firmware_data[i++] == 0xFF ? f_ctr + 1 : 0;
        if (f_ctr >= 47)break;
        f_ctr = firmware_data[i++] == 0xFF ? f_ctr + 1 : 0;
        if (f_ctr >= 47)break;
    }


    while ((io_read(ED_V_CFG_CNT_REG) & 4) == 0) {
        io_write(ED_V_CFG_DAT_REG, 0xFFFF);
        while ((io_read(ED_V_CFG_CNT_REG) & 8) != 0);
    }

    wait_ms(20);

    //io_write(ED_V_CFG_REG, ED_V_CFG_SDRAM_ON); //reenable sram

    //ed_init();

    debugf("Firmware update completed.\n");
}

uint16_t ed64_vseries_ll_message_read(void) {
    return io_read(ED_V_MSG_REG);
}

void ed64_vseries_ll_message_write(uint16_t data) {
    io_write(ED_V_MSG_REG, data);
}

void ed64_vseries_ll_dd_cc_ram_oe(void) {

    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg &= ~ED_V_CFG_DD_CC_WE;
    cfg |= ED_V_CFG_DD_CC_ON;
    io_write(ED_V_CFG_REG, cfg);
}

void ed64_vseries_ll_dd_cc_ram_we(void) {

    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg |= ED_V_CFG_DD_CC_ON | ED_V_CFG_DD_CC_WE;
    io_write(ED_V_CFG_REG, cfg);
}

void ed64_vseries_ll_dd_cc_ram_off(void) {

    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg &= ~(ED_V_CFG_DD_CC_ON | ED_V_CFG_DD_CC_WE);
    io_write(ED_V_CFG_REG, cfg);
}

void ed64_vseries_ll_dd_cc_ram_clr(void) {

    uint16_t cfg = io_read(ED_V_CFG_REG);
    cfg |= ED_V_CFG_DD_CC_WE;
    cfg &= ~ED_V_CFG_DD_CC_ON;
    io_write(ED_V_CFG_REG, cfg);
    wait_ms(100);
}

bool ed64_vseries_ll_dd_ram_supported(void) {

    return (io_read(ED_V_STATUS_REG) >> 15) & 1;
}
