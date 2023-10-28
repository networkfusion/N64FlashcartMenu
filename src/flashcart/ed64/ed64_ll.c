#include <malloc.h>
#include <libdragon.h>
#include "utils/utils.h"
#include "ed64_ll.h"



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

void pi_initialize (void);
void pi_initialize_sram (void);
void pi_dma_from_cart (void* dest, void* src, unsigned long size);
void pi_dma_to_cart (void* dest, void* src, unsigned long size);
void pi_dma_from_sram (void *dest, unsigned long offset, unsigned long size);
void pi_dma_to_sram (void* src, unsigned long offset, unsigned long size);
void pi_dma_from_cart_safe (void *dest, void *src, unsigned long size);

void ed64_ll_set_sdcard_timing (void);


typedef enum {
    SAV_EEP_ON_OFF = 0x01,
    SAV_SRM_ON_OFF = 0x02,
    SAV_EEP_SMALL_BIG = 0x04,
    SAV_SRM_SMALL_BIG = 0x08,
    SAV_RAM_BANK = 128,
    SAV_RAM_BANK_APPLY = 32768
} ed64_v_save_register_id_t;

uint32_t ed64_ll_reg_read (uint32_t reg);
void ed64_ll_reg_write (uint32_t reg, uint32_t data);

uint8_t ed64_ll_sram_bank;
ed64_save_type_t ed64_ll_save_type;


uint32_t ed64_ll_reg_read (uint32_t reg) {

    *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE);
    return *(volatile uint32_t *) (ED64_CONFIG_REGS_BASE + reg * 4);

}

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


void pi_initialize (void) {

	dma_wait();
	io_write(PI_STATUS_REG, 0x03);

}

// Inits PI for sram transfer
void pi_initialize_sram (void) {

	io_write(PI_BSD_DOM2_LAT_REG, 0x05);
	io_write(PI_BSD_DOM2_PWD_REG, 0x0C);
	io_write(PI_BSD_DOM2_PGS_REG, 0x0D);
	io_write(PI_BSD_DOM2_RLS_REG, 0x02);

}

void pi_dma_from_sram (void *dest, unsigned long offset, unsigned long size) {

	io_write(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
	io_write(PI_CART_ADDR_REG, (0xA8000000 + offset));
	 asm volatile ("" : : : "memory");
	io_write(PI_WR_LEN_REG, (size - 1));
	 asm volatile ("" : : : "memory");

}


void pi_dma_to_sram (void *src, unsigned long offset, unsigned long size) {

	dma_wait();

	io_write(PI_STATUS_REG, 2);
	io_write(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
	io_write(PI_CART_ADDR_REG, (0xA8000000 + offset));
	io_write(PI_RD_LEN_REG, (size - 1));

}

void pi_dma_from_cart (void* dest, void* src, unsigned long size) {

	dma_wait();

	io_write(PI_STATUS_REG, 0x03);
	io_write(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
	io_write(PI_CART_ADDR_REG, K0_TO_PHYS(src));
	io_write(PI_WR_LEN_REG, (size - 1));

}


void pi_dma_to_cart (void* dest, void* src, unsigned long size) {

	dma_wait();

	io_write(PI_STATUS_REG, 0x02);
	io_write(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
	io_write(PI_CART_ADDR_REG, K0_TO_PHYS(dest));
	io_write(PI_RD_LEN_REG, (size - 1));

}


// Wrapper to support unaligned access to memory
void pi_dma_from_cart_safe (void *dest, void *src, unsigned long size) {

	if (!dest || !src || !size) return;

	unsigned long unalignedSrc  = ((unsigned long)src)  % 2;
	unsigned long unalignedDest = ((unsigned long)dest) % 8;

	//FIXME: Do i really need to check if size is 16bit aligned?
	if (!unalignedDest && !unalignedSrc && !(size % 2)) {
		pi_dma_from_cart(dest, src, size);
		dma_wait();

		return;
	}

	void* newSrc = (void*)(((unsigned long)src) - unalignedSrc);
	unsigned long newSize = (size + unalignedSrc) + ((size + unalignedSrc) % 2);

	unsigned char *buffer = memalign(8, newSize);
	pi_dma_from_cart(buffer, newSrc, newSize);
	dma_wait();

	memcpy(dest, (buffer + unalignedSrc), size);

	free(buffer);

}


int ed64_ll_get_sram_128 (uint8_t *buffer, int size) {

    dma_wait();

    io_write(PI_BSD_DOM2_LAT_REG, 0x05);
    io_write(PI_BSD_DOM2_PWD_REG, 0x0C);
    io_write(PI_BSD_DOM2_PGS_REG, 0x0D);
    io_write(PI_BSD_DOM2_RLS_REG, 0x02);

    dma_wait();

    pi_initialize();

    dma_wait();

    // Offset Large RAM size 128KiB with a 16KiB nudge to allocate enough space
    // We do this because 128Kib is not recognised and a 32KiB allocates too little 
    pi_dma_from_sram(buffer, -(size - KiB(16)), size);

    dma_wait();

    io_write(PI_BSD_DOM2_LAT_REG, 0x40);
    io_write(PI_BSD_DOM2_PWD_REG, 0x12);
    io_write(PI_BSD_DOM2_PGS_REG, 0x07);
    io_write(PI_BSD_DOM2_RLS_REG, 0x03);

    return 1;

}


int ed64_ll_get_sram (uint8_t *buffer, int size) {

    dma_wait();

    io_write(PI_BSD_DOM2_LAT_REG, 0x05);
    io_write(PI_BSD_DOM2_PWD_REG, 0x0C);
    io_write(PI_BSD_DOM2_PGS_REG, 0x0D);
    io_write(PI_BSD_DOM2_RLS_REG, 0x02);

    dma_wait();

    pi_initialize();

    dma_wait();

    pi_dma_from_sram(buffer, 0, size) ;

    dma_wait();

    io_write(PI_BSD_DOM2_LAT_REG, 0x40);
    io_write(PI_BSD_DOM2_PWD_REG, 0x12);
    io_write(PI_BSD_DOM2_PGS_REG, 0x07);
    io_write(PI_BSD_DOM2_RLS_REG, 0x03);

    return 1;

}

void ed64_ll_get_eeprom (uint8_t *buffer, uint8_t eep_type) {


    uint32_t i;
    uint32_t slen = eep_type == SAVE_TYPE_EEPROM_16K ? 4 : SAVE_TYPE_EEPROM_4K ? 1 : 0;
    if (slen == 0) {
        return;
    }

    ed64_ll_set_save_type(eep_type);
    eeprom_present();

    for (i = 0; i < slen * 512; i += 8) {
        eeprom_read(i / 8, &buffer[i]);
    }
    ed64_ll_set_save_type(SAVE_TYPE_NONE);

}


int ed64_ll_get_fram (uint8_t *buffer, int size) {

    ed64_ll_set_save_type(SAVE_TYPE_SRAM_128K); //2
    dma_wait();

    ed64_ll_get_sram_128(buffer, size);
    data_cache_hit_writeback_invalidate(buffer, size);

    dma_wait();
    ed64_ll_set_save_type(SAVE_TYPE_FLASHRAM);

    return 1;

}

/*
sram upload
*/


int ed64_ll_set_sram_128 (uint8_t *buffer, int size) {

    //half working
    dma_wait();
    //Timing
    pi_initialize_sram();

    //Readmode
    pi_initialize();

    data_cache_hit_writeback_invalidate(buffer,size);
    dma_wait();

    // Offset Large RAM size 128KiB with a 16KiB nudge to allocate enough space
    // We do this because 128Kib is not recognised and a 32KiB allocates too little 
    pi_dma_to_sram(buffer, -(size - KiB(16)), size);
    data_cache_hit_writeback_invalidate(buffer,size);

    //Wait
    dma_wait();
    //Restore evd Timing
    ed64_ll_set_sdcard_timing();

    return 1;

}


int ed64_ll_set_sram (uint8_t *buffer, int size) {

    //half working
    dma_wait();
    //Timing
    pi_initialize_sram();

    //Readmode
    pi_initialize();

    data_cache_hit_writeback_invalidate(buffer,size);
    dma_wait();

    pi_dma_to_sram(buffer, 0, size);
    data_cache_hit_writeback_invalidate(buffer,size);

    //Wait
    dma_wait();
    //Restore evd Timing
    ed64_ll_set_sdcard_timing();

    return 1;

}


uint8_t ed64_ll_set_eeprom(uint8_t *src, uint8_t eep_type, bool verify) {

    uint8_t buff[2048];
    uint32_t i;
    uint8_t slen = eep_type == SAVE_TYPE_EEPROM_16K ? 4 : SAVE_TYPE_EEPROM_4K ? 1 : 0;
    if (slen == 0) {
        return 0;
    }

    ed64_ll_set_save_type(eep_type);
    eeprom_present();

    for (i = 0; i < slen * 512; i += 8) {
        eeprom_write(i / 8, &src[i]);
    }

    if (verify) {
        for (i = 0; i < slen * 512; i += 8) {
            eeprom_read(i / 8, &buff[i]);
        }
        for (i = 0; i < slen * 512; i++) {
            if (src[i] != buff[i]) {
                return 1; //ERR_EEP_CHECK;
            }
        }
    }

    ed64_ll_set_save_type(SAVE_TYPE_NONE);

    return 0;
}

int ed64_ll_set_fram (uint8_t *buffer, int size) {

    ed64_ll_set_save_type(SAVE_TYPE_SRAM_128K);
    dma_wait();

    ed64_ll_set_sram_128(buffer, size);
    data_cache_hit_writeback_invalidate(buffer, size);

    dma_wait();
    ed64_ll_set_save_type(SAVE_TYPE_FLASHRAM);

    return 1;

}


void ed64_ll_set_sdcard_timing (void) {

    io_write(PI_BSD_DOM1_LAT_REG, 0x40);
    io_write(PI_BSD_DOM1_PWD_REG, 0x12);
    io_write(PI_BSD_DOM1_PGS_REG, 0x07);
    io_write(PI_BSD_DOM1_RLS_REG, 0x03);

    io_write(PI_BSD_DOM2_LAT_REG, 0x40);
    io_write(PI_BSD_DOM2_PWD_REG, 0x12);
    io_write(PI_BSD_DOM2_PGS_REG, 0x07);
    io_write(PI_BSD_DOM2_RLS_REG, 0x03);

}
