/**
 * @file ed64_vseries_ll.h
 * @brief ed64v flashcart low level access
 * @ingroup flashcart
 */

#ifndef FLASHCART_ED64_VSERIES_LL_H__
#define FLASHCART_ED64_VSERIES_LL_H__

#define ED_V_BASE_REG             0x08040000

#define ED_V_CFG_REG              (ED_V_BASE_REG+0x00)
#define ED_V_STATUS_REG           (ED_V_BASE_REG+0x04)
#define ED_V_DMA_LEN_REG          (ED_V_BASE_REG+0x08)
#define ED_V_DMA_ADDR_REG         (ED_V_BASE_REG+0x0C)
#define ED_V_MSG_REG              (ED_V_BASE_REG+0x10)
#define ED_V_DMA_CFG_REG          (ED_V_BASE_REG+0x14)
#define ED_V_SPI_REG              (ED_V_BASE_REG+0x18)
#define ED_V_SPI_CFG_REG          (ED_V_BASE_REG+0x1C)
#define ED_V_KEY_REG              (ED_V_BASE_REG+0x20)
#define ED_V_SAV_CFG_REG          (ED_V_BASE_REG+0x24)
#define ED_V_SEC_REG              (ED_V_BASE_REG+0x28)
#define ED_V_VER_REG              (ED_V_BASE_REG+0x2C)
//#define ED_V_GPIO_REG             (ED_V_BASE_REG+0x30)

#define ED_V_CFG_CNT_REG          (ED_V_BASE_REG+0x40)
#define ED_V_CFG_DAT_REG          (ED_V_BASE_REG+0x44)
#define ED_V_MAX_MSG_REG          (ED_V_BASE_REG+0x48)
#define ED_V_MAX_VER_REG          (ED_V_BASE_REG+0x4C)
// #define ED_V_3_FL_ADDR_REG        (ED_V_BASE_REG+0x50)
// #define ED_V_3_FL_DATA_REG        (ED_V_BASE_REG+0x54)

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
 * @brief Enable the RTC on ED64 V series v3 carts.
 */
void ed64_vseries_ll_v3_enable_rtc (void);


/** @} */ /* ed64_vseries_ll */


#endif
