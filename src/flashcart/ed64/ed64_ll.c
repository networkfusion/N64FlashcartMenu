#include <malloc.h>
#include <libdragon.h>
#include "utils/utils.h"
#include "ed64_ll.h"
#include "../flashcart_utils.h"


/* ED64 save location base address  */
#define ED64_SAVE_ADDR_BASE   (0xA8000000)

/* ED64 configuration registers base address  */
#define ED64_CONFIG_REGS_BASE (0xA8040000)

/* ED64 configuration registers  */
typedef enum {
    // REG_CFG = 0x00,
    // REG_STATUS = 0x01,
    // REG_DMA_LENGTH = 0x02,
    // REG_DMA_RAM_ADDR = 0x03,
    // REG_MSG = 0x04,
    // REG_DMA_CFG = 0x05,
    // REG_SPI = 0x06,
    // REG_SPI_CFG = 0x07,
    // REG_KEY = 0x08,
    REG_SAV_CFG = 0x09,
    // REG_SEC = 0x0A, /* Sectors?? */
    // REG_FPGA_VERSION = 0x0B, //11, /* Altera (Intel) MAX */
    // REG_GPIO = 0x0C, //12,

} ed64_ci_registers_id_t;


/* ED64 Device Variant Mask  */
#define ED64_DEVICE_VARIANT_MASK         (0xF000)

void set_sram_pi_regs (void);

typedef enum {
    SAV_EEP_ON_OFF = 0x01,
    SAV_SRM_ON_OFF = 0x02,
    SAV_EEP_SMALL_BIG = 0x04,
    SAV_SRM_SMALL_BIG = 0x08,
    SAV_RAM_BANK = 128,
    SAV_RAM_BANK_APPLY = 32768
} ed64_v_save_register_id_t;

void ed64_ll_reg_write (uint32_t reg, uint32_t data);

uint8_t ed64_ll_sram_bank;
ed64_save_type_t ed64_ll_save_type;


void ed64_ll_reg_write (uint32_t reg, uint32_t data) {

    *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE);
    *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE + reg * 4) = data;
    *(volatile uint32_t *) (ROM_ADDRESS);

}


ed64_save_type_t ed64_ll_get_save_type (void) {

    return ed64_ll_save_type;

}

void ed64_ll_set_save_type (ed64_save_type_t type) {

    uint16_t save_cfg;
    uint8_t is_eeprom, is_sram, is_eeprom_16k, is_sram_128k, using_ram_bank;
    ed64_ll_save_type = type;
    is_eeprom = false;
    is_sram = false;
    is_eeprom_16k = false;
    is_sram_128k = false;
    using_ram_bank = ed64_ll_sram_bank;


    switch (type) {
        case SAVE_TYPE_EEPROM_16K:
            is_eeprom = true;
            is_eeprom_16k = true;
            break;
        case SAVE_TYPE_EEPROM_4K:
            is_eeprom = true;
            break;
        case SAVE_TYPE_SRAM:
            is_sram = true;
            break;
        case SAVE_TYPE_SRAM_128K:
            is_sram = true;
            is_sram_128k = true;
            break;
        case SAVE_TYPE_FLASHRAM:
            is_sram = false;
            is_sram_128k = true;
            break;
        default:
            is_sram = false;
            is_sram_128k = false;
            using_ram_bank = 1;
            break;
    }

    if (is_eeprom)save_cfg |= SAV_EEP_ON_OFF;
    if (is_sram)save_cfg |= SAV_SRM_ON_OFF;
    if (is_eeprom_16k)save_cfg |= SAV_EEP_SMALL_BIG;
    if (is_sram_128k)save_cfg |= SAV_SRM_SMALL_BIG;
    if (using_ram_bank)save_cfg |= SAV_RAM_BANK;
    save_cfg |= SAV_RAM_BANK_APPLY;

    ed64_ll_reg_write(REG_SAV_CFG, save_cfg);

}

void ed64_ll_set_sram_bank (uint8_t bank) {

    ed64_ll_sram_bank = bank == 0 ? 0 : 1;

}

// Inits PI for sram transfers
void set_sram_pi_regs (void) {

	io_write(PI_BSD_DOM2_LAT_REG, 0x05);
	io_write(PI_BSD_DOM2_PWD_REG, 0x0C);
	io_write(PI_BSD_DOM2_PGS_REG, 0x0D);
	io_write(PI_BSD_DOM2_RLS_REG, 0x02);

}

void ed64_ll_get_sram (uint8_t *buffer, uint32_t address_offset, uint32_t size) {

    // TODO: potentially set the type so it can be used rather than an offset from the initial address.

    // collect current timings
    uint32_t initalLatReg = io_read(PI_BSD_DOM2_LAT_REG);
    uint32_t initalPwdReg = io_read(PI_BSD_DOM2_PWD_REG);
    uint32_t initalPgsReg = io_read(PI_BSD_DOM2_PGS_REG);
    uint32_t initalRlsReg = io_read(PI_BSD_DOM2_RLS_REG);

    // set temporary timings for SRAM
    set_sram_pi_regs();

    pi_dma_read_data((void*)(ED64_SAVE_ADDR_BASE + address_offset), buffer, size);

    // restore inital timings
    io_write(PI_BSD_DOM2_LAT_REG, initalLatReg);
    io_write(PI_BSD_DOM2_PWD_REG, initalPwdReg);
    io_write(PI_BSD_DOM2_PGS_REG, initalPgsReg);
    io_write(PI_BSD_DOM2_RLS_REG, initalRlsReg);

}

void ed64_ll_get_eeprom (uint8_t *buffer, uint8_t eep_type) {

    uint32_t size = eep_type == SAVE_TYPE_EEPROM_16K ? 4 : SAVE_TYPE_EEPROM_4K ? 1 : 0;

    if (size != 0) {
        ed64_ll_set_save_type(eep_type);
        if (eeprom_present()) { // FIXME: does not correctly check type.

            for (uint32_t i = 0; i < size; i += 8) {
                eeprom_read(i / 8, &buffer[i]);
            }
        }
    }

}


void ed64_ll_get_fram (uint8_t *buffer, int size) {

    ed64_ll_set_save_type(SAVE_TYPE_SRAM_128K); //2
    dma_wait();

    ed64_ll_get_sram(buffer, 0, size);
    data_cache_hit_writeback_invalidate(buffer, size);

    dma_wait();
    ed64_ll_set_save_type(SAVE_TYPE_FLASHRAM);

}


void ed64_ll_set_sram (uint8_t *buffer, uint32_t address_offset, int size) {

    // collect current timings
    uint32_t initalLatReg = io_read(PI_BSD_DOM2_LAT_REG);
    uint32_t initalPwdReg = io_read(PI_BSD_DOM2_PWD_REG);
    uint32_t initalPgsReg = io_read(PI_BSD_DOM2_PGS_REG);
    uint32_t initalRlsReg = io_read(PI_BSD_DOM2_RLS_REG);

    // set temporary timings for SRAM
    set_sram_pi_regs();

    pi_dma_write_data(buffer, (void*)(ED64_SAVE_ADDR_BASE + address_offset), size);

    // restore inital timings
    io_write(PI_BSD_DOM2_LAT_REG, initalLatReg);
    io_write(PI_BSD_DOM2_PWD_REG, initalPwdReg);
    io_write(PI_BSD_DOM2_PGS_REG, initalPgsReg);
    io_write(PI_BSD_DOM2_RLS_REG, initalRlsReg);

}


uint8_t ed64_ll_set_eeprom(uint8_t *buffer, uint8_t eep_type) {

    uint8_t size = eep_type == SAVE_TYPE_EEPROM_16K ? 4 : SAVE_TYPE_EEPROM_4K ? 1 : 0;

    if (size == 0) { // SAVE_TYPE_NONE
        return 0;
    }

    ed64_ll_set_save_type(eep_type);

    if (eeprom_present()) { // FIXME: does not correctly check type.

        for (uint32_t i = 0; i < size; i += 8) {
            eeprom_write(i / 8, &buffer[i]);
        }
    }

    return 0;
}

void ed64_ll_set_fram (uint8_t *buffer, int size) {

    ed64_ll_set_save_type(SAVE_TYPE_SRAM_128K);
    dma_wait();

    ed64_ll_set_sram(buffer, 0, size);
    data_cache_hit_writeback_invalidate(buffer, size);

    dma_wait();
    ed64_ll_set_save_type(SAVE_TYPE_FLASHRAM);

}
