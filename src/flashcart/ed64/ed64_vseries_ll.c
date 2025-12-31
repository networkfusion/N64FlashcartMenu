/**
 * @file ed64_vseries_ll.c
 * @brief Low-level functions for ED64 Vseries
 * @ingroup flashcart
 */

#include <libdragon.h>

#include "../flashcart_utils.h"
#include "ed64_vseries_ll.h"


// Copy of libcart as prefered to depend on it directly here

#define MI_BASE_REG             0x04300000
#define MI_VERSION_REG          (MI_BASE_REG+0x04)
#define PI_BASE_REG             0x04600000
#define PI_BSD_DOM1_LAT_REG     (PI_BASE_REG+0x14)
#define PI_BSD_DOM1_PWD_REG     (PI_BASE_REG+0x18)
#define PI_BSD_DOM1_PGS_REG     (PI_BASE_REG+0x1C)
#define PI_BSD_DOM1_RLS_REG     (PI_BASE_REG+0x20)
#define PI_BSD_DOM2_LAT_REG     (PI_BASE_REG+0x24)
#define PI_BSD_DOM2_PWD_REG     (PI_BASE_REG+0x28)
#define PI_BSD_DOM2_PGS_REG     (PI_BASE_REG+0x2C)
#define PI_BSD_DOM2_RLS_REG     (PI_BASE_REG+0x30)

#define IO_READ(addr)       (*(volatile uint32_t *)PHYS_TO_K1(addr))
#define IO_WRITE(addr,data) \
        (*(volatile uint32_t *)PHYS_TO_K1(addr) = (uint32_t)(data))

#define PHYS_TO_K1(x)   ((uint32_t)(x)|0xA0000000)

#define CART_ABORT()            {__cart_acs_rel(); return -1;}

/* Temporary buffer aligned for DMA */
__attribute__((aligned(16))) static uint64_t __cart_buf[512/8];

static uint32_t __cart_dom1_rel;
static uint32_t __cart_dom2_rel;
static uint32_t __cart_dom1;
static uint32_t __cart_dom2;

uint32_t cart_size;

static void __cart_acs_get(void)
{
    /* Save PI BSD configuration and reconfigure */
    if (__cart_dom1)
    {
        __cart_dom1_rel =
            IO_READ(PI_BSD_DOM1_LAT_REG) <<  0 |
            IO_READ(PI_BSD_DOM1_PWD_REG) <<  8 |
            IO_READ(PI_BSD_DOM1_PGS_REG) << 16 |
            IO_READ(PI_BSD_DOM1_RLS_REG) << 20 |
            1 << 31;
        IO_WRITE(PI_BSD_DOM1_LAT_REG, __cart_dom1 >>  0);
        IO_WRITE(PI_BSD_DOM1_PWD_REG, __cart_dom1 >>  8);
        IO_WRITE(PI_BSD_DOM1_PGS_REG, __cart_dom1 >> 16);
        IO_WRITE(PI_BSD_DOM1_RLS_REG, __cart_dom1 >> 20);
    }
    if (__cart_dom2)
    {
        __cart_dom2_rel =
            IO_READ(PI_BSD_DOM2_LAT_REG) <<  0 |
            IO_READ(PI_BSD_DOM2_PWD_REG) <<  8 |
            IO_READ(PI_BSD_DOM2_PGS_REG) << 16 |
            IO_READ(PI_BSD_DOM2_RLS_REG) << 20 |
            1 << 31;
        IO_WRITE(PI_BSD_DOM2_LAT_REG, __cart_dom2 >>  0);
        IO_WRITE(PI_BSD_DOM2_PWD_REG, __cart_dom2 >>  8);
        IO_WRITE(PI_BSD_DOM2_PGS_REG, __cart_dom2 >> 16);
        IO_WRITE(PI_BSD_DOM2_RLS_REG, __cart_dom2 >> 20);
    }
}

static void __cart_acs_rel(void)
{
    /* Restore PI BSD configuration */
    if (__cart_dom1_rel)
    {
        IO_WRITE(PI_BSD_DOM1_LAT_REG, __cart_dom1_rel >>  0);
        IO_WRITE(PI_BSD_DOM1_PWD_REG, __cart_dom1_rel >>  8);
        IO_WRITE(PI_BSD_DOM1_PGS_REG, __cart_dom1_rel >> 16);
        IO_WRITE(PI_BSD_DOM1_RLS_REG, __cart_dom1_rel >> 20);
        __cart_dom1_rel = 0;
    }
    if (__cart_dom2_rel)
    {
        IO_WRITE(PI_BSD_DOM2_LAT_REG, __cart_dom2_rel >>  0);
        IO_WRITE(PI_BSD_DOM2_PWD_REG, __cart_dom2_rel >>  8);
        IO_WRITE(PI_BSD_DOM2_PGS_REG, __cart_dom2_rel >> 16);
        IO_WRITE(PI_BSD_DOM2_RLS_REG, __cart_dom2_rel >> 20);
        __cart_dom2_rel = 0;
    }
}

static void __cart_dma_rd(void *dram, uint32_t cart, uint32_t size)
{
    data_cache_hit_writeback_invalidate(dram, size);
    dma_read_raw_async(dram, cart, size);
    dma_wait();
}

static void __cart_dma_wr(const void *dram, uint32_t cart, uint32_t size)
{
    data_cache_hit_writeback((void *)dram, size);
    dma_write_raw_async(dram, cart, size);
    dma_wait();
}

static void __cart_buf_rd(const void *addr)
{
    int i;
    const u_uint64_t *ptr = addr;
    for (i = 0; i < 512/8; i += 2)
    {
        uint64_t a = ptr[i+0];
        uint64_t b = ptr[i+1];
        __cart_buf[i+0] = a;
        __cart_buf[i+1] = b;
    }
}

static void __cart_buf_wr(void *addr)
{
    int i;
    u_uint64_t *ptr = addr;
    for (i = 0; i < 512/8; i += 2)
    {
        uint64_t a = __cart_buf[i+0];
        uint64_t b = __cart_buf[i+1];
        ptr[i+0] = a;
        ptr[i+1] = b;
    }
}

#define CMD0    (0x40| 0)
#define CMD1    (0x40| 1)
#define CMD2    (0x40| 2)
#define CMD3    (0x40| 3)
#define CMD7    (0x40| 7)
#define CMD8    (0x40| 8)
#define CMD9    (0x40| 9)
#define CMD12   (0x40|12)
#define CMD18   (0x40|18)
#define CMD25   (0x40|25)
#define CMD55   (0x40|55)
#define CMD58   (0x40|58)
#define ACMD6   (0x40| 6)
#define ACMD41  (0x40|41)

static unsigned char __sd_resp[17];
static unsigned char __sd_cfg;
static unsigned char __sd_type;
static unsigned char __sd_flag;

static int __sd_crc7(const char *src)
{
    int i;
    int n;
    int crc = 0;
    for (i = 0; i < 5; i++)
    {
        crc ^= src[i];
        for (n = 0; n < 8; n++)
        {
            if ((crc <<= 1) & 0x100) crc ^= 0x12;
        }
    }
    return (crc & 0xFE) | 1;
}

/* Thanks to anacierdem for this brilliant implementation. */

/* Spread lower 32 bits into 64 bits */
/* x =     **** **** **** **** abcd efgh ijkl mnop */
/* result: a0b0 c0d0 e0f0 g0h0 i0j0 k0l0 m0n0 o0p0 */
static uint64_t __sd_crc16_spread(uint64_t x)
{
    x = (x << 16 | x) & 0x0000FFFF0000FFFF;
    x = (x <<  8 | x) & 0x00FF00FF00FF00FF;
    x = (x <<  4 | x) & 0x0F0F0F0F0F0F0F0F;
    x = (x <<  2 | x) & 0x3333333333333333;
    x = (x <<  1 | x) & 0x5555555555555555;
    return x;
}

/* Shuffle 32 bits of two values into 64 bits */
/* x =     **** **** **** **** abcd efgh ijkl mnop */
/* y =     **** **** **** **** ABCD EFGH IJKL MNOP */
/* result: aAbB cCdD eEfF gGhH iIjJ kKlL mMnN oOpP */
static uint64_t __sd_crc16_shuffle(uint32_t x, uint32_t y)
{
    return __sd_crc16_spread(x) << 1 | __sd_crc16_spread(y);
}

static void __sd_crc16(uint64_t *dst, const uint64_t *src)
{
    int i;
    int n;
    uint64_t x;
    uint64_t y;
    uint32_t a;
    uint32_t b;
    uint16_t crc[4] = {0};
    for (i = 0; i < 512/8; i++)
    {
        x = src[i];
        /* Transpose every 2x2 bit block in the 8x8 matrix */
        /* abcd efgh     aick emgo */
        /* ijkl mnop     bjdl fnhp */
        /* qrst uvwx     qys0 u2w4 */
        /* yz01 2345  \  rzt1 v3x5 */
        /* 6789 ABCD  /  6E8G AICK */
        /* EFGH IJKL     7F9H BJDL */
        /* MNOP QRST     MUOW QYS? */
        /* UVWX YZ?!     NVPX RZT! */
        y = (x ^ (x >> 7)) & 0x00AA00AA00AA00AA;
        x ^= y ^ (y << 7);
        /* Transpose 2x2 blocks inside their 4x4 blocks in the 8x8 matrix */
        /* aick emgo     aiqy emu2 */
        /* bjdl fnhp     bjrz fnv3 */
        /* qys0 u2w4     cks0 gow4 */
        /* rzt1 v3x5  \  dlt1 hpx5 */
        /* 6E8G AICK  /  6EMU AIQY */
        /* 7F9H BJDL     7FNV BJRZ */
        /* MUOW QYS?     8GOW CKS? */
        /* NVPX RZT!     9HPX DLT! */
        y = (x ^ (x >> 14)) & 0x0000CCCC0000CCCC;
        x ^= y ^ (y << 14);
        /* Interleave */
        /* x =     aiqy 6EMU bjrz 7FNV cks0 8GOW dlt1 9HPX */
        /* y =     emu2 AIQY fnv3 BJRZ gow4 CKS? hpx5 DLT! */
        /* result: aeim quy2 6AEI MQUY bfjn rvz3 7BFJ NRVZ */
        /*         cgko sw04 8CGK OSW? dhlp tx15 9DHL PTX! */
        x = __sd_crc16_shuffle(
            (x >> 32 & 0xF0F0F0F0) | (x >> 4 & 0x0F0F0F0F),
            (x >> 28 & 0xF0F0F0F0) | (x >> 0 & 0x0F0F0F0F)
        );
        for (n = 3; n >= 0; n--)
        {
            a = crc[n];
            /* (crc >> 8) ^ dat[0] */
            b = ((x ^ a) >> 8) & 0xFF;
            b ^= b >> 4;
            a = (a << 8) ^ b ^ (b << 5) ^ (b << 12);
            /* (crc >> 8) ^ dat[1] */
            b = (x ^ (a >> 8)) & 0xFF;
            b ^= b >> 4;
            a = (a << 8) ^ b ^ (b << 5) ^ (b << 12);
            crc[n] = a;
            x >>= 16;
        }
    }
    /* Interleave CRC */
    x = __sd_crc16_shuffle(crc[0] << 16 | crc[1], crc[2] << 16 | crc[3]);
    *dst = __sd_crc16_shuffle(x >> 32, x);
}

int cart_type = CART_NULL;

int cart_init(void)
{
    static int (*const init[CART_MAX])(void) =
    {
        ed_init,
    };
    int i, result;
    /* bbplayer */
    if ((IO_READ(MI_VERSION_REG) & 0xF0) == 0xB0) return -1;
    if (!__cart_dom1)
    {
        __cart_dom1 = 0x8030FFFF;
        __cart_acs_get();
        __cart_dom1 = io_read(0x10000000);
        __cart_acs_rel();
    }
    if (!__cart_dom2) __cart_dom2 = __cart_dom1;
    if (cart_type < 0)
    {
        for (i = 0; i < CART_MAX; i++)
        {
            if ((result = init[i]()) >= 0)
            {
                cart_type = i;
                return result;
            }
        }
        return -1;
    }
    return init[cart_type]();
}

int cart_exit(void)
{
    static int (*const exit[CART_MAX])(void) =
    {
        ed_exit,
    };
    if (cart_type < 0) return -1;
    return exit[cart_type]();
}

int cart_card_init(void)
{
    static int (*const card_init[CART_MAX])(void) =
    {
        ed_card_init,
    };
    if (cart_type < 0) return -1;
    return card_init[cart_type]();
}

int cart_card_rd_dram(void *dram, uint32_t lba, uint32_t count)
{
    static int (*const card_rd_dram[CART_MAX])(
        void *dram, uint32_t lba, uint32_t count
    ) =
    {
        ed_card_rd_dram,
    };
    if (cart_type < 0) return -1;
    return card_rd_dram[cart_type](dram, lba, count);
}

char cart_card_byteswap;

int cart_card_rd_cart(uint32_t cart, uint32_t lba, uint32_t count)
{
    static int (*const card_rd_cart[CART_MAX])(
        uint32_t cart, uint32_t lba, uint32_t count
    ) =
    {
        ed_card_rd_cart,
    };
    if (cart_type < 0) return -1;
    return card_rd_cart[cart_type](cart, lba, count);
}

int cart_card_wr_dram(const void *dram, uint32_t lba, uint32_t count)
{
    static int (*const card_wr_dram[CART_MAX])(
        const void *dram, uint32_t lba, uint32_t count
    ) =
    {
        ed_card_wr_dram,
    };
    if (cart_type < 0) return -1;
    return card_wr_dram[cart_type](dram, lba, count);
}

int cart_card_wr_cart(uint32_t cart, uint32_t lba, uint32_t count)
{
    static int (*const card_wr_cart[CART_MAX])(
        uint32_t cart, uint32_t lba, uint32_t count
    ) =
    {
        ed_card_wr_cart,
    };
    if (cart_type < 0) return -1;
    return card_wr_cart[cart_type](cart, lba, count);
}

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

#define ED_DMA_SD_TO_RAM        1
#define ED_DMA_RAM_TO_SD        2
#define ED_DMA_FIFO_TO_RAM      3
#define ED_DMA_RAM_TO_FIFO      4

#define ED_CFG_SDRAM_OFF        (0 << 0)
#define ED_CFG_SDRAM_ON         (1 << 0)
#define ED_CFG_BYTESWAP         (1 << 1)

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

#define ED_SAV_EEP_ON           (1 << 0)
#define ED_SAV_SRM_ON           (1 << 1)
#define ED_SAV_EEP_SIZE         (1 << 2)
#define ED_SAV_SRM_SIZE         (1 << 3)

#define ED_KEY                  0x1234

#define ED_SD_CMD_RD            (ED_SPI_CMD|ED_SPI_RD)
#define ED_SD_CMD_WR            (ED_SPI_CMD|ED_SPI_WR)
#define ED_SD_DAT_RD            (ED_SPI_DAT|ED_SPI_RD)
#define ED_SD_DAT_WR            (ED_SPI_DAT|ED_SPI_WR)

#define ED_SD_CMD_8b            ED_SPI_8BIT
#define ED_SD_CMD_1b            ED_SPI_1BIT
#define ED_SD_DAT_8b            ED_SPI_8BIT
#define ED_SD_DAT_1b            ED_SPI_1BIT

#define __ed_sd_mode(reg, val)  io_write(ED_SPI_CFG_REG, __sd_cfg|(reg)|(val))
#define __ed_sd_cmd_rd(val)     __ed_spi((val) & 0xFF)
#define __ed_sd_cmd_wr(val)     __ed_spi((val) & 0xFF)
#define __ed_sd_dat_rd()        __ed_spi(0xFF)
#define __ed_sd_dat_wr(val)     __ed_spi((val) & 0xFF)

int ed_init(void)
{
    uint32_t ver;
    __cart_acs_get();
    io_write(ED_KEY_REG, ED_KEY);
    ver = io_read(ED_VER_REG) & 0xFFFF;
    if (ver < 0x100 || ver >= 0x400) CART_ABORT();
    io_write(ED_CFG_REG, ED_CFG_SDRAM_ON);
    __cart_dom2 = 0x80370404;
    /* V1/V2/V2.5 do not have physical SRAM on board */
    /* The end of SDRAM is used for SRAM or FlashRAM save types */
    if (ver < 0x300)
    {
        uint32_t sav = io_read(ED_SAV_CFG_REG);
        /* Have 1M SRAM or FlashRAM */
        if (sav & ED_SAV_SRM_SIZE)
        {
            cart_size = 0x3FE0000; /* 64 MiB - 128 KiB */
        }
        /* Have 256K SRAM */
        else if (sav & ED_SAV_SRM_ON)
        {
            cart_size = 0x3FF8000; /* 64 MiB - 32KiB */
        }
        else
        {
            cart_size = 0x4000000; /* 64 MiB */
        }
    }
    __cart_acs_rel();
    return 0;
}

int ed_exit(void)
{
    __cart_acs_get();
    io_write(ED_KEY_REG, 0);
    __cart_acs_rel();
    return 0;
}

/* SPI exchange */
static int __ed_spi(int val)
{
    io_write(ED_SPI_REG, val);
    while (io_read(ED_STATUS_REG) & ED_STATE_SPI);
    return io_read(ED_SPI_REG);
}

static int __ed_sd_cmd(int cmd, uint32_t arg)
{
    int i;
    int n;
    char buf[6];
    buf[0] = cmd;
    buf[1] = arg >> 24;
    buf[2] = arg >> 16;
    buf[3] = arg >>  8;
    buf[4] = arg >>  0;
    buf[5] = __sd_crc7(buf);
    /* Send the command */
    __ed_sd_mode(ED_SD_CMD_WR, ED_SD_CMD_8b);
    __ed_sd_cmd_wr(0xFF);
    for (i = 0; i < 6; i++) __ed_sd_cmd_wr(buf[i]);
    /* Read the first response byte */
    __sd_resp[0] = 0xFF;
    __ed_sd_mode(ED_SD_CMD_RD, ED_SD_CMD_1b);
    n = 2048;
    while (__sd_resp[0] & 0xC0)
    {
        if (--n == 0) return -1;
        __sd_resp[0] = __ed_sd_cmd_rd(__sd_resp[0]);
    }
    /* Read the rest of the response */
    n = !__sd_type ?
        cmd == CMD8 || cmd == CMD58 ? 5 : 1 :
        cmd == CMD2 || cmd == CMD9 ? 17 : 6;
    __ed_sd_mode(ED_SD_CMD_RD, ED_SD_CMD_8b);
    for (i = 1; i < n; i++) __sd_resp[i] = __ed_sd_cmd_rd(0xFF);
    /* SPI: return "illegal command" flag */
    return !__sd_type ? (__sd_resp[0] & 4) : 0;
}

static int __ed_sd_close(int flag)
{
    int n;
    if (!flag)
    {
        /* SPI: Stop token (write) */
        __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_8b);
        __ed_sd_dat_wr(0xFD);
        __ed_sd_dat_wr(0xFF);
    }
    else
    {
        /* CMD12: STOP_TRANSMISSION */
        if (__ed_sd_cmd(CMD12, 0) < 0) return -1;
    }
    /* Wait for card */
    __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_8b);
    n = 65536;
    do
    {
        if (--n == 0) break;
    }
    while ((__ed_sd_dat_rd() & 0xFF) != 0xFF);
    return 0;
}

int ed_card_init(void)
{
    int i;
    int n;
    uint32_t rca;
    __cart_acs_get();
    /* Detect SD interface */
    /* 0: use SPI */
    /* 1: use SD */
    __sd_type = 0;
    if ((io_read(ED_VER_REG) & 0xFFFF) >= 0x116)
    {
        /* Check bootloader ROM label for "ED64 SD boot" */
        io_write(ED_CFG_REG, ED_CFG_SDRAM_OFF);
        /* label[4:8] == " SD " */
        if (io_read(0x10000024) == 0x20534420) __sd_type = 1;
        io_write(ED_CFG_REG, ED_CFG_SDRAM_ON);
    }
    /* SPI: SS = 0 */
    /* SD : SS = 1 */
    __sd_cfg = ED_SPI_SPD_LO;
    if (__sd_type) __sd_cfg |= ED_SPI_SS;
    /* Card needs 74 clocks, we do 80 */
    __ed_sd_mode(ED_SD_CMD_WR, ED_SD_CMD_8b);
    for (i = 0; i < 10; i++) __ed_sd_cmd_wr(0xFF);
    /* CMD0: GO_IDLE_STATE */
    __ed_sd_cmd(CMD0, 0);
    /* CMD8: SEND_IF_COND */
    /* If it returns an error, it is SD V1 */
    if (__ed_sd_cmd(CMD8, 0x1AA))
    {
        /* SD V1 */
        if (!__sd_type)
        {
            if (__ed_sd_cmd(CMD55, 0) < 0) CART_ABORT();
            if (__ed_sd_cmd(ACMD41, 0x40300000) < 0)
            {
                n = 1024;
                do
                {
                    if (--n == 0) CART_ABORT();
                    if (__ed_sd_cmd(CMD1, 0) < 0) CART_ABORT();
                }
                while (__sd_resp[0] != 0);
            }
            else
            {
                n = 1024;
                do
                {
                    if (--n == 0) CART_ABORT();
                    if (__ed_sd_cmd(CMD55, 0) < 0) CART_ABORT();
                    if (__sd_resp[0] != 1) continue;
                    if (__ed_sd_cmd(ACMD41, 0x40300000) < 0) CART_ABORT();
                }
                while (__sd_resp[0] != 0);
            }
        }
        else
        {
            n = 1024;
            do
            {
                if (--n == 0) CART_ABORT();
                if (__ed_sd_cmd(CMD55, 0) < 0) CART_ABORT();
                if (__ed_sd_cmd(ACMD41, 0x40300000) < 0) CART_ABORT();
            }
            while (__sd_resp[1] == 0);
        }
        __sd_flag = 0;
    }
    else
    {
        /* SD V2 */
        if (!__sd_type)
        {
            n = 1024;
            do
            {
                if (--n == 0) CART_ABORT();
                if (__ed_sd_cmd(CMD55, 0) < 0) CART_ABORT();
                if (__sd_resp[0] != 1) continue;
                if (__ed_sd_cmd(ACMD41, 0x40300000) < 0) CART_ABORT();
            }
            while (__sd_resp[0] != 0);
            if (__ed_sd_cmd(CMD58, 0) < 0) CART_ABORT();
        }
        else
        {
            n = 1024;
            do
            {
                if (--n == 0) CART_ABORT();
                if (__ed_sd_cmd(CMD55, 0) < 0) CART_ABORT();
                if (!(__sd_resp[3] & 1)) continue;
                __ed_sd_cmd(ACMD41, 0x40300000);
            }
            while (!(__sd_resp[1] & 0x80));
        }
        /* Card is SDHC */
        __sd_flag = __sd_resp[1] & 0x40;
    }
    if (!__sd_type)
    {
        __sd_cfg = ED_SPI_SPD_25;
    }
    else
    {
        /* CMD2: ALL_SEND_CID */
        if (__ed_sd_cmd(CMD2, 0) < 0) CART_ABORT();
        /* CMD3: SEND_RELATIVE_ADDR */
        if (__ed_sd_cmd(CMD3, 0) < 0) CART_ABORT();
        rca =
            __sd_resp[1] << 24 |
            __sd_resp[2] << 16 |
            __sd_resp[3] <<  8 |
            __sd_resp[4] <<  0;
        /* CMD9: SEND_CSD */
        if (__ed_sd_cmd(CMD9, rca) < 0) CART_ABORT();
        /* CMD7: SELECT_CARD */
        if (__ed_sd_cmd(CMD7, rca) < 0) CART_ABORT();
        /* ACMD6: SET_BUS_WIDTH */
        if (__ed_sd_cmd(CMD55, rca) < 0) CART_ABORT();
        if (__ed_sd_cmd(ACMD6, 2) < 0) CART_ABORT();
        __sd_cfg = ED_SPI_SPD_50|ED_SPI_SS;
    }
    __cart_acs_rel();
    return 0;
}

int ed_card_rd_dram(void *dram, uint32_t lba, uint32_t count)
{
    char *addr = dram;
    int i;
    int n;
    __cart_acs_get();
    /* SDSC takes byte address, SDHC takes LBA */
    if (!__sd_flag) lba *= 512;
    /* CMD18: READ_MULTIPLE_BLOCK */
    if (__ed_sd_cmd(CMD18, lba) < 0) CART_ABORT();
    while (count-- > 0)
    {
        /* Wait for card */
        __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_1b);
        n = 65536;
        do
        {
            if (--n == 0) CART_ABORT();
        }
        while (__ed_sd_dat_rd() & 1);
        /* Read data */
        __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_8b);
        for (i = 0; i < 512; i++) addr[i] = __ed_sd_dat_rd();
        /* SPI: 1x16-bit CRC (2 byte) */
        /* SD:  4x16-bit CRC (8 byte) */
        /* We ignore the CRC */
        n = !__sd_type ? 2 : 8;
        for (i = 0; i < n; i++) __ed_sd_dat_rd();
        addr += 512;
    }
    if (__ed_sd_close(1)) CART_ABORT();
    __cart_acs_rel();
    return 0;
}

int ed_card_rd_cart(uint32_t cart, uint32_t lba, uint32_t count)
{
    int i;
    int n;
    uint32_t resp;
    __cart_acs_get();
    /* SDSC takes byte address, SDHC takes LBA */
    if (!__sd_flag) lba *= 512;
    /* CMD18: READ_MULTIPLE_BLOCK */
    if (__ed_sd_cmd(CMD18, lba) < 0) CART_ABORT();
    /* DMA requires 2048-byte alignment */
    if (cart & 0x7FF)
    {
        while (count-- > 0)
        {
            /* Wait for card */
            __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_1b);
            n = 65536;
            do
            {
                if (--n == 0) CART_ABORT();
            }
            while (__ed_sd_dat_rd() & 1);
            /* Read data */
            __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_8b);
            for (i = 0; i < 512; i++)
            {
                ((char *)__cart_buf)[i] = __ed_sd_dat_rd();
            }
            /* SPI: 1x16-bit CRC (2 byte) */
            /* SD:  4x16-bit CRC (8 byte) */
            /* We ignore the CRC */
            n = !__sd_type ? 2 : 8;
            for (i = 0; i < n; i++) __ed_sd_dat_rd();
            __cart_dma_wr(__cart_buf, cart, 512);
            cart += 512;
        }
    }
    else
    {
        if (cart_card_byteswap)
        {
            io_write(ED_CFG_REG, ED_CFG_SDRAM_ON|ED_CFG_BYTESWAP);
        }
        __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_8b);
        io_write(ED_DMA_LEN_REG, count-1);
        io_write(ED_DMA_ADDR_REG, (cart & 0x3FFFFFF) >> 11);
        io_write(ED_DMA_CFG_REG, ED_DMA_SD_TO_RAM);
        while ((resp = io_read(ED_STATUS_REG)) & ED_STATE_DMA_BUSY)
        {
            if (resp & ED_STATE_DMA_TOUT)
            {
                io_write(ED_CFG_REG, ED_CFG_SDRAM_ON);
                CART_ABORT();
            }
        }
        if (cart_card_byteswap)
        {
            io_write(ED_CFG_REG, ED_CFG_SDRAM_ON);
        }
    }
    if (__ed_sd_close(1)) CART_ABORT();
    __cart_acs_rel();
    return 0;
}

int ed_card_wr_dram(const void *dram, uint32_t lba, uint32_t count)
{
    const char *addr = dram;
    int i;
    int n;
    int resp;
    __cart_acs_get();
    /* SDSC takes byte address, SDHC takes LBA */
    if (!__sd_flag) lba *= 512;
    /* CMD25: WRITE_MULTIPLE_BLOCK */
    if (__ed_sd_cmd(CMD25, lba) < 0) CART_ABORT();
    if (!__sd_type)
    {
        /* SPI: padding (why 2 bytes?) */
        __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_8b);
        __ed_sd_dat_wr(0xFF);
        __ed_sd_dat_wr(0xFF);
    }
    while (count-- > 0)
    {
        __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_8b);
        if (!__sd_type)
        {
            /* SPI: data token */
            __ed_sd_dat_wr(0xFC);
        }
        else
        {
            /* SD: start bit (why not only write F0?) */
            __ed_sd_dat_wr(0xFF);
            __ed_sd_dat_wr(0xF0);
        }
        /* Write data */
        for (i = 0; i < 512; i++) __ed_sd_dat_wr(addr[i]);
        if (!__sd_type)
        {
            /* SPI: write dummy CRC */
            for (i = 0; i < 2; i++) __ed_sd_dat_wr(0xFF);
        }
        else
        {
            /* SD: write real CRC */
            if ((long)addr & 7)
            {
                __cart_buf_rd(addr);
                __sd_crc16(__cart_buf, __cart_buf);
            }
            else
            {
                __sd_crc16(__cart_buf, (const uint64_t *)addr);
            }
            for (i = 0; i < 8; i++) __ed_sd_dat_wr(((char *)__cart_buf)[i]);
            /* End bit */
            __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_1b);
            __ed_sd_dat_wr(0xFF);
            /* Wait for start of response */
            __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_1b);
            n = 1024;
            do
            {
                if (--n == 0) CART_ABORT();
            }
            while (__ed_sd_dat_rd() & 1);
            /* Read response */
            resp = 0;
            for (i = 0; i < 3; i++) resp = resp << 1 | (__ed_sd_dat_rd() & 1);
            if (resp != 2) CART_ABORT();
        }
        /* Wait for card */
        __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_8b);
        n = 65536;
        do
        {
            if (--n == 0) CART_ABORT();
        }
        while ((__ed_sd_dat_rd() & 0xFF) != 0xFF);
        addr += 512;
    }
    if (__ed_sd_close(__sd_type)) CART_ABORT();
    __cart_acs_rel();
    return 0;
}

int ed_card_wr_cart(uint32_t cart, uint32_t lba, uint32_t count)
{
    int i;
    int n;
    int resp;
    __cart_acs_get();
    /* SDSC takes byte address, SDHC takes LBA */
    if (!__sd_flag) lba *= 512;
    /* CMD25: WRITE_MULTIPLE_BLOCK */
    if (__ed_sd_cmd(CMD25, lba) < 0) CART_ABORT();
    if (!__sd_type)
    {
        /* SPI: padding (why 2 bytes?) */
        __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_8b);
        __ed_sd_dat_wr(0xFF);
        __ed_sd_dat_wr(0xFF);
    }
    while (count-- > 0)
    {
        __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_8b);
        if (!__sd_type)
        {
            /* SPI: data token */
            __ed_sd_dat_wr(0xFC);
        }
        else
        {
            /* SD: start bit (why not only write F0?) */
            __ed_sd_dat_wr(0xFF);
            __ed_sd_dat_wr(0xF0);
        }
        __cart_dma_rd(__cart_buf, cart, 512);
        /* Write data */
        for (i = 0; i < 512; i++) __ed_sd_dat_wr(((char *)__cart_buf)[i]);
        if (!__sd_type)
        {
            /* SPI: write dummy CRC */
            for (i = 0; i < 2; i++) __ed_sd_dat_wr(0xFF);
        }
        else
        {
            /* SD: write real CRC */
            __sd_crc16(__cart_buf, __cart_buf);
            for (i = 0; i < 8; i++) __ed_sd_dat_wr(((char *)__cart_buf)[i]);
            /* End bit */
            __ed_sd_mode(ED_SD_DAT_WR, ED_SD_DAT_1b);
            __ed_sd_dat_wr(0xFF);
            /* Wait for start of response */
            __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_1b);
            n = 1024;
            do
            {
                if (--n == 0) CART_ABORT();
            }
            while (__ed_sd_dat_rd() & 1);
            /* Read response */
            resp = 0;
            for (i = 0; i < 3; i++) resp = resp << 1 | (__ed_sd_dat_rd() & 1);
            if (resp != 2) CART_ABORT();
        }
        /* Wait for card */
        __ed_sd_mode(ED_SD_DAT_RD, ED_SD_DAT_8b);
        n = 65536;
        do
        {
            if (--n == 0) CART_ABORT();
        }
        while ((__ed_sd_dat_rd() & 0xFF) != 0xFF);
        cart += 512;
    }
    if (__ed_sd_close(__sd_type)) CART_ABORT();
    __cart_acs_rel();
    return 0;
}

// end of libcart copy

// Register definitions for ED64 V-Series not in libcart
#define ED_MAX_VER_REG           (ED_BASE_REG+0x4C)
#define ED_V3_FLA_ADDR_REG       (ED_BASE_REG+0x50)
#define ED_V3_FLA_DATA_REG       (ED_BASE_REG+0x54)

// #define ED_DMA_SD_TO_RAM 1
// #define ED_DMA_RAM_TO_SD 2
// #define ED_DMA_FIFO_TO_RAM 3 //USB
// #define ED_DMA_RAM_TO_FIFO 4 //USB

// #define ED_SAV_EEP_ON           (1 << 0)
// #define ED_SAV_SRM_ON           (1 << 1)
// #define ED_SAV_EEP_SIZE_LARGE   (1 << 2)
// #define ED_SAV_SRM_SIZE_LARGE   (1 << 3)
#define ED_SAV_RAM_BANK_ON      (1 << 7)
#define ED_SAV_RAM_BANK_APPLY   (1 << 15)

// #define ED_STATE_DMA_BUSY       (1 << 0) // 1
// #define ED_STATE_DMA_TOUT       (1 << 1) // 2
// #define ED_STATE_USB_TXE        (1 << 2) // 4
// #define ED_STATE_USB_RXF        (1 << 3) // 8
// #define ED_STATE_SPI            (1 << 4) // 16

// #define ED_CFG_SDRAM_OFF        (0 << 0) // 0
// #define ED_CFG_SDRAM_ON         (1 << 0) // 1
// #define ED_CFG_BYTESWAP         (1 << 1) // 2
#define ED_CFG_WR_MOD           (1 << 2) // 4
#define ED_CFG_WR_ADDR_MASK     (1 << 3) // 8
// 16 reserved
#define ED_CFG_MODE_RTC_OFF     (0 << 5)
#define ED_CFG_MODE_RTC_ON      (1 << 5) // 32
// 64 reserved
#define ED_CFG_MODE_GPIO_OFF    (0 << 6)
#define ED_CFG_MODE_GPIO_ON     (1 << 6) // 96 - this is strange... TODO: how to correctly use?
// 128 reserved
#define ED_CFG_DD_CC_ON         (1 << 8) // 256 // handle cart rom dd-cart conversion rom
#define ED_CFG_DD_CC_WE         (1 << 9) // 512



uint8_t ed64_vseries_ll_dma_busy() {
    // uint32_t resp;
    // while ((resp = io_read(ED_STATUS_REG)) & ED_STATE_DMA_BUSY) {
    //     if (resp & ED_STATE_DMA_TOUT)
    //     {
    //         return resp & ED_STATE_DMA_TOUT;
    //     }
    // }
    // return 0;
    while ((io_read(ED_STATUS_REG) & ED_STATE_DMA_BUSY) != 0);
    return io_read(ED_STATUS_REG) & ED_STATE_DMA_TOUT;

}

uint8_t ed64_vseries_ll_usb_read_busy() {
    return io_read(ED_STATUS_REG) & ED_STATE_RXF;
}

uint8_t ed64_vseries_ll_usb_write_busy() {
    return io_read(ED_STATUS_REG) & ED_STATE_TXE;
}


uint8_t ed64_vseries_ll_usb_read(uint32_t address, uint32_t length) {

    address /= 4;
    while (ed64_vseries_ll_usb_read_busy() != 0);

    io_write(ED_DMA_LEN_REG, length - 1);
    io_write(ED_DMA_ADDR_REG, address);
    io_write(ED_DMA_CFG_REG, ED_DMA_FIFO_TO_RAM);

    if (ed64_vseries_ll_dma_busy() != 0)return 1; //EVD_ERROR_FIFO_TIMEOUT;

    return 0;
}

uint8_t ed64_vseries_ll_usb_write(uint32_t address, uint32_t length) {

    address /= 4;
    while (ed64_vseries_ll_usb_write_busy() != 0);

    io_write(ED_DMA_LEN_REG, length - 1);
    io_write(ED_DMA_ADDR_REG, address);
    io_write(ED_DMA_CFG_REG, ED_DMA_RAM_TO_FIFO);

    if (ed64_vseries_ll_dma_busy() != 0)return 1; //EVD_ERROR_FIFO_TIMEOUT;

    return 0;
}

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

void ed64_vseries_ll_enable_gpio(void)
{
    uint16_t cfg = io_read(ED_CFG_REG);
    cfg |= ED_CFG_MODE_GPIO_ON;
    io_write(ED_CFG_REG, cfg);
}

void ed64_vseries_ll_disable_gpio(void)
{
    uint16_t cfg = io_read(ED_CFG_REG);
    cfg &= ~ED_CFG_MODE_GPIO_ON;
    io_write(ED_CFG_REG, cfg);
}

void ed64_vseries_ll_v3_enable_rtc (void) {
    uint16_t cfg = io_read(ED_CFG_REG);
    cfg &= ~ED_CFG_MODE_GPIO_ON;
    cfg |= ED_CFG_MODE_RTC_ON;
    io_write(ED_CFG_REG, cfg);
}

void ed64_vseries_ll_v3_write(uint16_t address, uint16_t data) {
    io_write(ED_V3_FLA_ADDR_REG, address);
    io_write(ED_V3_FLA_DATA_REG, data);
}

bool ed64_vseries_ll_set_save_type(ed64_vseries_save_type_t type, bool use_config_ram_bank) {

    bool config_ram_bank_enable = !use_config_ram_bank;
    uint16_t save_cfg = 0; // io_read(ED_SAV_CFG_REG);

    switch (type) {
        case SAVE_TYPE_EEPROM_4KBIT:
            save_cfg |= ED_SAV_EEP_ON;
            break;
        case SAVE_TYPE_EEPROM_16KBIT:
            save_cfg |= ED_SAV_EEP_ON;
            save_cfg |= ED_SAV_EEP_SIZE;
            break;
        case SAVE_TYPE_SRAM_256KBIT:
            save_cfg |= ED_SAV_SRM_ON;
            break;
        case SAVE_TYPE_SRAM_BANKED:
        case SAVE_TYPE_SRAM_1MBIT:
            save_cfg |= ED_SAV_SRM_ON;
            save_cfg |= ED_SAV_SRM_SIZE;
            break;
        case SAVE_TYPE_FLASHRAM_1MBIT:
            save_cfg |= ED_SAV_SRM_SIZE;
            break;
        case SAVE_TYPE_NONE:
        default:
            save_cfg |= ~ED_SAV_SRM_ON;
            save_cfg |= ~ED_SAV_SRM_SIZE;
            save_cfg |= ED_SAV_RAM_BANK_ON;
            break;
    }
    
    if (config_ram_bank_enable) { save_cfg |= ED_SAV_RAM_BANK_ON; }

    save_cfg |= ED_SAV_RAM_BANK_APPLY;

    io_write(ED_SAV_CFG_REG, save_cfg);

    // TODO: verify write
    return false; // false on success
}

void ed64_vseries_ll_update_firmware(uint8_t *firmware_data) {

    debugf("Starting firmware update...\n");

    //uint16_t cfg = io_read(ED_CFG_REG);
    io_write(ED_CFG_REG, ED_CFG_SDRAM_OFF); // disable sram during firmware update

    io_write(ED_CFG_CNT_REG, 0);
    wait_ms(10);
    io_write(ED_CFG_CNT_REG, 1);
    wait_ms(10);

    uint32_t i = 0;
    uint16_t f_ctr = 0;
    for (;;) {

        io_write(ED_CFG_DAT_REG, *(uint16_t *) & firmware_data[i]);
        while ((io_read(ED_CFG_CNT_REG) & 8) != 0);

        f_ctr = firmware_data[i++] == 0xFF ? f_ctr + 1 : 0;
        if (f_ctr >= 47)break;
        f_ctr = firmware_data[i++] == 0xFF ? f_ctr + 1 : 0;
        if (f_ctr >= 47)break;
    }


    while ((io_read(ED_CFG_CNT_REG) & 4) == 0) {
        io_write(ED_CFG_DAT_REG, 0xFFFF);
        while ((io_read(ED_CFG_CNT_REG) & 8) != 0);
    }

    wait_ms(20);

    //io_write(ED_CFG_REG, ED_CFG_SDRAM_ON); //re-enable sram

    ed_init();

    debugf("Firmware update completed.\n");
}

uint16_t ed64_vseries_ll_message_read(void) {
    return io_read(ED_MSG_REG);
}

void ed64_vseries_ll_message_write(uint16_t data) {
    io_write(ED_MSG_REG, data);
}

void ed64_vseries_ll_dd_cc_ram_oe(void) {

    uint16_t cfg = io_read(ED_CFG_REG);
    cfg &= ~ED_CFG_DD_CC_WE;
    cfg |= ED_CFG_DD_CC_ON;
    io_write(ED_CFG_REG, cfg);
}

void ed64_vseries_ll_dd_cc_ram_we(void) {

    uint16_t cfg = io_read(ED_CFG_REG);
    cfg |= ED_CFG_DD_CC_ON | ED_CFG_DD_CC_WE;
    io_write(ED_CFG_REG, cfg);
}

void ed64_vseries_ll_dd_cc_ram_off(void) {

    uint16_t cfg = io_read(ED_CFG_REG);
    cfg &= ~(ED_CFG_DD_CC_ON | ED_CFG_DD_CC_WE);
    io_write(ED_CFG_REG, cfg);
}

void ed64_vseries_ll_dd_cc_ram_clr(void) {

    uint16_t cfg = io_read(ED_CFG_REG);
    cfg |= ED_CFG_DD_CC_WE;
    cfg &= ~ED_CFG_DD_CC_ON;
    io_write(ED_CFG_REG, cfg);
    wait_ms(100);
}

bool ed64_vseries_ll_dd_ram_supported(void) {

    return (io_read(ED_STATUS_REG) >> 15) & 1;
}
