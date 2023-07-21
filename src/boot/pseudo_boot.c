#include <libdragon.h>;
#include <stdint.h>;

static datel_cheats_enabled = false;

void pseudo_boot(uint32_t cart_cic, uint8_t pseudo_cic, uint32_t *cheat_lists[2]) {
    // // Clear screen
    // IO_WRITE(VI_V_INT, 0x3FF);
    // IO_WRITE(VI_H_LIMITS, 0);
    // IO_WRITE(VI_CUR_LINE, 0);

    // // Reset controller and clear interrupt
    // IO_WRITE(PI_STATUS_REG, 0x03);

    // // Set cart latency registers with values specified in ROM
    // uint32_t lat = ROM[0];
    // IO_WRITE(PI_BSD_DOM1_LAT_REG, lat & 0xFF);
    // IO_WRITE(PI_BSD_DOM1_PWD_REG, lat >> 8);
    // IO_WRITE(PI_BSD_DOM1_PGS_REG, lat >> 16);
    // IO_WRITE(PI_BSD_DOM1_RLS_REG, lat >> 20);

    // Fix RAM size location (State required by CIC-NUS-6105)
    // volatile uint32_t *ram_size = (cart_cic == CIC_6105) ? &RAM_SIZE_2 : &RAM_SIZE_1;
    // *ram_size = (pseudo_cic == CIC_6105) ? RAM_SIZE_2 : RAM_SIZE_1;

    if (datel_cheats_enabled) {
        // Copy patcher into a memory location where it will not be overwritten
        void *patcher = (void*)0x80700000; // Temporary patcher location (within Expansion PAK region)
        uint32_t datel_patcher_length = &&datel_patcher_end - &&datel_patcher_start;
        memcpy(patcher, &&datel_patcher_start, datel_patcher_length);

        // Copy code lists into memory, behind the patcher
        uint32_t *p = patcher + datel_patcher_length;
        *(uint32_t**)0xA06FFFFC = p; // Save temporary code list location
        for (int i = 0; i < 2; i++) {
            int j = -2;
            do {
                j += 2;
                datel_patcher_length += 8;

                *p++ = cheat_lists[i][j];
                *p++ = cheat_lists[i][j + 1];
            } while (cheat_lists[i][j] || cheat_lists[i][j + 1]);
        }

        // Write cache to physical memory and invalidate
        data_cache_hit_writeback(patcher, datel_patcher_length);
        inst_cache_hit_invalidate(patcher, datel_patcher_length);
    }

    // Start ROM via CIC boot code
    asm __volatile__ (
        ".set noreorder;"

        "lui    $t0, 0x8000;"

        // State required by all CICs
        "move   $s3, $zero;"            // osRomType (0: N64, 1: 64DD)
        "lw     $s4, 0x0300($t0);"      // osTvType (0: PAL, 1: NTSC, 2: MPAL)
        "move   $s5, $zero;"            // osResetType (0: Cold, 1: NMI)
        "lui    $s6, %%hi(cic_ids);"    // osCicId (See cic_ids LUT)
        "addu   $s6, $s6, %0;"
        "lbu    $s6, %%lo(cic_ids)($s6);"
        "lw     $s7, 0x0314($t0);"      // osVersion

        // Copy PIF code to RSP IMEM (State required by CIC-NUS-6105)
        "lui    $a0, 0xA400;"
        "lui    $a1, %%hi(imem_start);"
        "ori    $a2, $zero, 0x0008;"
    "1:"
        "lw     $t0, %%lo(imem_start)($a1);"
        "addiu  $a1, $a1, 4;"
        "sw     $t0, 0x1000($a0);"
        "addiu  $a2, $a2, -1;"
        "bnez   $a2, 1b;"
        "addiu  $a0, $a0, 4;"

        // Copy CIC boot code to RSP DMEM
        "lui    $t3, 0xA400;"
        "ori    $t3, $t3, 0x0040;"      // State required by CIC-NUS-6105
        "move   $a0, $t3;"
        "lui    $a1, 0xB000;"
        "ori    $a2, 0x0FC0;"
    "1:"
        "lw     $t0, 0x0040($a1);"
        "addiu  $a1, $a1, 4;"
        "sw     $t0, 0x0000($a0);"
        "addiu  $a2, $a2, -4;"
        "bnez   $a2, 1b;"
        "addiu  $a0, $a0, 4;"

        // Boot with or without datel cheats enabled?
        "beqz   %1, 2f;"

        // Patch CIC boot code
        "lui    $a1, %%hi(cic_patch_offsets);"
        "addu   $a1, $a1, %0;"
        "lbu    $a1, %%lo(cic_patch_offsets)($a1);"
        "addu   $a0, $t3, $a1;"
        "lui    $a1, 0x081C;"           // "j 0x80700000"
        "ori    $a2, $zero, 0x06;"
        "bne    %0, $a2, 1f;"
        "lui    $a2, 0x8188;"
        "ori    $a2, $a2, 0x764A;"
        "xor    $a1, $a1, $a2;"         // CIC-NUS-6106 encryption
    "1:"
        "sw     $a1, 0x0700($a0);"      // Patch CIC boot code with jump

        // Patch CIC boot code to disable checksum failure halt
        // Required for CIC-NUS-6105
        "ori    $a2, $zero, 0x05;"
        "beql   %0, $a2, 2f;"
        "sw     $zero, 0x06CC($a0);"

        // Go!
    "2:"
        "lui    $sp, 0xA400;"
        "ori    $ra, $sp, 0x1550;"      // State required by CIC-NUS-6105
        "jr     $t3;"
        "ori    $sp, $sp, 0x1FF0;"      // State required by CIC-NUS-6105


    // Table of all CIC IDs
    "cic_ids:"
        ".byte  0x00;"                  // Unused
        ".byte  0x3F;"                  // NUS-CIC-6101
        ".byte  0x3F;"                  // NUS-CIC-6102
        ".byte  0x78;"                  // NUS-CIC-6103
        ".byte  0xAC;"                  // Unused NUS-CIC-5101 hacked to 4 0xAC seed
        ".byte  0x91;"                  // NUS-CIC-6105
        ".byte  0x85;"                  // NUS-CIC-6106
        ".byte  0xDD;"                  // NUS-CIC-5167

    "cic_patch_offsets:"
        ".byte  0x00;"                  // Unused
        ".byte  0x30;"                  // CIC-NUS-6101
        ".byte  0x2C;"                  // CIC-NUS-6102
        ".byte  0x20;"                  // CIC-NUS-6103
        ".byte  0x30;"                  // Unused NUS-CIC-5101 hacked to 4 same patch offset like 6101
        ".byte  0x8C;"                  // CIC-NUS-6105
        ".byte  0x60;"                  // CIC-NUS-6106
        ".byte  0x30;"                  // NUS-CIC-5167

    // These instructions are copied to RSP IMEM; we don't execute them.
    "imem_start:"
        "lui    $t5, 0xBFC0;"
    "1:"
        "lw     $t0, 0x07FC($t5);"
        "addiu  $t5, $t5, 0x07C0;"
        "andi   $t0, $t0, 0x0080;"
        "bnezl  $t0, 1b;"
        "lui    $t5, 0xBFC0;"
        "lw     $t0, 0x0024($t5);"
        "lui    $t3, 0xB000;"

        :                               // outputs
        : "r" (cart_cic),               // inputs
          "r" (datel_cheats_enabled)
        : "$4", "$5", "$6", "$8",       // clobber
          "$11", "$19", "$20", "$21",
          "$22", "$23", "memory"
    );
// Start ROM with datel cheats
datel_patcher_start:
    asm __volatile__ (
        ".set noat;"
        ".set noreorder;"

    // Installs general exception handler, router, and code engine
    "patcher:"
        "lui    $t5, 0x8070;"           // Start of temporary patcher location
        "lui    $t6, 0x8000;"           // Start of cached memory
        "li     $t7, 0x007FFFFF;"       // Address mask
        "li     $t8, 0x807C5C00;"       // Permanent code engine location
//      "lw     $t9, 0xFFFC($t5);"      // Assembles incorrectly (gcc sucks)
        "addiu  $t9, $t5, -4;"          // Go to hell, gcc!
        "lw     $t9, 0x0000($t9);"      // Get temporary code lists location

    "1:"
        // Apply boot-time cheats
        "lw     $v0, 0x0000($t9);"
        "bnez   $v0, 2f;"
        "lw     $v1, 0x0004($t9);"
        "beqz   $v1, 1f;"

    "2:"
        "addiu  $t9, $t9, 0x0008;"
        "srl    $t2, $v0, 24;"
        "li     $at, 0xEE;"
        "beq    $t2, $at, 5f;"
        "li     $at, 0xF0;"
        "and    $v0, $v0, $t7;"
        "beq    $t2, $at, 4f;"
        "or     $v0, $v0, $t6;"
        "li     $at, 0xF1;"
        "beq    $t2, $at, 3f;"
        "nop;"

        // Apply FF code type
        "li     $at, 0xFFFFFFFC;"       // Mask address
        "b      1b;"
        "and    $t8, $v0, $at;"         // Update permanent code engine location

    "3:"
        // Apply F1 code type
        "b      1b;"
        "sh     $v1, 0x0000($v0);"

    "4:"
        // Apply F0 code type
        "b      1b;"
        "sb     $v1, 0x0000($v0);"

    "5:"
        // Apply EE code type
        "lui    $v0, 0x0040;"
        "sw     $v0, 0x0318($t6);"
        "b      1b;"
        "sw     $v0, 0x03F0($t6);"


    "1:"
        // Install General Exception Handler
        "srl    $at, $t8, 2;"
        "and    $v0, $at, $t7;"
        "lui    $at, 0x0800;"
        "or     $v0, $v0, $at;"         // Jump to code engine
        "sw     $v0, 0x0180($t6);"
        "sw     $zero, 0x0184($t6);"


        // Install code engine to permanent location
        "sw     $t8, 0x0188($t6);"      // Save permanent code engine location
        "la     $at, %%lo(patcher);"
        "la     $v0, %%lo(code_engine_start);"
        "la     $v1, %%lo(code_engine_end);"
        "subu   $at, $v0, $at;"         // Get patcher length
        "subu   $v1, $v1, $v0;"         // Get code engine length
        "addu   $v0, $t5, $at;"         // Get temporary code engine location
    "1:"
        "lw     $at, 0x0000($v0);"
        "addiu  $v1, $v1, -4;"
        "sw     $at, 0x0000($t8);"
        "addiu  $v0, $v0, 4;"
        "bgtz   $v1, 1b;"
        "addiu  $t8, $t8, 4;"
        "sw     $t8, 0x018C($t6);"      // Save permanent code list location


    "1:"
        // Install in-game code list
        "lw     $v0, 0x0000($t9);"
        "lw     $v1, 0x0004($t9);"
        "addiu  $t9, $t9, 8;"
        "sw     $v0, 0x0000($t8);"
        "sw     $v1, 0x0004($t8);"
        "bnez   $v0, 1b;"
        "addiu  $t8, $t8, 8;"
        "bnez   $v1, 1b;"
        "nop;"


        // Write cache to physical memory and invalidate (GEH)
        "ori    $t0, $t6, 0x0180;"
        "li     $at, 0x0010;"
    "1:"
        "cache  0x19, 0x0000($t0);"     // Data cache hit writeback
        "cache  0x10, 0x0000($t0);"     // Instruction cache hit invalidate
        "addiu  $at, $at, -4;"
        "bnez   $at, 1b;"
        "addiu  $t0, $t0, 4;"


        // Write cache to physical memory and invalidate (code engine + list)
        "lw     $t0, 0x0188($t6);"
        "subu   $at, $t8, $t0;"
    "1:"
        "cache  0x19, 0x0000($t0);"     // Data cache hit writeback
        "cache  0x10, 0x0000($t0);"     // Instruction cache hit invalidate
        "addiu  $at, $at, -4;"
        "bnez   $at, 1b;"
        "addiu  $t0, $t0, 4;"


        // Protect GEH via WatchLo/WatchHi
        "li     $t0, 0x0181;"           // Watch 0x80000180 for writes
        "mtc0   $t0, $18;"              // Cp0 WatchLo
        "nop;"
        "mtc0   $zero, $19;"            // Cp0 WatchHi


        // Start game!
        "jr     $t1;"
        "nop;"


    "code_engine_start:"
        "mfc0   $k0, $13;"              // Cp0 Cause
        "andi   $k1, $k0, 0x1000;"      // Pre-NMI
        "bnezl  $k1, 1f;"
        "mtc0   $zero, $18;"            // Cp0 WatchLo
    "1:"

        "andi   $k0, $k0, 0x7C;"
        "li     $k1, 0x5C;"             // Watchpoint
        "bne    $k0, $k1, 1f;"

        // Watch exception; manipulate register contents
        "mfc0   $k1, $14;"              // Cp0 EPC
        "lw     $k1, 0x0000($k1);"      // Load cause instruction
        "lui    $k0, 0x03E0;"
        "and    $k1, $k1, $k0;"         // Mask (base) register
        "srl    $k1, $k1, 5;"           // Shift it to the "rt" position
        "lui    $k0, 0x3740;"           // Upper half "ori $zero, $k0, 0x0120"
        "or     $k1, $k1, $k0;"
        "ori    $k1, $k1, 0x0120;"      // Lower half "ori $zero, $k0, 0x0120"
        "lui    $k0, 0x8000;"
        "lw     $k0, 0x0188($k0);"      // Load permanent code engine location
        "sw     $k1, 0x0060($k0);"      // Self-modifying code FTW!
        "cache  0x19, 0x0060($k0);"     // Data cache hit writeback
        "cache  0x10, 0x0060($k0);"     // Instruction cache hit invalidate
        "lui    $k0, 0x8000;"
        "nop;"                          // Short delay for cache sync
        "nop;"
        "nop;"
        "nop;"                          // Placeholder for self-modifying code
        "eret;"                         // Back to game

    "1:"
        // Run code engine
        "lui    $k0, 0x8000;"
        "lw     $k0, 0x0188($k0);"
        "addiu  $k0, $k0, -0x28;"
        "sd     $v1, 0x0000($k0);"
        "sd     $v0, 0x0008($k0);"
        "sd     $t9, 0x0010($k0);"
        "sd     $t8, 0x0018($k0);"
        "sd     $t7, 0x0020($k0);"

        // Handle cheats
        "lui    $t9, 0x8000;"
        "lw     $t9, 0x018C($t9);"      // Get code list location
    "1:"
        "lw     $v0, 0x0000($t9);"      // Load address
        "bnez   $v0, 2f;"
        "lw     $v1, 0x0004($t9);"      // Load value
        "beqz   $v1, 4f;"
        "nop;"

        // Address == 0 (TODO)
        "b      1b;"

    "2:"
        // Address != 0
        "addiu  $t9, $t9, 0x0008;"
        "srl    $t7, $v0, 24;"
        "sltiu  $k1, $t7, 0xD0;"        // Code type < 0xD0 ?
        "sltiu  $t8, $t7, 0xD4;"        // Code type < 0xD4 ?
        "xor    $k1, $k1, $t8;"         // $k1 = (0xD0 >= code_type < 0xD4)
        "li     $t8, 0x50;"
        "bne    $t7, $t8, 3f;"          // Code type != 0x50 ? -> 3

        // GS Patch/Repeater
        "srl    $t8, $v0, 8;"
        "andi   $t8, $t8, 0x00FF;"      // Get address count
        "andi   $t7, $v0, 0x00FF;"      // Get address increment
        "lw     $v0, 0x0000($t9);"      // Load address
        "lw     $k1, 0x0004($t9);"      // Load value
        "addiu  $t9, $t9, 0x0008;"

    "2:"
        "sh     $k1, 0x0000($v0);"      // Repeater/Patch write
        "addiu  $t8, $t8, -1;"
        "addu   $v0, $v0, $t7;"
        "bnez   $t8, 2b;"
        "addu   $k1, $k1, $v1;"
        "b      1b;"

    "3:"
        // GS RAM write or Conditional
        "lui    $t7, 0x0300;"
        "and    $t7, $v0, $t7;"         // Test for 8-bit or 16-bit code type
        "li     $t8, 0xA07FFFFF;"
        "and    $v0, $v0, $t8;"
        "lui    $t8, 0x8000;"
        "beqz   $k1, 2f;"
        "or     $v0, $v0, $t8;"         // Mask address

        // GS Conditional
        "sll    $k1, $t7, 7;"
        "beqzl  $k1, 3f;"
        "lbu    $t8, 0x0000($v0);"      // 8-bit conditional
        "lhu    $t8, 0x0000($v0);"      // 16-bit conditional
    "3:"
        "srl    $t7, $t7, 22;"
        "andi   $t7, $t7, 8;"           // Test for equal-to or not-equal-to
        "beql   $v1, $t8, 1b;"
        "add    $t9, $t9, $t7;"         // Add if equal
        "xori   $t7, $t7, 8;"
        "b      1b;"
        "add    $t9, $t9, $t7;"         // Add if not-equal

    "2:"
        // GS RAM write
        "sll    $k1, $t7, 7;"
        "beqzl  $k1, 3f;"
        "sb     $v1, 0x0000($v0);"      // Constant 8-bit write
        "sh     $v1, 0x0000($v0);"      // Constant 16-bit write
    "3:"
        "b      1b;"

    "4:"
        // Restore registers from our temporary stack, and back to the game!
        "ld     $t7, 0x0020($k0);"
        "ld     $t8, 0x0018($k0);"
        "ld     $t9, 0x0010($k0);"
        "ld     $v0, 0x0008($k0);"
        "j      0x80000120;"
        "ld     $v1, 0x0000($k0);"
    "code_engine_end:"

        :                               // outputs
        :                               // inputs
        : "memory"                      // clobber
    );
datel_patcher_end:

    return;
}
