/**
 * @file ed64_vseries_ll.h
 * @brief ed64v flashcart low level access
 * @ingroup flashcart
 */

#ifndef FLASHCART_ED64_VSERIES_LL_H__
#define FLASHCART_ED64_VSERIES_LL_H__

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
