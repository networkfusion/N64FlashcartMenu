#include <malloc.h>
#include <libdragon.h>
#include "utils/utils.h"
#include "ed64_ll.h"

void sleep(unsigned long ms);

void sleep(unsigned long ms) {

    unsigned long current_ms = get_ticks_ms();

    while (get_ticks_ms() - current_ms < ms);

}


/* ED64 configuration registers base address  */
#define ED64_CONFIG_REGS_BASE (0xA8040000)

typedef enum {
    // REG_CFG = 0,
    // REG_STATUS = 1,
    // REG_DMA_LENGTH = 2,
    // REG_DMA_RAM_ADDR = 3,
    // REG_MSG = 4,
    // REG_DMA_CFG = 5,
    // REG_SPI = 6,
    // REG_SPI_CFG = 7,
    // REG_KEY = 8,
    REG_SAV_CFG = 9,
    // REG_SEC = 10, /* Sectors?? */
    // REG_FPGA_VERSION = 11, /* Altera (Intel) MAX */
    // REG_GPIO = 12,

} ed64_registers_t;

void pi_initialize (void);
void pi_dma_wait (void);
void pi_initialize_sram (void);
void pi_dma_from_cart (void* dest, void* src, unsigned long size);
void pi_dma_to_cart (void* dest, void* src, unsigned long size);
void pi_dma_from_sram (void *dest, unsigned long offset, unsigned long size);
void pi_dma_to_sram (void* src, unsigned long offset, unsigned long size);
void pi_dma_from_cart_safe (void *dest, void *src, unsigned long size);

void ed64_ll_set_sdcard_timing (void);


#define SAV_EEP_ON 1
#define SAV_SRM_ON 2
#define SAV_EEP_SIZE 4
#define SAV_SRM_SIZE 8

#define SAV_RAM_BANK 128
#define SAV_RAM_BANK_APPLY 32768

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
    uint8_t eeprom_on, sram_on, eeprom_size, sram_size, ram_bank;
    ed64_ll_save_type = type;
    eeprom_on = 0;
    sram_on = 0;
    eeprom_size = 0;
    sram_size = 0;
    ram_bank = ed64_ll_sram_bank;


    switch (type) {
        case SAVE_TYPE_EEPROM_16K:
            eeprom_on = 1;
            eeprom_size = 1;
            break;
        case SAVE_TYPE_EEPROM_4K:
            eeprom_on = 1;
            break;
        case SAVE_TYPE_SRAM:
            sram_on = 1;
            break;
        case SAVE_TYPE_SRAM_128K:
            sram_on = 1;
            sram_size = 1;
            break;
        case SAVE_TYPE_FLASHRAM:
            sram_on = 0;
            sram_size = 1;
            break;
        default:
            sram_on = 0;
            sram_size = 0;
            ram_bank = 1;
            break;
    }

    if (eeprom_on)save_cfg |= SAV_EEP_ON;
    if (sram_on)save_cfg |= SAV_SRM_ON;
    if (eeprom_size)save_cfg |= SAV_EEP_SIZE;
    if (sram_size)save_cfg |= SAV_SRM_SIZE;
    if (ram_bank)save_cfg |= SAV_RAM_BANK;
    save_cfg |= SAV_RAM_BANK_APPLY;

    ed64_ll_reg_write(REG_SAV_CFG, save_cfg);

}

void ed64_ll_set_sram_bank (uint8_t bank) {

    ed64_ll_sram_bank = bank == 0 ? 0 : 1;

}

void _data_cache_invalidate_all(void) {
    asm(
        "li $8,0x80000000;"
        "li $9,0x80000000;"
        "addu $9,$9,0x1FF0;"
        "cacheloop:;"
        "cache 1,0($8);"
        "cache 1,16($8);"
        "cache 1,32($8);"
        "cache 1,48($8);"
        "cache 1,64($8);"
        "cache 1,80($8);"
        "cache 1,96($8);"
        "addu $8,$8,112;"
        "bne $8,$9,cacheloop;"
        "cache 1,0($8);"
    : // no outputs
    : // no inputs
    : "$8", "$9" // trashed registers
    );
}

void pi_initialize (void) {

	pi_dma_wait();
	ED_IO_WRITE(PI_STATUS_REG, 0x03);

}

void pi_dma_wait (void) {
	  
	while (ED_IO_READ(PI_STATUS_REG) & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY));
}

// Inits PI for sram transfer
void pi_initialize_sram (void) {

	ED_IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
	ED_IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0C);
	ED_IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0D);
	ED_IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);

}

void pi_dma_from_sram (void *dest, unsigned long offset, unsigned long size) {

	ED_IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
	ED_IO_WRITE(PI_CART_ADDR_REG, (0xA8000000 + offset));
	 asm volatile ("" : : : "memory");
	ED_IO_WRITE(PI_WR_LEN_REG, (size - 1));
	 asm volatile ("" : : : "memory");

}


void pi_dma_to_sram (void *src, unsigned long offset, unsigned long size) {

	pi_dma_wait();

	ED_IO_WRITE(PI_STATUS_REG, 2);
	ED_IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
	ED_IO_WRITE(PI_CART_ADDR_REG, (0xA8000000 + offset));
    _data_cache_invalidate_all();
	ED_IO_WRITE(PI_RD_LEN_REG, (size - 1));

}

void pi_dma_from_cart (void* dest, void* src, unsigned long size) {

	pi_dma_wait();

	ED_IO_WRITE(PI_STATUS_REG, 0x03);
	ED_IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(dest));
	ED_IO_WRITE(PI_CART_ADDR_REG, K0_TO_PHYS(src));
	ED_IO_WRITE(PI_WR_LEN_REG, (size - 1));

}


void pi_dma_to_cart (void* dest, void* src, unsigned long size) {

	pi_dma_wait();

	ED_IO_WRITE(PI_STATUS_REG, 0x02);
	ED_IO_WRITE(PI_DRAM_ADDR_REG, K1_TO_PHYS(src));
	ED_IO_WRITE(PI_CART_ADDR_REG, K0_TO_PHYS(dest));
	ED_IO_WRITE(PI_RD_LEN_REG, (size - 1));

}


// Wrapper to support unaligned access to memory
void pi_dma_from_cart_safe (void *dest, void *src, unsigned long size) {

	if (!dest || !src || !size) return;

	unsigned long unalignedSrc  = ((unsigned long)src)  % 2;
	unsigned long unalignedDest = ((unsigned long)dest) % 8;

	//FIXME: Do i really need to check if size is 16bit aligned?
	if (!unalignedDest && !unalignedSrc && !(size % 2)) {
		pi_dma_from_cart(dest, src, size);
		pi_dma_wait();

		return;
	}

	void* newSrc = (void*)(((unsigned long)src) - unalignedSrc);
	unsigned long newSize = (size + unalignedSrc) + ((size + unalignedSrc) % 2);

	unsigned char *buffer = memalign(8, newSize);
	pi_dma_from_cart(buffer, newSrc, newSize);
	pi_dma_wait();

	memcpy(dest, (buffer + unalignedSrc), size);

	free(buffer);

}


int ed64_ll_get_sram_128 (uint8_t *buffer, int size) {

    while (dma_busy()) ;

    ED_IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
    ED_IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0C);
    ED_IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0D);
    ED_IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);

    while (dma_busy()) ;

    pi_initialize();

    sleep(250);

    // Offset Large RAM size 128KiB with a 16KiB nudge to allocate enough space
    // We do this because 128Kib is not recognised and a 32KiB allocates too little 
    pi_dma_from_sram(buffer, -(size - KiB(16)), size);

    while (dma_busy()) ;

    ED_IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    ED_IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    ED_IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    ED_IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);

    return 1;

}


int ed64_ll_get_sram (uint8_t *buffer, int size) {

    while (dma_busy()) ;

    ED_IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
    ED_IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0C);
    ED_IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0D);
    ED_IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);

    while (dma_busy()) ;

    pi_initialize();

    sleep(250);

    pi_dma_from_sram(buffer, 0, size) ;

    while (dma_busy()) ;

    ED_IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    ED_IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    ED_IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    ED_IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);

    return 1;

}

int ed64_ll_get_eeprom (uint8_t *buffer, int size) {

    int blocks=size/8;
    for( int b = 0; b < blocks; b++ ) {
        eeprom_read( b, &buffer[b * 8] );
    }

    return 1;

}


int ed64_ll_get_fram (uint8_t *buffer, int size) {

    ed64_ll_set_save_type(SAVE_TYPE_SRAM_128K); //2
    sleep(512);

    ed64_ll_get_sram_128(buffer, size);
    data_cache_hit_writeback_invalidate(buffer, size);

    sleep(512);
    ed64_ll_set_save_type(SAVE_TYPE_FLASHRAM);

    return 1;

}

/*
sram upload
*/


int ed64_ll_set_sram_128 (uint8_t *buffer, int size) {

    //half working
    pi_dma_wait();
    //Timing
    pi_initialize_sram();

    //Readmode
    pi_initialize();

    data_cache_hit_writeback_invalidate(buffer,size);
    while (dma_busy()) ;

    // Offset Large RAM size 128KiB with a 16KiB nudge to allocate enough space
    // We do this because 128Kib is not recognised and a 32KiB allocates too little 
    pi_dma_to_sram(buffer, -(size - KiB(16)), size);
    data_cache_hit_writeback_invalidate(buffer,size);

    //Wait
     pi_dma_wait();
    //Restore evd Timing
    ed64_ll_set_sdcard_timing();

    return 1;

}


int ed64_ll_set_sram (uint8_t *buffer, int size) {

    //half working
     pi_dma_wait();
    //Timing
    pi_initialize_sram();

    //Readmode
    pi_initialize();

    data_cache_hit_writeback_invalidate(buffer,size);
    while (dma_busy()) ;

    pi_dma_to_sram(buffer, 0, size);
    data_cache_hit_writeback_invalidate(buffer,size);

    //Wait
     pi_dma_wait();
    //Restore evd Timing
    ed64_ll_set_sdcard_timing();

    return 1;

}


int ed64_ll_set_eeprom (uint8_t *buffer, int size) {

    int blocks=size/8;
    for( int b = 0; b < blocks; b++ ) {
        eeprom_write( b, &buffer[b * 8] );
    }

    return 1;

}

int ed64_ll_set_fram (uint8_t *buffer, int size) {

    ed64_ll_set_save_type(SAVE_TYPE_SRAM_128K);
    sleep(512);

    ed64_ll_set_sram_128(buffer, size);
    data_cache_hit_writeback_invalidate(buffer, size);

    sleep(512);
    ed64_ll_set_save_type(SAVE_TYPE_FLASHRAM);

    return 1;

}


void ed64_ll_set_sdcard_timing (void) {

    ED_IO_WRITE(PI_BSD_DOM1_LAT_REG, 0x40);
    ED_IO_WRITE(PI_BSD_DOM1_PWD_REG, 0x12);
    ED_IO_WRITE(PI_BSD_DOM1_PGS_REG, 0x07);
    ED_IO_WRITE(PI_BSD_DOM1_RLS_REG, 0x03);

    ED_IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x40);
    ED_IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x12);
    ED_IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x07);
    ED_IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x03);

}
