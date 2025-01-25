/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter.h                                         *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef M64P_DEVICE_R4300_VR4300_JITTER_H
#define M64P_DEVICE_R4300_VR4300_JITTER_H

#if !defined(NEW_DYNAREC) && defined(VR4300_JITTER)

#include <sys/types.h>
#include <stdalign.h>

#include "util.h"

#define HOST_CODE_SIZE (1 << 28)

#include "device/r4300/recomp_types.h" /* for precomp_instr */

#include <stdint.h>
#include <stddef.h>

#include "../cp1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEMORY_MASK (RAM_MASK & ~3)

#define VR4300_JITTER_X86 1
#define VR4300_JITTER_X64 2
#define VR4300_JITTER_ARM 3
#define VR4300_JITTER_ARM64 4

// TODO: Create a JitSettings class and move things below here there as runtime options.

#define DISABLE_FASTMEM 0
#define DISABLE_RDRAM_OPTIMIZATION 0

#define DISABLE_CALLRET_OPTIMIZIATION 0

#define MAX_FAULT_COUNT 0

#define DEBUG_PREDICTIONS 0
//#define PRINT_EXTRA_STUFF
#define MAX_LINK_CONTINUE 10000
#define MAX_BLOCK_EXTENSIONS 10000
#define MAX_BRANCH_FOLLOWS 10000

#define LINK_CONTINUE_INSTRUCTION_THRESHOLD 4096
#define EXTEND_INSTRUCTION_THRESHOLD 4096
#define FOLLOW_INSTRUCTION_THRESHOLD 4096

#define MAX_INSTR_PER_BLOCK 4096
#define USE_REG_FOR_STORED_PC 0
#define DISABLE_IDLE_SKIPPING 0
#define DISABLE_DISCARD 0
#define ALWAYS_BIND_64BIT_IMMEDIATES 0
#define MARK_INTERRUPT_UNSAFE_STATE 0
#define DISABLE_GPR_REG_CACHE 0
#define DISABLE_FLOAT_REG_CACHE 0
#define PRINT_FLUSHES 0
#define PRINT_CP1_REGS 1
#define PRINT_IM_HERE_DEBUG 0
#define PRINT_IM_HERE_DEBUG_NO_REGS 1
#define PRINT_IM_HERE_DEBUG_SKIP 15707270

#define HUGE_MAP_FOR_ENTRY_POINTS 1

#define FAST_DISPATCHER_ALWAYS_TRANSLATE 0

// Enabling this is slower, kernel code is processing a lot.
// TODO: Investigate this further, it could be a nice improvement if it works.
// and i think it did work at some point.
#define MEMMAP_TLB_REGIONS 0

#define NO_FLOAT_INVALIDATE 1

#define PROFILE_INSTRUCTIONS 0

struct r4300_core;

struct precomp_instr;

struct recompiler_hot_state
{
    uint32_t pc;
    uint32_t stored_pc;
    int cycle_count;
    int hot_cycles;
    int cycles_added;
    unsigned int next_interrupt;
    int64_t regs[32];
    cp1_reg cp1_regs[32];
    uint32_t cp0_regs[32];

    uint32_t cp1_fcr0;
    uint32_t cp1_fcr31;
    int64_t  hi;
    int64_t  lo;
    uint32_t dbg_pc;
    int compiler_cycles;
    int stop;
    int last_block_broken;
    int pc_changed;
    void *curBlock;
    int llbit;

    uint64_t cp0_latch;
    uint64_t cp2_latch;
    unsigned int delay_slot;

    uint8_t *stored_stack_pointer;

    float* cp1_regs_simple[32];
    double* cp1_regs_double[32];
    uint32_t rounding_modes[4];

    int64_t *gprs_tmp_ptr;
    int64_t *fprs_tmp_ptr;
    int64_t gprs_tmp[32];
    int64_t fprs_tmp[32];
    int instructionsLeft;
    int isLastInstruction;
    int inDelaySlot;
    struct jit_instr *op;
    int modifies_count_reg;
    int modifies_status_reg;
    int float_check_compiled;
    int ctc2_check_compiled;
    int is_idle_wait_loop;
    uint32_t branch_to;
    uint32_t fr_is_set;

    struct precomp_instr *fake_pc;
    int exception;
    int exceptionless_block;
    int rdram_generate_slowcode;
    int rdram_corruption_changed;

#if COMPARE_CORE
#define NUM_DBG_INSTRUCTIONS 10
    uint32_t last_addresses[NUM_DBG_INSTRUCTIONS];
    uint32_t last_instructions[NUM_DBG_INSTRUCTIONS];
    char *last_names[NUM_DBG_INSTRUCTIONS];
    int last_idx;
#endif
};

void vr4300_jitter_fix_hot_cycles(void);

void vr4300_jitter_invalidate_cached_code(struct r4300_core* r4300, uint32_t address, size_t size);
void vr4300_jitter_invalidate_cached_code_just_erase(struct r4300_core* r4300, uint32_t address, size_t size);
void vr4300_jitter_init(void);
void vr4300_jitter_start(void);
void vr4300_jitter_cleanup(void);
void *vr4300_jitter_recompile_block(unsigned int addr);
const void *vr4300_jitter_dispatch(unsigned int addr);

void *vr4300_jitter_initialize_fastmem(void);
void vr4300_jitter_uninitialize_fastmem(void);
uint8_t *vr4300_jitter_get_logical_memory(int type);
uint32_t vr4300_jitter_translate_address(uint32_t address, int w);
uint32_t vr4300_jitter_translate_address_no_exception(uint32_t, int w);
uint32_t vr4300_jitter_translate_pc_no_exception(uint32_t);

void vr4300_jitter_map_corrupt_rdram(int corrupt);

#define HOT_STATE (&g_dev.r4300.recompiler_hot_state)

#ifdef __cplusplus
};
#endif

#endif

#endif /* M64P_DEVICE_R4300_VR4300_JITTER_H */
