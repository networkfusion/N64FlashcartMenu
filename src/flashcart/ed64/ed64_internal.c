#include <libdragon.h>

#include "ed64_internal.h"


typedef struct {
    uint32_t SR_CMD;
    uint32_t DATA[2];
    uint32_t IDENTIFIER;
    uint32_t KEY;
} ed64_regs_t;

#define ED64_REGS_BASE          (0x1FFF0000UL)
#define ED64_REGS               ((ed64_regs_t *) ED64_REGS_BASE)

#define ED64_SR_IRQ_PENDING     (1 << 29)
#define ED64_SR_CMD_ERROR       (1 << 30)
#define ED64_SR_CPU_BUSY        (1 << 31)

#define ED64_V2_IDENTIFIER      (0x53437632)

#define ED64_KEY_RESET          (0x00000000UL)
#define ED64_KEY_UNLOCK_1       (0x5F554E4CUL)
#define ED64_KEY_UNLOCK_2       (0x4F434B5FUL)
#define ED64_KEY_LOCK           (0xFFFFFFFFUL)


typedef enum {
    CMD_ID_VERSION_GET          = 'V',
    CMD_ID_CONFIG_GET           = 'c',
    CMD_ID_CONFIG_SET           = 'C',
    CMD_ID_WRITEBACK_SD_INFO    = 'W',
    CMD_ID_FLASH_PROGRAM        = 'K',
    CMD_ID_FLASH_WAIT_BUSY      = 'p',
    CMD_ID_FLASH_ERASE_BLOCK    = 'P',
} ed64_cmd_id_t;

typedef struct {
    ed64_cmd_id_t id;
    uint32_t arg[2];
    uint32_t rsp[2];
} ed64_cmd_t;


static ed64_error_t ed64_execute_cmd (ed64_cmd_t *cmd) {
    io_write((uint32_t) (ED64_REGS->DATA[0]), cmd->arg[0]);
    io_write((uint32_t) (&ED64_REGS->DATA[1]), cmd->arg[1]);

    io_write((uint32_t) (&ED64_REGS->SR_CMD), (cmd->id & 0xFF));

    uint32_t sr;
    do {
        sr = io_read((uint32_t) (&ED64_REGS->SR_CMD));
    } while (sr & ED64_SR_CPU_BUSY);

    if (sr & ED64_SR_CMD_ERROR) {
        return (ed64_error_t) (io_read((uint32_t) (&ED64_REGS->DATA[0])));
    }

    cmd->rsp[0] = io_read((uint32_t) (&ED64_REGS->DATA[0]));
    cmd->rsp[1] = io_read((uint32_t) (&ED64_REGS->DATA[1]));

    return ED64_OK;
}


void ed64_unlock (void) {
    io_write((uint32_t) (&ED64_REGS->KEY), ED64_KEY_RESET);
    io_write((uint32_t) (&ED64_REGS->KEY), ED64_KEY_UNLOCK_1);
    io_write((uint32_t) (&ED64_REGS->KEY), ED64_KEY_UNLOCK_2);
}

void ed64_lock (void) {
    io_write((uint32_t) (&ED64_REGS->KEY), ED64_KEY_LOCK);
}

bool ed64_check_presence (void) {
    return (io_read((uint32_t) (&ED64_REGS->IDENTIFIER)) == ED64_V2_IDENTIFIER);
}

void ed64_read_data (void *src, void *dst, size_t length) {
    data_cache_hit_writeback_invalidate(dst, length);
    dma_read_raw_async(dst, (uint32_t) (src), length);
    dma_wait();
}

void ed64_write_data (void *src, void *dst, size_t length) {
    data_cache_hit_writeback(src, length);
    dma_write_raw_async(src, (uint32_t) (dst), length);
    dma_wait();
}

ed64_error_t ed64_get_version (uint16_t *major, uint16_t *minor) {
    ed64_cmd_t cmd = { .id = CMD_ID_VERSION_GET };
    ed64_error_t error = ed64_execute_cmd(&cmd);
    *major = ((cmd.rsp[0] >> 16) & 0xFFFF);
    *minor = (cmd.rsp[0] & 0xFFFF);
    return error;
}

ed64_error_t ed64_get_config (ed64_cfg_t id, void *value) {
    ed64_cmd_t cmd = { .id = CMD_ID_CONFIG_GET, .arg = { id } };
    ed64_error_t error = ed64_execute_cmd(&cmd);
    *((uint32_t *) (value)) = cmd.rsp[1];
    return error;
}

ed64_error_t ed64_set_config (ed64_cfg_t id, uint32_t value) {
    ed64_cmd_t cmd = { .id = CMD_ID_CONFIG_SET, .arg = { id, value } };
    return ed64_execute_cmd(&cmd);
}

ed64_error_t ed64_writeback_enable (void *address) {
    ed64_cmd_t cmd = { .id = CMD_ID_WRITEBACK_SD_INFO, .arg = { (uint32_t) (address) } };
    return ed64_execute_cmd(&cmd);
}

ed64_error_t ed64_flash_program (void *address, size_t length) {
    ed64_cmd_t cmd = { .id = CMD_ID_FLASH_PROGRAM, .arg = { (uint32_t) (address), length } };
    return ed64_execute_cmd(&cmd);
}

ed64_error_t ed64_flash_wait_busy (void) {
    ed64_cmd_t cmd = { .id = CMD_ID_FLASH_WAIT_BUSY, .arg = { true } };
    return ed64_execute_cmd(&cmd);
}

ed64_error_t ed64_flash_get_erase_block_size (size_t *erase_block_size) {
    ed64_cmd_t cmd = { .id = CMD_ID_FLASH_WAIT_BUSY, .arg = { false } };
    ed64_error_t error = ed64_execute_cmd(&cmd);
    *erase_block_size = (size_t) (cmd.rsp[0]);
    return error;
}

ed64_error_t ed64_flash_erase_block (void *address) {
    ed64_cmd_t cmd = { .id = CMD_ID_FLASH_ERASE_BLOCK, .arg = { (uint32_t) (address) } };
    return ed64_execute_cmd(&cmd);
}
