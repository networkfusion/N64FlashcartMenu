/**
 * @file ed64_vseries_ll.h
 * @brief ed64v flashcart low level access
 * @ingroup flashcart
 */

#ifndef FLASHCART_ED64_VSERIES_LL_H__
#define FLASHCART_ED64_VSERIES_LL_H__

// #define ED_V_BASE_REG             0xA8040000 // Base address for ED64 V series registers should be 0x08040000 (non cached) ideally.


/** @brief CPLD Version Enumeration. */
typedef enum {
    CPLD_VERSION_3_0      = 0x3000, /**< Device variant 3 */
    CPLD_VERSION_2_5      = 0x2000, /**< Device variant 2.5 */
    CPLD_VERSION_2_0      = 0x0000, /**< Device variant 2 or below */
} ed64_vseries_cpld_version_t;

/** @brief Save Type Enumeration. */
typedef enum {
    SAVE_TYPE_NONE, /**< No save type */
    SAVE_TYPE_EEPROM_4KBIT, /**< EEPROM 4Kbit */
    SAVE_TYPE_EEPROM_16KBIT, /**< EEPROM 16Kbit */
    SAVE_TYPE_SRAM_256KBIT, /**< SRAM 256Kbit */
    SAVE_TYPE_SRAM_BANKED, /**< SRAM Banked */
    SAVE_TYPE_SRAM_1MBIT, /**< FlashRAM 1Mbit */
    SAVE_TYPE_FLASHRAM_1MBIT, /**< FlashRAM 1Mbit */
} ed64_vseries_save_type_t;


// Copy of libcart functions for low level access
/* EverDrive-64 functions */
#include <stdint.h>

/* Cartridge types */
#define CART_NULL       -1
#define CART_CI         0       /* 64Drive */
#define CART_EDX        1       /* EverDrive-64 X-series */
#define CART_ED         2       /* EverDrive-64 V1, V2, V2.5, V3 and ED64+ */
#define CART_SC         3       /* SummerCart64 */
#define CART_MAX        4


/* Size of cartridge SDRAM */
extern uint32_t cart_size;

/* Cartridge type */
extern int cart_type;

/* Detect cartridge and initialize it */
extern int cart_init(void);
/* Close the cartridge interface */
extern int cart_exit(void);

/* Swap high and low bytes per 16-bit word when reading into SDRAM */
extern char cart_card_byteswap;

/* Initialize card */
extern int cart_card_init(void);
/* Read sectors from card to system RDRAM */
extern int cart_card_rd_dram(void *dram, uint32_t lba, uint32_t count);
/* Read sectors from card to cartridge SDRAM */
extern int cart_card_rd_cart(uint32_t cart, uint32_t lba, uint32_t count);
/* Write sectors from system RDRAM to card */
extern int cart_card_wr_dram(const void *dram, uint32_t lba, uint32_t count);
/* Write sectors from cartridge SDRAM to card */
extern int cart_card_wr_cart(uint32_t cart, uint32_t lba, uint32_t count);

int ed_init(void);
int ed_exit(void);
int ed_card_init(void);
int ed_card_byteswap(int flag);
int ed_card_rd_dram(void *dram, uint32_t lba, uint32_t count);
int ed_card_rd_cart(uint32_t cart, uint32_t lba, uint32_t count);
int ed_card_wr_dram(const void *dram, uint32_t lba, uint32_t count);
int ed_card_wr_cart(uint32_t cart, uint32_t lba, uint32_t count);
// End copy of libcart functions

/**
 * @brief Get the ED64 V series cpld version.
 * 
 * @param cpld_version Pointer to store the cpld version.
 * @return true if successful, false otherwise.
 */
bool ed64_vseries_ll_get_cpld_version(uint16_t *cpld_version);

/**
 * @brief Get the ED64 V series fpga version.
 * 
 * @param fpga_version Pointer to store the FPGA version.
 * @return true if successful, false otherwise.
 */
bool ed64_vseries_ll_get_fpga_version(uint16_t *fpga_version);

/**
 * @brief Set the save type.
 * 
 * @param type The save type to set.
 * @param use_ram_bank Whether to use RAM banked mode.
 * @return true if successful, false otherwise.
 */
bool ed64_vseries_ll_set_save_type(ed64_vseries_save_type_t type, bool use_ram_bank);

/**
 * @brief Enable the GPIO on ED64 V series carts.
 */
void ed64_vseries_ll_enable_gpio(void);

/**
 * @brief Disable the GPIO on ED64 V series carts.
 */
void ed64_vseries_ll_disable_gpio(void);

/**
 * @brief Enable the RTC on ED64 V series v3 carts.
 */
void ed64_vseries_ll_v3_enable_rtc (void);


/**
 * @brief Update the firmware of the ED64 V series cart.
 * 
 * @param firmware_data Pointer to the firmware data.
 */
void ed64_vseries_ll_update_firmware(uint8_t *firmware_data);


/**
 * @brief Read a message from the ED64 V series cart.
 * 
 * @return The message data read.
 */
uint16_t ed64_vseries_ll_message_read(void);

/**
 * @brief Write a message to the ED64 V series cart.
 * 
 * @param data The message data to write.
 */
void ed64_vseries_ll_message_write(uint16_t data);

/**
 * @brief Enable the DD cart conversion RAM output enable.
 */
void ed64_vseries_ll_dd_cc_ram_oe(void);

/**
 * @brief Enable the DD cart conversion RAM write enable.
 */
void ed64_vseries_ll_dd_cc_ram_we(void);

/**
 * @brief Disable the DD cart conversion RAM.
 */
void ed64_vseries_ll_dd_cc_ram_off(void);

/**
 * @brief Check if DD cart conversion RAM is supported.
 * 
 * @return true if supported, false otherwise.
 */
bool ed64_vseries_ll_dd_ram_supported(void);

/** @} */ /* ed64_vseries_ll */


#endif
