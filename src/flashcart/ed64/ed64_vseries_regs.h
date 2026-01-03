// This is a clone of contents in libdragon/src/libcart/cart.c for ED64 V-series registers.
// we are unable to include that file directly here.
// Additions will be noted and should be ported back to libdragon.

# ifndef ED64V_REGS_H
# define ED64V_REGS_H

// #define ED_V_BASE_REG             0xA8040000 // ADDITION: Base address for ED64 V series registers should be 0x08040000 (non cached) ideally.
#define ED_BASE_REG             0x08040000

#define ED_CFG_REG              (ED_BASE_REG+0x00)
#define ED_STATUS_REG           (ED_BASE_REG+0x04)
#define ED_DMA_LEN_REG          (ED_BASE_REG+0x08)
#define ED_DMA_ADDR_REG         (ED_BASE_REG+0x0C)
#define ED_MSG_REG              (ED_BASE_REG+0x10)
#define ED_DMA_CFG_REG          (ED_BASE_REG+0x14)
#define ED_SPI_REG              (ED_BASE_REG+0x18)
#define ED_SPI_CFG_REG          (ED_BASE_REG+0x1C)
#define ED_KEY_REG              (ED_BASE_REG+0x20)
#define ED_SAV_CFG_REG          (ED_BASE_REG+0x24)
#define ED_SEC_REG              (ED_BASE_REG+0x28)
#define ED_VER_REG              (ED_BASE_REG+0x2C)

#define ED_CFG_CNT_REG          (ED_BASE_REG+0x40)
#define ED_CFG_DAT_REG          (ED_BASE_REG+0x44)
#define ED_MAX_MSG_REG          (ED_BASE_REG+0x48)
#define ED_CRC_REG              (ED_BASE_REG+0x4C)

// Additional Register definitions for ED64 V-Series not in libcart
#define ED_MAX_VER_REG           (ED_BASE_REG+0x4C) // Is this correct? overlaps with CRC_REG?
#define ED_V3_FLA_ADDR_REG       (ED_BASE_REG+0x50)
#define ED_V3_FLA_DATA_REG       (ED_BASE_REG+0x54)
// End Additional Register definitions


#define ED_DMA_SD_TO_RAM        1
#define ED_DMA_RAM_TO_SD        2
#define ED_DMA_FIFO_TO_RAM      3
#define ED_DMA_RAM_TO_FIFO      4

#define ED_CFG_SDRAM_OFF        (0 << 0)
#define ED_CFG_SDRAM_ON         (1 << 0)
#define ED_CFG_BYTESWAP         (1 << 1)
// Additional Register definitions for ED64 V-Series not in libcart:
#define ED_CFG_WR_MOD           (1 << 2) // 4
#define ED_CFG_WR_ADDR_MASK     (1 << 3) // 8
// 16 reserved
#define ED_CFG_MODE_RTC_OFF     (0 << 5)
#define ED_CFG_MODE_RTC_ON      (1 << 5) // 32
// 64 reserved
//#define ED_CFG_MODE_GPIO_ON     // 96 - this is strange... TODO: how to correctly use?
// 128 reserved
#define ED_CFG_DD_CC_ON         (1 << 8) // 256 // handle cart rom dd-cart conversion rom
#define ED_CFG_DD_CC_WE         (1 << 9) // 512
// End Additional Register definitions

#define ED_STATE_DMA_BUSY       (1 << 0)
#define ED_STATE_DMA_TOUT       (1 << 1)
#define ED_STATE_TXE            (1 << 2)
#define ED_STATE_RXF            (1 << 3)
#define ED_STATE_SPI            (1 << 4)

#define ED_SPI_SPD_50           (0 << 0)
#define ED_SPI_SPD_25           (1 << 0)
#define ED_SPI_SPD_LO           (2 << 0)
#define ED_SPI_SS               (1 << 2)
#define ED_SPI_WR               (0 << 3)
#define ED_SPI_RD               (1 << 3)
#define ED_SPI_CMD              (0 << 4)
#define ED_SPI_DAT              (1 << 4)
#define ED_SPI_8BIT             (0 << 5)
#define ED_SPI_1BIT             (1 << 5)

// Includeds improved Register definitions for ED64 V-Series not in libcart
#define ED_SAV_EEP_OFF          (0 << 0)
#define ED_SAV_EEP_ON           (1 << 0)
#define ED_SAV_SRM_OFF          (0 << 1)
#define ED_SAV_SRM_ON           (1 << 1)
#define ED_SAV_EEP_SMALL        (0 << 2)
#define ED_SAV_EEP_LARGE        (1 << 2)
#define ED_SAV_SRM_SMALL        (0 << 3)
#define ED_SAV_SRM_LARGE        (1 << 3)
// Additional Register definitions for ED64 V-Series not in libcart
#define ED_SAV_RAM_BANK_ON      (1 << 7)
#define ED_SAV_RAM_BANK_APPLY   (1 << 15)
// End Additional Register definitions

#define ED_KEY                  0x1234

#define ED_SD_CMD_RD            (ED_SPI_CMD|ED_SPI_RD)
#define ED_SD_CMD_WR            (ED_SPI_CMD|ED_SPI_WR)
#define ED_SD_DAT_RD            (ED_SPI_DAT|ED_SPI_RD)
#define ED_SD_DAT_WR            (ED_SPI_DAT|ED_SPI_WR)

#define ED_SD_CMD_8b            ED_SPI_8BIT
#define ED_SD_CMD_1b            ED_SPI_1BIT
#define ED_SD_DAT_8b            ED_SPI_8BIT
#define ED_SD_DAT_1b            ED_SPI_1BIT

# endif /* ED64V_REGS_H */
