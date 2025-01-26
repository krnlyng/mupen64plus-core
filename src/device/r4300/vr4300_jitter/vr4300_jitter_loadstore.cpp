/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_loadstore.cpp                             *
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

#include "RegCache/GPRRegCache.h"
#include "RegCache/FPURegCache.h"

#include "vr4300_jitter_instruction_decoder.h"
#include "vr4300_jitter_internal.h"
#include "Common/x64Emitter.h"

using namespace Gen;

static unsigned int bshift(uint32_t address)
{
    return ((address & 3) ^ 3) << 3;
}

static unsigned int hshift(uint32_t address)
{
    return ((address & 2) ^ 2) << 3;
}

u32 read_word_from_dynarec(u32 lsaddr)
{
    u32 value;
    if (r4300_read_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, &value)) {
        return value;
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "%s EXCEPTION @ %d!\n", __func__, __LINE__);
#endif
    return 0;
}

u32 read_word_from_dynarec_set_llbit_on_success(u32 lsaddr)
{
    u32 value;
    if (r4300_read_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, &value)) {
        *r4300_llbit(VR4300_Jitter::GetInstance()->GetR4300Core()) = 1;
        return value;
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "%s EXCEPTION @ %d!\n", __func__, __LINE__);
#endif
    return 0;
}

u32 read_hword_from_dynarec(u32 lsaddr)
{
    u32 value;
    unsigned int shift = hshift(lsaddr);
    if (r4300_read_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, &value)) {
        return (value >> shift) & 0xffff;
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "%s EXCEPTION @ %d!\n", __func__, __LINE__);
#endif
    return 0;
}

u64 read_dword_from_dynarec(u32 lsaddr)
{
    u64 value;
    if (r4300_read_aligned_dword(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, &value)) {
        return value;
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "%s EXCEPTION @ %d!\n", __func__, __LINE__);
#endif
    return 0;
}

u32 read_byte_unsigned_from_dynarec(u32 lsaddr)
{
    u32 value;
    unsigned int shift = bshift(lsaddr);
    if (r4300_read_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, &value)) {
        return (value >> shift) & 0xff;
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "%s EXCEPTION @ %d!\n", __func__, __LINE__);
#endif
    return 0;
}

void write_word_from_dynarec(u32 lsaddr, u32 value)
{
    r4300_write_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, value, ~UINT32_C(0));
}

#define BITS_BELOW_MASK32(x) ((UINT32_C(1) << (x)) - 1)
#define BITS_ABOVE_MASK32(x) (~(BITS_BELOW_MASK32((x))))

#define BITS_BELOW_MASK64(x) ((UINT64_C(1) << (x)) - 1)
#define BITS_ABOVE_MASK64(x) (~(BITS_BELOW_MASK64((x))))

void write_word_from_dynarec_SWL(u32 lsaddr, u32 value)
{
    unsigned int n = (lsaddr & 3);
    unsigned int shift = 8 * n;
    uint32_t mask = (n == 0)
        ? ~UINT32_C(0)
        : BITS_BELOW_MASK32(8 * (4 - n));

    r4300_write_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr & ~UINT32_C(0x3), value >> shift, mask);
}

void write_word_from_dynarec_SWR(u32 lsaddr, u32 value)
{
    unsigned int n = (lsaddr & 3);
    unsigned int shift = 8 * (3 - n);
    uint32_t mask = BITS_ABOVE_MASK32(8 * (3 - n));

    r4300_write_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr & ~UINT32_C(0x3), value << shift, mask);
}

void write_dword_from_dynarec_SDL(u32 lsaddr, u64 value)
{
    unsigned int n = (lsaddr & 7);
    unsigned int shift = 8 * n;
    uint64_t mask = (n == 0)
        ? ~UINT64_C(0)
        : BITS_BELOW_MASK64(8 * (8 - n));

    r4300_write_aligned_dword(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr & ~UINT32_C(0x7), value >> shift, mask);
}

void write_dword_from_dynarec_SDR(u32 lsaddr, u64 value)
{
    unsigned int n = (lsaddr & 7);
    unsigned int shift = 8 * (7 - n);
    uint64_t mask = BITS_ABOVE_MASK64(8 * (7 - n));

    r4300_write_aligned_dword(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr & ~UINT32_C(0x7), value << shift, mask);
}

void write_hword_from_dynarec(u32 lsaddr, u32 value)
{
    unsigned int shift = hshift(lsaddr);

    r4300_write_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, value << shift, UINT32_C(0xffff) << shift);
}

void write_dword_from_dynarec(u32 lsaddr, u64 value)
{
    r4300_write_aligned_dword(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, value, ~UINT64_C(0));
}

void write_byte_from_dynarec(u32 lsaddr, u32 value)
{
    unsigned int shift = bshift(lsaddr);
    r4300_write_aligned_word(VR4300_Jitter::GetInstance()->GetR4300Core(), lsaddr, value << shift, UINT32_C(0xff) << shift);
}

void VR4300_Jitter::recompile_LW(struct jit_instr *op, bool unsigned_lw)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && (vr4300_jitter_is_rdram_address(address) || (vr4300_jitter_is_mi_regs_address(address)))))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
            RegCache::Realize(Rt);

            if (vr4300_jitter_is_rdram_address(address)) {
                if (unsigned_lw) {
                    MOVZX(64, 32, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
                } else {
                    MOVSX(64, 32, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
                }
                exception_check = false;
            } else if (vr4300_jitter_is_mi_regs_address(address)) {
                MOV(64, R(RSCRATCH), ImmPtr(&VR4300_Jitter::GetInstance()->GetR4300Core()->mi->regs[mi_reg(address)]));
                if (unsigned_lw) {
                    MOVZX(64, 32, Rt, MatR(RSCRATCH));
                } else {
                    MOVSX(64, 32, Rt, MatR(RSCRATCH));
                }
                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::Write);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();

            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~3);
                memory_location = MDisp(RDRAM, address & ~3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                AND(32, R(RSCRATCH), Imm32(~3));
                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            if (unsigned_lw) {
                MOVZX(64, 32, Rt, memory_location);
            } else {
                MOVSX(64, 32, Rt, memory_location);
            }

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_word_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
            if (unsigned_lw) {
                MOVZX(64, 32, Rt, R(ABI_RETURN));
            } else {
                MOVSX(64, 32, Rt, R(ABI_RETURN));
            }
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LWU(struct jit_instr *op)
{
    recompile_LW(op, true);
}

void VR4300_Jitter::recompile_LL(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
            RegCache::Realize(Rt);

            if (vr4300_jitter_is_rdram_address(address)) {
                MOVSX(64, 32, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
                MOV(32, HOTSTATE_VAR(llbit), Imm32(1));
                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::Write);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();

            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~3);
                memory_location = MDisp(RDRAM, address & ~3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                AND(32, R(RSCRATCH), Imm32(~3));
                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            MOVSX(64, 32, Rt, memory_location);
            MOV(32, HOTSTATE_VAR(llbit), Imm32(1));

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_word_from_dynarec_set_llbit_on_success);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
            MOVSX(64, 32, Rt, R(ABI_RETURN));
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LWL(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        bool rt_imm = false;
        u64 rt_imm_value = 0;
        if (m_gpr.IsImm(op->t)) {
            rt_imm = true;
            rt_imm_value = m_gpr.Imm64(op->t);
        }

        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
            u32 base = op->b ? m_gpr.Imm64(op->b) : 0;
            u32 address = base + (u32)(s16)op->f;
            RegCache::Realize(Rt);

            unsigned int n = (address & 3);
            unsigned int shift = 8 * n;
            uint32_t mask = BITS_BELOW_MASK32(8 * n);

            assert(Rt != ABI_RETURN);
            if (vr4300_jitter_is_rdram_address(address)) {
                MOV(32, R(ABI_RETURN), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));

                if (rt_imm) {
                    SHL(32, R(ABI_RETURN), Imm8(shift));
                    OR(32, R(ABI_RETURN), Imm32(rt_imm_value & mask));
                    MOVSX(64, 32, Rt, R(ABI_RETURN));
                } else {
                    AND(32, Rt, Imm32(mask));
                    SHL(32, R(ABI_RETURN), Imm8(shift));
                    OR(32, Rt, R(ABI_RETURN));
                    MOVSX(64, 32, Rt, Rt);
                }
                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::ReadWrite);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(Rb, Rt, scratch);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();
#endif

            OpArg memory_location;
            RCOpArg address_or_address_reg;

#if !DISABLE_FASTMEM
            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~3);
                unsigned int n = (address & 3);
                unsigned int shift = 8 * n;
                uint32_t mask = BITS_BELOW_MASK32(8 * n);

                MOV(32, R(RSCRATCH2), Imm32(shift));
                MOV(32, R(scratch), Imm32(mask));
                memory_location = MDisp(RDRAM, address & ~3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                MOV(32, R(RSCRATCH2), R(RSCRATCH));
                AND(32, R(RSCRATCH), Imm32(~UINT32_C(3)));
                AND(32, R(RSCRATCH2), Imm32(3));
                SHL(32, R(RSCRATCH2), Imm8(3)); /* RSCRATCH2 = shift */

                MOV(32, R(scratch), Imm32(1));
                SHLX(32, scratch, R(scratch), RSCRATCH2);
                SUB(32, R(scratch), Imm32(1)); /* scratch = mask */
                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            MOV(32, R(ABI_RETURN), memory_location);

            SHLX(32, ABI_RETURN, R(ABI_RETURN), RSCRATCH2);
            AND(32, Rt, R(scratch));
            OR(32, Rt, R(ABI_RETURN));
            MOVSX(64, 32, Rt, Rt);

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_word_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            assert(ABI_RETURN != RSCRATCH2 && ABI_RETURN != scratch && Rt != ABI_RETURN && Rt != RSCRATCH2);

            MOV_sum(32, RSCRATCH2, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(RSCRATCH2), Imm32(3));
            SHL(32, R(RSCRATCH2), Imm8(3)); // RSCRATCH2 = shift

            MOV(32, R(scratch), Imm32(1));
            SHLX(32, scratch, R(scratch), RSCRATCH2);
            SUB(32, R(scratch), Imm32(1)); // scratch = mask

            SHLX(32, ABI_RETURN, R(ABI_RETURN), RSCRATCH2);
            AND(32, Rt, R(scratch));
            OR(32, Rt, R(ABI_RETURN));
            MOVSX(64, 32, Rt, Rt);
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LDL(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
            RegCache::Realize(Rt);

            unsigned int n = (address & 7);
            unsigned int shift = 8 * n;
            uint64_t mask = BITS_BELOW_MASK64(8 * n);

            if (vr4300_jitter_is_rdram_address(address)) {
                MOV(64, R(ABI_RETURN), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)));
                ROR(64, R(ABI_RETURN), Imm8(32));
                exception_check = false;
                assert(Rt != ABI_RETURN);

                AND(32, Rt, Imm32(mask));
                SHL(64, R(ABI_RETURN), Imm8(shift));
                OR(64, Rt, R(ABI_RETURN));
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::ReadWrite);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(Rb, Rt, scratch);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();
#endif

#if !DISABLE_FASTMEM
            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~7);
                unsigned int n = (address & 7);
                unsigned int shift = 8 * n;
                uint64_t mask = BITS_BELOW_MASK64(8 * n);

                MOV(32, R(RSCRATCH2), Imm32(shift));
                MOV(32, R(scratch), Imm32(mask));
                memory_location = MDisp(RDRAM, address & ~7);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                MOV(32, R(RSCRATCH2), R(RSCRATCH));
                AND(32, R(RSCRATCH), Imm32(~UINT32_C(7)));

                AND(32, R(RSCRATCH2), Imm32(7));
                SHL(32, R(RSCRATCH2), Imm8(3)); /* RSCRATCH2 = shift */

                MOV(32, R(scratch), Imm32(1));
                SHLX(64, scratch, R(scratch), RSCRATCH2);
                SUB(64, R(scratch), Imm32(1)); /* scratch = mask */
                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            MOV(64, R(ABI_RETURN), memory_location);
            ROR(64, R(ABI_RETURN), Imm8(32));

            SHLX(64, ABI_RETURN, R(ABI_RETURN), RSCRATCH2);
            AND(64, Rt, R(scratch));
            OR(64, Rt, R(ABI_RETURN));

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(ABI_PARAM1), Imm32(~UINT32_C(7)));
            ABI_CallFunction(read_dword_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            assert(ABI_RETURN != RSCRATCH2 && ABI_RETURN != scratch && Rt != ABI_RETURN && Rt != RSCRATCH2);

            MOV_sum(32, RSCRATCH2, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(RSCRATCH2), Imm32(7));
            SHL(32, R(RSCRATCH2), Imm8(3)); // RSCRATCH2 = shift

            MOV(32, R(scratch), Imm32(1));
            SHLX(64, scratch, R(scratch), RSCRATCH2);
            SUB(64, R(scratch), Imm32(1)); // scratch = mask

            SHLX(64, ABI_RETURN, R(ABI_RETURN), RSCRATCH2);
            AND(64, Rt, R(scratch));
            OR(64, Rt, R(ABI_RETURN));
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LWR(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        bool rt_imm = false;
        u64 rt_imm_value = 0;
        if (m_gpr.IsImm(op->t)) {
            rt_imm = true;
            rt_imm_value = m_gpr.Imm64(op->t);
        }

        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
            RegCache::Realize(Rt);

            unsigned int n = (address & 3);
            unsigned int shift = 8 * (3 - n);
            uint32_t mask = (n == 3)
                ? UINT32_C(0)
                : BITS_ABOVE_MASK32(8 * (n + 1));

            if (vr4300_jitter_is_rdram_address(address)) {
                MOV(32, R(ABI_RETURN), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));

                if (rt_imm) {
                    SHR(32, R(ABI_RETURN), Imm8(shift));
                    OR(32, R(ABI_RETURN), Imm32(rt_imm_value & mask));
                    MOVSX(64, 32, Rt, R(ABI_RETURN));
                } else {
                    AND(32, Rt, Imm32(mask));
                    SHR(32, R(ABI_RETURN), Imm8(shift));
                    OR(32, Rt, R(ABI_RETURN));
                    MOVSX(64, 32, Rt, Rt);
                }
                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::ReadWrite);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg scratch = m_gpr.Scratch(), scratch3 = m_gpr.Scratch();
            RegCache::Realize(Rb, Rt, scratch, scratch3);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();
#endif


#if !DISABLE_FASTMEM
            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~3);
                unsigned int n = (address & 3);
                unsigned int shift = 8 * (3 - n);
                uint32_t mask = (n == 3)
                    ? UINT32_C(0)
                    : BITS_ABOVE_MASK32(8 * (n + 1));

                MOV(32, R(scratch3), Imm32(shift));
                AND(32, Rt, Imm32(mask));

                memory_location = MDisp(RDRAM, address & ~3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                MOV(32, R(RSCRATCH2), R(RSCRATCH));
                AND(32, R(RSCRATCH), Imm32(~UINT32_C(3)));

                AND(32, R(RSCRATCH2), Imm32(3)); /* n */

                MOV(32, R(scratch3), Imm32(3));
                SUB(32, R(scratch3), R(RSCRATCH2));
                SHL(32, R(scratch3), Imm8(3)); /* scratch3 = shift */

                ADD(32, R(RSCRATCH2), Imm32(1));
                SHL(32, R(RSCRATCH2), Imm8(3));

                MOV(32, R(scratch), Imm32(1));
                SHLX(32, scratch, R(scratch), RSCRATCH2);
                SUB(32, R(scratch), Imm32(1));
                NOT(32, R(scratch));

                MOV(32, R(RSCRATCH2), Imm32(0));
                TEST(32, R(scratch3), R(scratch3));
                CMOVcc(64, RSCRATCH2, R(scratch), CC_NZ); /* RSCRATCH2 = mask */

                AND(32, Rt, R(RSCRATCH2));
                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            MOV(64, R(ABI_RETURN), memory_location);

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            SHRX(32, ABI_RETURN, R(ABI_RETURN), scratch3);
            OR(32, Rt, R(ABI_RETURN));
            MOVSX(64, 32, Rt, Rt);

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif

            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_word_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            assert(ABI_RETURN != scratch3 && ABI_RETURN != scratch && Rt != ABI_RETURN && ABI_RETURN != RSCRATCH2);
            MOV_sum(32, RSCRATCH2, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(RSCRATCH2), Imm32(3)); // n

            MOV(32, R(scratch3), Imm32(3));
            SUB(32, R(scratch3), R(RSCRATCH2));
            SHL(32, R(scratch3), Imm8(3)); // scratch3 = shift

            ADD(32, R(RSCRATCH2), Imm32(1));
            SHL(32, R(RSCRATCH2), Imm8(3));

            MOV(32, R(scratch), Imm32(1));
            SHLX(32, scratch, R(scratch), RSCRATCH2);
            SUB(32, R(scratch), Imm32(1));
            NOT(32, R(scratch));

            MOV(32, R(RSCRATCH2), Imm32(0));
            TEST(32, R(scratch3), R(scratch3));
            CMOVcc(64, RSCRATCH2, R(scratch), CC_NZ); // RSCRATCH2 = mask

            AND(32, Rt, R(RSCRATCH2));
            SHRX(32, ABI_RETURN, R(ABI_RETURN), scratch3);
            OR(32, Rt, R(ABI_RETURN));
            MOVSX(64, 32, Rt, Rt);
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LDR(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
            RegCache::Realize(Rt);

            unsigned int n = (address & 7);
            unsigned int shift = 8 * (7 - n);
            uint32_t mask = (n == 7)
                ? UINT64_C(0)
                : BITS_ABOVE_MASK64(8 * (n + 1));

            if (vr4300_jitter_is_rdram_address(address)) {
                MOV(64, R(ABI_RETURN), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)));
                ROR(64, R(ABI_RETURN), Imm8(32));
                AND(32, Rt, Imm32(mask));
                SHR(64, R(ABI_RETURN), Imm8(shift));
                OR(64, Rt, R(ABI_RETURN));

                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::ReadWrite);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg scratch = m_gpr.Scratch(), scratch3 = m_gpr.Scratch();
            RegCache::Realize(Rb, Rt, scratch, scratch3);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();
#endif

#if !DISABLE_FASTMEM
            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~7);
                unsigned int n = (address & 7);
                unsigned int shift = 8 * (7 - n);
                uint32_t mask = (n == 7)
                    ? UINT64_C(0)
                    : BITS_ABOVE_MASK64(8 * (n + 1));

                MOV(32, R(scratch3), Imm32(shift));
                AND(64, Rt, Imm32(mask));

                memory_location = MDisp(RDRAM, address & ~7);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                MOV(32, R(scratch), R(RSCRATCH));
                MOV(32, R(RSCRATCH2), R(RSCRATCH));
                AND(32, R(RSCRATCH), Imm32(~UINT32_C(7)));

                AND(32, R(RSCRATCH2), Imm32(7)); /* n */

                MOV(32, R(scratch3), Imm32(7));
                SUB(32, R(scratch3), R(RSCRATCH2));
                SHL(32, R(scratch3), Imm8(3)); /* scratch3 = shift */

                ADD(32, R(RSCRATCH2), Imm32(1));
                SHL(32, R(RSCRATCH2), Imm8(3));

                MOV(32, R(scratch), Imm32(1));
                SHLX(64, scratch, R(scratch), RSCRATCH2);
                SUB(64, R(scratch), Imm32(1));
                NOT(64, R(scratch));

                MOV(32, R(RSCRATCH2), Imm32(0));
                TEST(32, R(scratch3), R(scratch3));
                CMOVcc(64, RSCRATCH2, R(scratch), CC_NZ); /* RSCRATCH2 = mas */

                AND(64, Rt, R(RSCRATCH2));

                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            MOV(64, R(ABI_RETURN), memory_location);
            ROR(64, R(ABI_RETURN), Imm8(32));

            SHRX(64, ABI_RETURN, R(ABI_RETURN), scratch3);
            OR(64, Rt, R(ABI_RETURN));

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(ABI_PARAM1), Imm32(~UINT32_C(7)));
            ABI_CallFunction(read_dword_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            assert(ABI_RETURN != scratch3 && ABI_RETURN != scratch && Rt != ABI_RETURN && ABI_RETURN != RSCRATCH2);
            MOV_sum(32, RSCRATCH2, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(RSCRATCH2), Imm32(7)); // n

            MOV(32, R(scratch3), Imm32(7));
            SUB(32, R(scratch3), R(RSCRATCH2));
            SHL(32, R(scratch3), Imm8(3)); // scratch3 = shift

            ADD(32, R(RSCRATCH2), Imm32(1));
            SHL(32, R(RSCRATCH2), Imm8(3));

            MOV(32, R(scratch), Imm32(1));
            SHLX(64, scratch, R(scratch), RSCRATCH2);
            SUB(64, R(scratch), Imm32(1));
            NOT(64, R(scratch));

            MOV(32, R(RSCRATCH2), Imm32(0));
            TEST(32, R(scratch3), R(scratch3));
            CMOVcc(64, RSCRATCH2, R(scratch), CC_NZ); // RSCRATCH2 = mask

            AND(64, Rt, R(RSCRATCH2));
            SHRX(64, ABI_RETURN, R(ABI_RETURN), scratch3);
            OR(64, Rt, R(ABI_RETURN));
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LH(struct jit_instr *op, bool unsigned_lh)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);

            RegCache::Realize(Rt);

            if (vr4300_jitter_is_rdram_address(address)) {
                if (unsigned_lh) {
                    MOVZX(64, 16, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address((address & ~3) | (((address & 2) ^ 2)))));
                } else {
                    MOVSX(64, 16, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address((address & ~3) | (((address & 2) ^ 2)))));
                }
                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::Write);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();

            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address ^ 2);
                memory_location = MDisp(RDRAM, address ^ 2);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                XOR(32, R(RSCRATCH), Imm32(2));

                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            if (unsigned_lh) {
                MOVZX(64, 16, Rt, memory_location);
            } else {
                MOVSX(64, 16, Rt, memory_location);
            }

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_hword_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            if (!unsigned_lh) {
                MOVSX(64, 16, Rt, R(ABI_RETURN));
            } else {
                MOV(64, Rt, R(ABI_RETURN));
            }
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LHU(struct jit_instr *op)
{
    recompile_LH(op, true);
}

void VR4300_Jitter::recompile_LD(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
            RegCache::Realize(Rt);

            if (vr4300_jitter_is_rdram_address(address)) {
                MOV(64, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
                ROR(64, Rt, Imm8(32));
                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::Write);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();

            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~3);
                memory_location = MDisp(RDRAM, address & ~3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                AND(32, R(RSCRATCH), Imm32(~3));

                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            MOV(64, Rt, memory_location);
            ROR(64, Rt, Imm8(32));

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_dword_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
            MOV(64, Rt, R(ABI_RETURN));
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LB(struct jit_instr *op, bool unsigned_lb)
{
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
            RegCache::Realize(Rt);

            if (vr4300_jitter_is_rdram_address(address)) {
                if (unsigned_lb) {
                    MOVZX(64, 8, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 3)));
                } else {
                    MOVSX(64, 8, Rt, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 3)));
                }
                exception_check = false;
           } else {
                abort();
           }
        } else
#endif
        {
            RCX64Reg Rt = m_gpr.RevertableBind(op->t, RCMode::Write);
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();

            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address ^ 3);
                memory_location = MDisp(RDRAM, address ^ 3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                XOR(32, R(RSCRATCH), Imm32(3));

                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = GetWritableCodePtr();
            if (unsigned_lb) {
                MOVZX(64, 8, Rt, memory_location);
            } else {
                MOVSX(64, 8, Rt, memory_location);
            }

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = true;

            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif

            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV_sum(32, ABI_PARAM1, Rb, Imm32((u32)(s16)op->f));
            ABI_CallFunction(read_byte_unsigned_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            if (!unsigned_lb) {
                MOVSX(64, 8, Rt, R(ABI_RETURN));
            } else {
                MOV(64, Rt, R(ABI_RETURN));
            }
        }

        if (exception_check) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
            if (is_in_far_code()) {
                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);
            }
#endif
        }
    }
}

void VR4300_Jitter::recompile_LBU(struct jit_instr *op)
{
    recompile_LB(op, true);
}

void VR4300_Jitter::recompile_SW(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && (vr4300_jitter_is_rdram_address(address) || (vr4300_jitter_is_mi_regs_address(address)))))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        if (vr4300_jitter_is_rdram_address(address)) {
            if (Rt.IsImm()) {
                MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Imm32(Rt.Imm64()));
            } else {
                if (Rt.IsSimpleReg()) {
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Rt);
                } else {
                    MOV(32, R(RSCRATCH), Rt);
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH));
                }
            }

            compile_invalidate_code_constaddress(address & ~3);

            exception_check = false;
        } else if (vr4300_jitter_is_rsp_regs_address(address)) {
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV(64, R(ABI_PARAM1), ImmPtr(&g_dev.sp));
            MOV(64, R(ABI_PARAM2), Imm32(address & ~3));
            MOV(64, R(ABI_PARAM3), Rt);
            MOV(64, R(ABI_PARAM4), Imm32(0xffffffff));
            ABI_CallFunction(write_rsp_regs);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
        } else if (vr4300_jitter_is_mi_regs_address(address)) {
            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV(64, R(ABI_PARAM1), ImmPtr(m_r4300->mi));
            MOV(64, R(ABI_PARAM2), Imm32(address & ~3));
            MOV(64, R(ABI_PARAM3), Rt);
            MOV(64, R(ABI_PARAM4), Imm32(0xffffffff));
            ABI_CallFunction(write_mi_regs);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratch = m_gpr.Scratch();
        RegCache::Realize(Rb, Rt, scratch);

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address & ~3);
            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(RSCRATCH), Imm32(~3));
            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        u8 *mov_address = nullptr;
        if (Rt.IsImm()) {
            mov_address = GetWritableCodePtr();
            MOV(32, memory_location, Imm32(Rt.Imm64()));
        } else {
            if (Rt.IsSimpleReg()) {
                mov_address = GetWritableCodePtr();
                MOV(32, memory_location, Rt);
            } else {
                MOV(32, R(RSCRATCH2), Rt);
                mov_address = GetWritableCodePtr();
                MOV(32, memory_location, R(RSCRATCH2));
            }
        }

        compile_invalidate_code(address_or_address_reg, 0, scratch);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif
        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(32, ABI_PARAM1, 32, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_word_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SC(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    if (op->t) {
        RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
        RegCache::Realize(Rt);

        CMP(32, HOTSTATE_VAR(llbit), Imm32(0));
        FixupBranch llbit_not_set = J_CC(CC_E, XEmitter::Jump::Near);

        u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
        u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
        if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
            if (vr4300_jitter_is_rdram_address(address)) {
                MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Rt);
                MOV(32, HOTSTATE_VAR(llbit), Imm32(0));
                MOV(32, Rt, Imm32(1));
                compile_invalidate_code_constaddress(address & ~3);

                exception_check = false;
            } else {
                abort();
            }
        } else
#endif
        {
            RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(Rb, Rt, scratch);

#if !DISABLE_FASTMEM
            u8 *code_before = GetWritableCodePtr();

            OpArg memory_location;
            RCOpArg address_or_address_reg;

            if (m_gpr.IsImm(op->b)) {
                address_or_address_reg = RCOpArg::Imm64(address & ~3);
                memory_location = MDisp(RDRAM, address & ~3);
            } else {
                MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
                AND(32, R(RSCRATCH), Imm32(~3));
                address_or_address_reg = RCOpArg::R(RSCRATCH);
                memory_location = MRegSum(RDRAM, RSCRATCH);
            }

            u8 *mov_address = nullptr;
            mov_address = GetWritableCodePtr();
            MOV(32, memory_location, Rt);

            MOV(32, HOTSTATE_VAR(llbit), Imm32(0));
            MOV(32, Rt, Imm32(1));

            compile_invalidate_code(address_or_address_reg, 0, scratch);

            BackPatchInfo &info = m_back_patch_info[mov_address];
            info.code_after = GetCodePtr();
            info.code_before = code_before;
            info.read = false;
            switch_to_far_code();
            info.farcode = GetCodePtr();
#endif

            update_hot_cycles(op);

            BitSet32 registers_in_use = caller_saved_registers_in_use();
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            mov_2(32, ABI_PARAM1, 32, ABI_PARAM2, Rb, RCOpArg::R(Rt));
            if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
            ABI_CallFunction(write_word_from_dynarec);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

            TEST(32, R(ABI_RETURN), R(ABI_RETURN));
            FixupBranch dont_clear_llbit = J_CC(CC_E);
            MOV(32, HOTSTATE_VAR(llbit), Imm32(0));
            MOV(32, Rt, Imm32(1));
            SetJumpTarget(dont_clear_llbit);
        }

        FixupBranch exit = J();
        SetJumpTarget(llbit_not_set);
        MOV(32, Rt, Imm32(0));
        SetJumpTarget(exit);
    } else {
        exception_check = false;
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SWL(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        unsigned int n = (address & 3);
        unsigned int shift = 8 * n;
        uint32_t mask = (n == 0)
            ? ~UINT32_C(0)
            : BITS_BELOW_MASK32(8 * (4 - n));

        if (vr4300_jitter_is_rdram_address(address)) {
            if (mask != 0xffffffff || shift != 0) {
                MOV(32, R(RSCRATCH2), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
                AND(32, R(RSCRATCH2), Imm32(~mask));
                if (Rt.IsImm()) {
                    MOV(32, R(RSCRATCH), Imm32((Rt.Imm64() >> shift) & mask));
                } else {
                    MOV(32, R(RSCRATCH), Rt);
                    SHR(32, R(RSCRATCH), Imm8(shift));
                    AND(32, R(RSCRATCH), Imm32(mask));
                }
                OR(32, R(RSCRATCH2), R(RSCRATCH));
                MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH2));
            } else {
                if (Rt.IsImm()) {
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Imm32(Rt.Imm64()));
                } else if (!Rt.IsSimpleReg()) {
                    MOV(32, R(RSCRATCH2), Rt);
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH2));
                } else {
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Rt);
                }
            }

            compile_invalidate_code_constaddress(address & ~3);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rb, Rt);
#if !DISABLE_FASTMEM
        RCX64Reg scratch = m_gpr.Scratch();
        RCX64Reg scratch3 = m_gpr.Scratch();
        RCX64Reg scratchinval = m_gpr.Scratch();
        RegCache::Realize(scratch, scratch3, scratchinval);

        u8 *code_before = GetWritableCodePtr();
#endif

#if !DISABLE_FASTMEM
        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address & ~3);

            unsigned int n = (address & 3);
            unsigned int shift = 8 * n;
            uint32_t mask = (n == 0)
                ? ~UINT32_C(0)
                : BITS_BELOW_MASK32(8 * (4 - n));

            MOV(32, R(scratch3), Imm32(shift));
            MOV(32, R(RSCRATCH2), Imm32(mask));

            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            MOV_sum(32, scratch3, Rb, Imm32((u32)(s16)op->f));
            MOV(32, R(RSCRATCH), R(scratch3));
            AND(32, R(RSCRATCH), Imm32(~UINT32_C(3)));
            AND(32, R(scratch3), Imm32(UINT32_C(3))); /* n */

            MOV(32, R(RSCRATCH2), Imm32(4));
            SUB(32, R(RSCRATCH2), R(scratch3));
            SHL(32, R(RSCRATCH2), Imm8(3));

            MOV(32, R(scratch), Imm32(1));
            SHLX(32, scratch, R(scratch), RSCRATCH2);
            SUB(32, R(scratch), Imm32(1));

            MOV(32, R(RSCRATCH2), Imm32(0xffffffff));
            TEST(32, R(scratch3), R(scratch3));
            CMOVcc(64, RSCRATCH2, R(scratch), CC_NZ); /* RSCRATCH2 = mask */

            SHL(32, R(scratch3), Imm8(3)); /* R(scratch3) = shift */

            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        if (!Rt.IsSimpleReg()) {
            MOV(64, R(scratch), Rt);
            SHRX(64, scratch3, R(scratch), scratch3);
        } else {
            SHRX(64, scratch3, Rt, scratch3);
        }

        u8 *mov_address = GetWritableCodePtr();
        MOV(32, R(scratch), memory_location);
        AND(32, R(scratch3), R(RSCRATCH2));
        NOT(32, R(RSCRATCH2));
        AND(32, R(scratch), R(RSCRATCH2));
        OR(32, R(scratch3), R(scratch));
        MOV(32, memory_location, R(scratch3));

        compile_invalidate_code(address_or_address_reg, 0, scratchinval);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif

        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(32, ABI_PARAM1, 32, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_word_from_dynarec_SWL);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SDL(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        unsigned int n = (address & 7);
        unsigned int shift = 8 * n;
        uint64_t mask = (n == 0)
            ? ~UINT64_C(0)
            : BITS_BELOW_MASK64(8 * (8 - n));

        if (vr4300_jitter_is_rdram_address(address)) {
            if (mask != 0xffffffffffffffff || shift != 0) {
                MOV(64, R(RSCRATCH2), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)));
                ROR(64, R(RSCRATCH2), Imm8(32));
                AND(32, R(RSCRATCH2), Imm32(~mask));
                if (Rt.IsImm()) {
                    MOV(64, R(RSCRATCH), Imm64((Rt.Imm64() >> shift) & mask));
                    OR(64, R(RSCRATCH2), R(RSCRATCH));
                    ROR(64, R(RSCRATCH2), Imm8(32));
                    MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                } else {
                    MOV(64, R(RSCRATCH), Rt);
                    SHR(64, R(RSCRATCH), Imm8(shift));
                    AND(32, R(RSCRATCH), Imm32(mask));
                    OR(64, R(RSCRATCH2), R(RSCRATCH));
                    ROR(64, R(RSCRATCH2), Imm8(32));
                    MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                }
            } else {
                if (Rt.IsImm()) {
                    if (vr4300_jitter_value_fits_in_32_bit_imm_positive(Rt.Imm64())) {
                        u64 val = Rt.Imm64();
                        // untested
                        MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address((address & ~7) + 4)), Imm32(val & 0xffffffff));
                        MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), Imm32(0));
                    } else {
                        MOV(64, R(RSCRATCH2), Rt);
                        ROR(64, R(RSCRATCH2), Imm8(32));
                        MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                    }
                } else {
                    MOV(64, R(RSCRATCH2), Rt);
                    ROR(64, R(RSCRATCH2), Imm8(32));
                    MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                }
            }

            compile_invalidate_code_constaddress(address & ~7);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
        RCX64Reg scratch = m_gpr.Scratch();
        RCX64Reg scratch3 = m_gpr.Scratch();
        RCX64Reg scratchinval = m_gpr.Scratch();
        RegCache::Realize(scratch, scratch3, scratchinval);
#endif

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();
#endif


#if !DISABLE_FASTMEM
        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address & ~3);
            unsigned int n = (address & 7);
            unsigned int shift = 8 * n;
            uint64_t mask = (n == 0)
                ? ~UINT64_C(0)
                : BITS_BELOW_MASK64(8 * (8 - n));

            MOV(32, R(scratch3), Imm32(shift));
            MOV(32, R(RSCRATCH2), Imm32(mask));

            memory_location = MDisp(RDRAM, address & ~7);
        } else {
            MOV_sum(32, scratch3, Rb, Imm32((u32)(s16)op->f));
            MOV(32, R(RSCRATCH), R(scratch3));
            AND(32, R(RSCRATCH), Imm32(~UINT32_C(7)));
            AND(32, R(scratch3), Imm32(UINT32_C(7))); /* n */

            MOV(32, R(RSCRATCH2), Imm32(8));
            SUB(64, R(RSCRATCH2), R(scratch3));
            SHL(64, R(RSCRATCH2), Imm8(3));

            MOV(32, R(scratch), Imm32(1));
            SHLX(64, scratch, R(scratch), RSCRATCH2);
            SUB(64, R(scratch), Imm32(1));

            MOV(64, R(RSCRATCH2), Imm64(0xffffffffffffffff));
            TEST(64, R(scratch3), R(scratch3));
            CMOVcc(64, RSCRATCH2, R(scratch), CC_NZ); /* RSCRATCH2 = mask */

            SHL(64, R(scratch3), Imm8(3)); /* R(scratch3) = shift */

            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        if (!Rt.IsSimpleReg()) {
            MOV(64, R(scratch), Rt);
            SHRX(64, scratch3, R(scratch), scratch3);
        } else {
            SHRX(64, scratch3, Rt, scratch3);
        }

        u8 *mov_address = GetWritableCodePtr();
        MOV(64, R(scratch), memory_location);
        ROR(64, R(scratch), Imm8(32));
        AND(64, R(scratch3), R(RSCRATCH2));
        NOT(64, R(RSCRATCH2));
        AND(64, R(scratch), R(RSCRATCH2));
        OR(64, R(scratch3), R(scratch));
        ROR(64, R(scratch3), Imm8(32));
        MOV(64, memory_location, R(scratch3));

        compile_invalidate_code(address_or_address_reg, 0, scratchinval);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif

        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(64, ABI_PARAM1, 64, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_dword_from_dynarec_SDL);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SDR(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        unsigned int n = (address & 7);
        unsigned int shift = 8 * (7 - n);
        uint64_t mask = BITS_ABOVE_MASK64(8 * (7 - n));

        if (vr4300_jitter_is_rdram_address(address)) {
            if (mask != 0xffffffffffffffff || shift != 0) {
                MOV(64, R(RSCRATCH2), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)));
                AND(32, R(RSCRATCH2), Imm32(~mask));
                if (Rt.IsImm()) {
                    if (vr4300_jitter_value_fits_in_32_bit_imm_positive((Rt.Imm64() << shift) & mask)) {
                        OR(64, R(RSCRATCH2), Imm32((Rt.Imm64() << shift) & mask));
                        ROR(64, R(RSCRATCH2), Imm8(32));
                        MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                    } else {
                        MOV(64, R(RSCRATCH), Imm64((Rt.Imm64() << shift) & mask));
                        OR(64, R(RSCRATCH2), R(RSCRATCH));
                        ROR(64, R(RSCRATCH2), Imm8(32));
                        MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                    }
                } else {
                    MOV(64, R(RSCRATCH), Rt);
                    SHL(64, R(RSCRATCH), Imm8(shift));
                    AND(32, R(RSCRATCH), Imm32(mask));
                    OR(64, R(RSCRATCH2), R(RSCRATCH));
                    ROR(64, R(RSCRATCH2), Imm8(32));
                    MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
                }
            } else {
                MOV(64, R(RSCRATCH2), Rt);
                ROR(64, R(RSCRATCH2), Imm8(32));
                MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~7)), R(RSCRATCH2));
            }

            compile_invalidate_code_constaddress(address & ~7);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rb, Rt);
#if !DISABLE_FASTMEM
        RCX64Reg scratch = m_gpr.Scratch();
        RCX64Reg scratch3 = m_gpr.Scratch();
        RCX64Reg scratchinval = m_gpr.Scratch();
        RegCache::Realize(scratch, scratch3, scratchinval);
#endif

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();
#endif

#if !DISABLE_FASTMEM
        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address & ~7);

            unsigned int n = (address & 7);
            unsigned int shift = 8 * (7 - n);
            uint64_t mask = BITS_ABOVE_MASK64(8 * (7 - n));

            MOV(32, R(scratch), Imm32(mask));
            MOV(32, R(RSCRATCH2), Imm32(shift));

            memory_location = MDisp(RDRAM, address & ~7);
        } else {
            MOV_sum(32, scratch3, Rb, Imm32((u32)(s16)op->f));
            MOV(32, R(RSCRATCH), R(scratch3));
            AND(32, R(RSCRATCH), Imm32(~UINT32_C(7)));
            AND(32, R(scratch3), Imm32(UINT32_C(7))); /* n */

            MOV(32, R(RSCRATCH2), Imm32(7));
            SUB(64, R(RSCRATCH2), R(scratch3));
            SHL(64, R(RSCRATCH2), Imm8(3)); /* RSCRATCH2 = 8 * (7 - n) == shift */

            MOV(32, R(scratch), Imm32(1));
            SHLX(64, scratch, R(scratch), RSCRATCH2);
            SUB(64, R(scratch), Imm32(1));
            NOT(64, R(scratch)); /* scratch = BITS_ABOVE_MASK64(8 * (7 - n)) == mask */

            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        if (!Rt.IsSimpleReg()) {
            MOV(64, R(scratch3), Rt);
            SHLX(64, RSCRATCH2, R(scratch3), RSCRATCH2);
        } else {
            SHLX(64, RSCRATCH2, Rt, RSCRATCH2);
        }

        u8 *mov_address = GetWritableCodePtr();

        MOV(64, R(scratch3), memory_location); // scratch3 == dst
        ROR(64, R(scratch3), Imm8(32));
        AND(64, R(RSCRATCH2), R(scratch));
        NOT(64, R(scratch));
        AND(64, R(scratch3), R(scratch));
        OR(64, R(scratch3), R(RSCRATCH2));
        ROR(64, R(scratch3), Imm8(32));
        MOV(64, memory_location, R(scratch3));

        compile_invalidate_code(address_or_address_reg, 0, scratchinval);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif
        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(64, ABI_PARAM1, 64, ABI_PARAM3, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_dword_from_dynarec_SDR);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SWR(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        unsigned int n = (address & 3);
        unsigned int shift = 8 * (3 - n);
        uint32_t mask = BITS_ABOVE_MASK32(8 * (3 - n));

        if (vr4300_jitter_is_rdram_address(address)) {
            if (mask != 0xffffffff || shift != 0) {
                MOV(32, R(RSCRATCH2), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
                AND(32, R(RSCRATCH2), Imm32(~mask));
                if (Rt.IsImm()) {
                    OR(32, R(RSCRATCH2), Imm32((Rt.Imm64() >> shift) & mask));
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH2));
                } else {
                    MOV(32, R(RSCRATCH), Rt);
                    SHL(32, R(RSCRATCH), Imm8(shift));
                    AND(32, R(RSCRATCH), Imm32(mask));
                    OR(32, R(RSCRATCH2), R(RSCRATCH));
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH2));
                }
            } else {
                if (Rt.IsImm()) {
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Imm32((Rt.Imm64() >> shift) & mask));
                } else if (!Rt.IsSimpleReg()) {
                    MOV(32, R(RSCRATCH2), Rt);
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH2));
                } else {
                    MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), Rt);
                }
            }

            compile_invalidate_code_constaddress(address & ~3);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
#if !DISABLE_FASTMEM
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratch = m_gpr.Scratch(), scratch3 = m_gpr.Scratch();
        RCX64Reg scratchinval = m_gpr.Scratch();
        RegCache::Realize(Rb, Rt, scratch, scratch3, scratchinval);
#else
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rb, Rt);
#endif

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();
#endif

#if !DISABLE_FASTMEM
        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address & ~3);

            unsigned int n = (address & 3);
            unsigned int shift = 8 * (3 - n);
            uint32_t mask = BITS_ABOVE_MASK32(8 * (3 - n));

            MOV(32, R(scratch3), Imm32(shift));
            MOV(32, R(scratch), Imm32(mask));

            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            MOV_sum(32, RSCRATCH2, Rb, Imm32((u32)(s16)op->f));
            MOV(32, R(RSCRATCH), R(RSCRATCH2));

            AND(32, R(RSCRATCH2), Imm32(UINT32_C(3))); /* n */
            MOV(32, R(scratch3), Imm32(3));
            SUB(32, R(scratch3), R(RSCRATCH2));
            SHL(32, R(scratch3), Imm8(3)); /* scratch3 == shift */

            MOV(32, R(scratch), Imm32(1));
            SHLX(32, scratch, R(scratch), scratch3);
            SUB(32, R(scratch), Imm32(1));
            NOT(32, R(scratch)); /* scratch == mask */

            AND(32, R(RSCRATCH), Imm32(~3));

            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        if (!Rt.IsSimpleReg()) {
            MOV(64, R(RSCRATCH2), Rt);
            SHLX(64, scratch3, R(RSCRATCH2), scratch3);
        } else {
            SHLX(64, scratch3, Rt, scratch3);
        }

        u8 *mov_address = GetWritableCodePtr();
        MOV(32, R(RSCRATCH2), memory_location);
        AND(32, R(scratch3), R(scratch));
        NOT(32, R(scratch));
        AND(32, R(RSCRATCH2), R(scratch));
        OR(32, R(scratch3), R(RSCRATCH2));
        MOV(32, memory_location, R(scratch3));

        compile_invalidate_code(address_or_address_reg, 0, scratchinval);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif

        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(64, ABI_PARAM1, 32, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_word_from_dynarec_SWR);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::compile_exception_general(struct jit_instr *op)
{
    SUB(32, HOTSTATE_VAR(cycle_count), Imm32(m_r4300->cp0.count_per_op));
    update_cycle_count(op, true);
    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

    OR(32, HOTSTATE_CP0REG(CP0_STATUS_REG), Imm32(CP0_STATUS_EXL));

    if (HOT_STATE->inDelaySlot) {
        OR(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_BD));
        MOV(32, HOTSTATE_CP0REG(CP0_EPC_REG), Imm32(op->address - 4));
    } else {
        MOV(32, HOTSTATE_CP0REG(CP0_EPC_REG), Imm32(op->address));
        AND(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(~CP0_CAUSE_BD));
    }

    MOV(32, HOTSTATE_VAR(pc), Imm32(op->address));

    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        compile_goto_dispatcher(op, 0x80000180, false);
    }
}

void VR4300_Jitter::compile_cop1_usable_check(struct jit_instr *op)
{
    TEST(32, HOTSTATE_CP0REG(CP0_STATUS_REG), Imm32(CP0_STATUS_CU1));
    FixupBranch unusable = J_CC(CC_Z, XEmitter::Jump::Near);

    switch_to_far_code();
    SetJumpTarget(unusable);

    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_CPU | CP0_CAUSE_CE1));

    compile_exception_general(op);

    FixupBranch near_code = J(XEmitter::Jump::Near);
    switch_to_near_code();
    SetJumpTarget(near_code);
}

void VR4300_Jitter::compile_cop2_usable_check(struct jit_instr *op)
{
    TEST(32, HOTSTATE_CP0REG(CP0_STATUS_REG), Imm32(CP0_STATUS_CU2));
    FixupBranch unusable = J_CC(CC_Z, XEmitter::Jump::Near);

    switch_to_far_code();
    SetJumpTarget(unusable);

    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_CPU | CP0_CAUSE_CE2));

    compile_exception_general(op);

    FixupBranch near_code = J(XEmitter::Jump::Near);
    switch_to_near_code();
    SetJumpTarget(near_code);
}

void VR4300_Jitter::recompile_SB(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        if (vr4300_jitter_is_rdram_address(address)) {
            if (Rt.IsSimpleReg()) {
                MOV(8, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 3)), Rt);
            } else if(Rt.IsImm()) {
                MOV(8, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 3)), Imm8(Rt.Imm64()));
            } else {
                MOV(8, R(RSCRATCH2), Rt);
                MOV(8, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 3)), R(RSCRATCH2));
            }

            compile_invalidate_code_constaddress(address ^ 3);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratchinval = m_gpr.Scratch();
        RegCache::Realize(Rb, Rt, scratchinval);

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address ^ 3);
            memory_location = MDisp(RDRAM, address ^ 3);
        } else {
            MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
            XOR(8, R(RSCRATCH), Imm8(3));

            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }
        u8 *mov_address;
        if (Rt.IsSimpleReg()) {
            mov_address = GetWritableCodePtr();
            MOV(8, memory_location, Rt);
        } else if (Rt.IsImm()) {
            mov_address = GetWritableCodePtr();
            MOV(8, memory_location, Imm8(Rt.Imm64()));
        } else {
            MOV(8, R(RSCRATCH2), Rt);
            mov_address = GetWritableCodePtr();
            MOV(8, memory_location, R(RSCRATCH2));
        }

        compile_invalidate_code(Rb, (s32)(s16)op->f, scratchinval);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif
        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(32, ABI_PARAM1, 32, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_byte_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SH(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        if (vr4300_jitter_is_rdram_address(address)) {
            if (Rt.IsSimpleReg()) {
                MOV(16, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 2)), Rt);
            } else if(Rt.IsImm()) {
                MOV(16, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 2)), Imm16(Rt.Imm64()));
            } else {
                MOV(16, R(RSCRATCH2), Rt);
                MOV(16, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address ^ 2)), R(RSCRATCH2));
            }

            compile_invalidate_code_constaddress(address ^ 2);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratchinval = m_gpr.Scratch();
        RegCache::Realize(Rb, Rt, scratchinval);

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address ^ 2);
            memory_location = MDisp(RDRAM, address ^ 2);
        } else {
            MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
            XOR(32, R(RSCRATCH), Imm8(2));
            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        u8 *mov_address = nullptr;
        if (Rt.IsSimpleReg()) {
            mov_address = GetWritableCodePtr();
            MOV(16, memory_location, Rt);
        } else if(Rt.IsImm()) {
            mov_address = GetWritableCodePtr();
            MOV(16, memory_location, Imm16(Rt.Imm64()));
        } else {
            MOV(16, R(RSCRATCH2), Rt);
            mov_address = GetWritableCodePtr();
            MOV(16, memory_location, R(RSCRATCH2));
        }

        compile_invalidate_code(address_or_address_reg, 0, scratchinval);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif

        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(32, ABI_PARAM1, 32, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_hword_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

void VR4300_Jitter::recompile_SD(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
#if !DISABLE_RDRAM_OPTIMIZATION
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        if (vr4300_jitter_is_rdram_address(address)) {
            MOV(64, R(RSCRATCH), Rt);
            ROR(64, R(RSCRATCH), Imm8(32));
            MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(RSCRATCH));

            compile_invalidate_code_constaddress(address & ~3);

            exception_check = false;
        } else {
            abort();
        }
    } else
#endif
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rb, Rt);

#if !DISABLE_FASTMEM
        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;
        RCOpArg address_or_address_reg;

        if (m_gpr.IsImm(op->b)) {
            address_or_address_reg = RCOpArg::Imm64(address & ~3);
            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            MOV_sum(32, RSCRATCH, Rb, Imm32((u32)(s16)op->f));
            AND(32, R(RSCRATCH), Imm32(~3));

            address_or_address_reg = RCOpArg::R(RSCRATCH);
            memory_location = MRegSum(RDRAM, RSCRATCH);
        }

        MOV(64, R(RSCRATCH2), Rt);
        ROR(64, R(RSCRATCH2), Imm8(32));
        u8 *mov_address = GetWritableCodePtr();
        MOV(64, memory_location, R(RSCRATCH2));

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();
#endif
        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(64, ABI_PARAM1, 64, ABI_PARAM2, Rb, Rt);
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_dword_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);

#if !DISABLE_FASTMEM
        if (is_in_far_code()) {
            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
#endif
    }
}

