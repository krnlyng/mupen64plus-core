/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - new_dynarec.h                                           *
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

#ifndef M64P_DEVICE_R4300_VR4300_JITTER_UTIL_H
#define M64P_DEVICE_R4300_VR4300_JITTER_UTIL_H

#include <stdint.h>
#include <stdio.h>

static inline int vr4300_jitter_value_fits_in_32_bit_imm_positive(uint64_t value)
{
    return (value & 0xffffffff00000000) == 0;
}

static inline int vr4300_jitter_address_needs_translation(uint32_t address)
{
    if ((address & UINT32_C(0xc0000000)) != UINT32_C(0x80000000)) {
        return 1;
    }

    return 0;
}

static inline int vr4300_jitter_is_rdram_address(uint32_t address)
{
    return ((address & 0xDF800000) == 0) || ((address & 0xDF800000) == 0x80000000);
}

static inline int vr4300_jitter_is_mi_regs_address(uint32_t address)
{
    return ((address & 0x1fff0000) == 0x04300000);
}

static inline int vr4300_jitter_is_rsp_regs_address(uint32_t address)
{
    return ((address & 0x1fff0000) == 0x04040000);
}

#define RAM_MASK 0x7FFFFF

static inline uint32_t vr4300_jitter_rdram_dram_address(uint32_t address)
{
    return (address & RAM_MASK);
}

#define my_assert(op, arg, x) do { if (!(x)) fprintf(stderr, "ASSERT: " #x ": %d %s\n", op->arg, op->name); assert(x); } while (0)
#define my_assert2(op, reg, x) if (!(x)) fprintf(stderr, "ASSERT2: " #x ": %d %s\n", reg, op->name)

#define VALIDATE_IN(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->regsIn[op->arg] || !op->arg)); \
    } while(0)

#define VALIDATE_OUT(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->regsOut[op->arg] || !op->arg)); \
    } while(0)

#define VALIDATE_FIN(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->fregsIn[op->arg])); \
    } while(0)

#define VALIDATE_FIN_DOUBLE_NO_FR(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->fregsIn[op->arg & ~1])); \
    } while(0)

#define VALIDATE_FOUT(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->fregsOut[op->arg])); \
    } while(0)

#define VALIDATE_FOUT_DOUBLE_NO_FR(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->fregsOut[op->arg & ~1])); \
    } while(0)

#define VALIDATE_FIN32(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->fregsIn32[op->arg])); \
    } while(0)

#define VALIDATE_FOUT32(op, arg) \
    do { \
        my_assert(op, arg, op->has_##arg && (op->fregsOut32[op->arg])); \
    } while(0)

#define VALIDATE_REG_IN(op, arg) \
    do { \
        /* my_assert2(op, arg, (op->regsIn[arg] || !arg)); */ \
    } while(0)

#define VALIDATE_REG_OUT(op, arg) \
    do { \
        my_assert2(op, arg, (op->regsOut[arg] || !arg)); \
    } while(0)

#define VALIDATE_REG_FIN(op, arg) \
    do { \
        /* my_assert2(op, arg, op->fregsIn[arg]); */ \
    } while(0)

#define VALIDATE_REG_FOUT(op, arg) \
    do { \
        my_assert2(op, arg, op->fregsOut[arg]); \
    } while(0)

#define VALIDATE_REG_FIN32(op, arg) \
    do { \
        /* my_assert2(op, arg, op->fregsIn32[arg]); */ \
    } while(0)

#define VALIDATE_REG_FOUT32(op, arg) \
    do { \
        my_assert2(op, arg, op->fregsOut32[arg]); \
    } while(0)
#endif

//TODO
#ifdef M64P_BIG_ENDIAN
#define DOUBLE_HALF_XOR 1
#else
#define DOUBLE_HALF_XOR 0
#endif
