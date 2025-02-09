/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter.cpp                                       *
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

#include "vr4300_jitter_internal.h"
#include "vr4300_jitter.h"
#include "api/callbacks.h"
#include "main/main.h"
#include "main/rom.h"
#include "device/memory/memory.h"
#include "device/r4300/cached_interp.h"
#include "device/r4300/cp0.h"
#include "device/r4300/cp1.h"
#include "device/r4300/interrupt.h"
#include "device/r4300/tlb.h"
#include "device/r4300/fpu.h"
#include "device/rcp/mi/mi_controller.h"
#include "device/rcp/rsp/rsp_core.h"

#include <signal.h>
#include <bitset>
#include <unordered_set>

#include "regs.h"

#include <set>
#include <map>
#include <list>

#include "Common/CPUDetect.h"
#include "Common/x64Emitter.h"
#include "JitCache.h"
#include "Common/JitRegister.h"
#include "rangeset/include/rangeset/rangesizeset.h"

#include "vr4300_jitter_instruction_decoder.h"

#include "RegCache/GPRRegCache.h"
#include "RegCache/FPURegCache.h"

#include "ConstantPool.h"

#include <assert.h>

VR4300_Jitter *gJitterInstance = nullptr;

#include <sys/types.h>
#if defined(__APPLE__)
#define MAP_ANONYMOUS MAP_ANON
#endif

#if !defined(WIN32)
#include <sys/mman.h>
#endif

#include <signal.h>

extern "C" void CoreCompareCallback(void);

struct prepared_code_block {
    jit_instr instr[MAX_INSTR_PER_BLOCK];
    bool stopped_early;
    bool exception;
    int num_instructions;
    bool is_idle_wait_loop;
    u32 branch_to;
    bool modifies_count_reg;
    bool modifies_status_reg;
    bool crosses_page;
    bool exceptionless_block;
};

using namespace Gen;

void JitCache::WriteLinkBlock(const JitBlock::LinkData& source, const JitBlock* dest)
{
    u8* location = source.exitPtrs;
    const u8* address = dest ? dest->host_entry : VR4300_Jitter::GetInstance()->GetDispatcherStart();

    if (source.call) {
        Gen::XEmitter emit(location, location + 5);
        emit.CALL(address);
    } else {
        // If we're going to link with the next block, there is no need
        // to emit JMP. So just NOP out the gap to the next block.
        // Support up to 3 additional bytes because of alignment.
        s64 offset = address - location;
        if (offset > 0 && offset <= 5 + 3)
        {
            Gen::XEmitter emit(location, location + offset);
            emit.NOP(offset);
        }
        else
        {
            Gen::XEmitter emit(location, location + 5);
            emit.JMP(address, Gen::XEmitter::Jump::Near);
        }
    }
}

#if DEBUG_PREDICTIONS
static void vr4300_jitter_mispredicted_return(uint32_t m, int i)
{
    DebugMessage(M64MSG_VERBOSE, "Mispredict %d %x %x\n", i, m, HOT_STATE->pc);
    if (m == HOT_STATE->pc) abort();
}

static void vr4300_jitter_successfully_predicted_return(int m)
{
    DebugMessage(M64MSG_VERBOSE, "Predict %d\n", m);
}
#endif

static void PrintSomething(char *s, uint64_t x) {
    bool is_set = VR4300_Jitter::GetInstance()->ValidBlockIsSetVirtual(x);
    fprintf(stderr, "SKIP: PRINTING SOMETHING %s %ld %lx %x %d\n", s, x, x, HOT_STATE->pc, is_set);
}

#define PRINT_SOMETHING(s, x) \
    do { \
        BitSet32 registers_in_use = BitSet32::AllTrue(32); \
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
        MOV(64, R(ABI_PARAM1), ImmPtr(s)); \
        MOV(64, R(ABI_PARAM2), x); \
        ABI_CallFunction(PrintSomething); \
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
    } while(0)

int skipped_log = true;

extern "C" int is_skip_log()
{
    return skipped_log;
}

static long long int log_skip = PRINT_IM_HERE_DEBUG_SKIP;

extern "C" int skip_log(void) {
#if PRINT_IM_HERE_DEBUG
    if (log_skip > 0) {
        log_skip--;
        return true;
    }
    skipped_log = false;
    return false;
#else
    return true;
#endif
}

static void ImHereBlockStart(u64 p)
{
    bool is_set = VR4300_Jitter::GetInstance()->ValidBlockIsSetVirtual(p);
    DebugMessage(M64MSG_VERBOSE, "BLOCK START %x %d\n", p, is_set);
}

#define COUNT_INSTRUCTIONS 0
#if COUNT_INSTRUCTIONS
static long long int instrcount = 0;
#endif

#define PRINT_HAS(v, instr, h) if (instr.has_ ##h) fprintf(v, "SKIP: " #h ": %d\n", instr.h)

static void ImHere(u32 address, u32 instruction, char *name, u32 physical_address)
{
    if (skipped_log) return;

    //DebugMessage(M64MSG_VERBOSE, "PADDR 0x%x: %s\n", physical_address, name);
    fprintf(stderr, "0x%x: %s\n", address, name);
    fprintf(stderr, "cycle_count: %d\n", HOT_STATE->cycle_count);
    uint32_t* cp0_regs = r4300_cp0_regs(&g_dev.r4300.cp0);
    fprintf(stderr, "count_reg: %d\n", cp0_regs[CP0_COUNT_REG]);
    fprintf(stderr, "hi: %ld\n", *r4300_mult_hi(&g_dev.r4300));
    fprintf(stderr, "lo: %ld\n", *r4300_mult_lo(&g_dev.r4300));
    struct jit_instr instr;
    VR4300_Jitter::GetInstance()->AnalyzeInstruction(&instr, instruction, address);

    PRINT_HAS(stderr, instr, a);
    PRINT_HAS(stderr, instr, b);
    PRINT_HAS(stderr, instr, c);
    PRINT_HAS(stderr, instr, d);
    PRINT_HAS(stderr, instr, f);
    PRINT_HAS(stderr, instr, k);
    PRINT_HAS(stderr, instr, s);
    PRINT_HAS(stderr, instr, t);
    PRINT_HAS(stderr, instr, x);
}

static void print_cp0_regs(void)
{
    if (!skipped_log) {
        uint32_t* cp0_regs = r4300_cp0_regs(&g_dev.r4300.cp0);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 8; j++) {
                int k = i * 8 + j;
                fprintf(stderr, "CP0(%2d(%2x)): 0x%8x, ", k, k, cp0_regs[k]);
            }
            fprintf(stderr, "\n");
        }
    }
}

static void print_cp1_regs(void)
{
#if PRINT_CP1_REGS
    if (!skipped_log) {
        cp1_reg* cp1_regs = (cp1_reg*)HOT_STATE->fprs_tmp_ptr;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 8; j++) {
                int k = i * 8 + j;
                fprintf(stderr, "CP1(%2d(%2x)): 0x%16lx, ", k, k, cp1_regs[k].dword);
            }
            fprintf(stderr, "\n");
        }
    }
#endif
}

static void print_gpr_reg(uint64_t val, uint64_t reg)
{
    if (!skipped_log) {
        fprintf(stderr, "%2d(%2x): 0x%16lx, ", (int)reg, (int)reg, val);
    }
}

static void print_newline()
{
    if (!skipped_log) {
        fprintf(stderr, "\n");
    }
}

#if PRINT_IM_HERE_DEBUG_NO_REGS
#define PRINT_IM_HERE(x, y, n, p) \
    do { \
        BitSet32 registers_in_use = caller_saved_registers_in_use(); \
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
        MOV(64, R(ABI_PARAM1), x); \
        MOV(32, R(ABI_PARAM2), y); \
        MOV(64, R(ABI_PARAM3), n); \
        MOV(64, R(ABI_PARAM4), p); \
        ABI_CallFunction(ImHere); \
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
    } while(0)
#else
#define PRINT_IM_HERE(x, y, n, p) \
    do { \
        BitSet32 registers_in_use = caller_saved_registers_in_use(); \
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
        MOV(64, R(ABI_PARAM1), x); \
        MOV(32, R(ABI_PARAM2), y); \
        MOV(64, R(ABI_PARAM3), n); \
        MOV(64, R(ABI_PARAM4), p); \
        ABI_CallFunction(ImHere); \
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
        for (int i = 0; i < 4; i++) { \
            for (int j = 0; j < 8; j++) { \
                int k = i * 8 + j; \
                if (m_gpr.IsBound(k) && k && m_gpr.IsDirty(k)) { \
                    RCOpArg reg = (k != 0) ? m_gpr.Use(k, RCMode::Read) : RCOpArg::Imm64(0); \
                    RegCache::Realize(reg); \
                    BitSet32 registers_in_use2 = caller_saved_registers_in_use(); \
                    ABI_PushRegistersAndAdjustStack(registers_in_use2, 0); \
                    if (!reg.IsSimpleReg(ABI_PARAM1)) { \
                        MOV(64, R(ABI_PARAM1), reg); \
                    } \
                    /*MOV(64, R(ABI_PARAM1), Imm32(reg.GetSimpleReg())); */ \
                    MOV(64, R(ABI_PARAM2), Imm32(k)); \
                    ABI_CallFunction(print_gpr_reg); \
                    ABI_PopRegistersAndAdjustStack(registers_in_use2, 0); \
                } else if (k && !m_gpr.IsDiscarded(k) && m_gpr.IsImm(k)) { \
                    BitSet32 registers_in_use2 = caller_saved_registers_in_use(); \
                    ABI_PushRegistersAndAdjustStack(registers_in_use2, 0); \
                    MOV(64, R(ABI_PARAM1), Imm64(m_gpr.Imm64(k))); \
                    MOV(64, R(ABI_PARAM2), Imm32(k)); \
                    ABI_CallFunction(print_gpr_reg); \
                    ABI_PopRegistersAndAdjustStack(registers_in_use2, 0); \
                } else { \
                    BitSet32 registers_in_use2 = caller_saved_registers_in_use(); \
                    ABI_PushRegistersAndAdjustStack(registers_in_use2, 0); \
                    MOV(64, R(ABI_PARAM1), HOTSTATE_REG_TMP(k)); \
                    MOV(64, R(ABI_PARAM2), Imm32(k)); \
                    ABI_CallFunction(print_gpr_reg); \
                    ABI_PopRegistersAndAdjustStack(registers_in_use2, 0); \
                } \
            } \
            registers_in_use = caller_saved_registers_in_use(); \
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
            ABI_CallFunction(print_newline); \
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
        } \
        registers_in_use = caller_saved_registers_in_use(); \
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
        ABI_CallFunction(print_cp0_regs); \
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
        for (int i = 0; i < 4; i++) { \
            for (int j = 0; j < 8; j++) { \
                int k = i * 8 + j; \
                /*if (m_fpr.IsBound(k) && m_fpr.IsDirty(k)) { */ \
                if (m_fpr.IsBound(k) && m_fpr.IsDirty(k)) { \
                    RCX64Reg reg = m_fpr.Bind(k, RCMode::Read, m_fpr.Is32BitOnly(k)); \
                    RegCache::Realize(reg); \
                    if (m_fpr.Is32BitOnly(k)) { \
                        if (HOT_STATE->fr_is_set) { \
                            MOVSS(HOTSTATE_CP1REG32FR_TMP(k), reg); \
                        } else { \
                            MOVSS(HOTSTATE_CP1REG32NOFR_TMP(k), reg); \
                        } \
                    } else { \
                        MOVSD(HOTSTATE_ARRAY(fprs_tmp, k), reg); \
                    } \
                } \
                /* } */ \
            } \
        } \
        registers_in_use = caller_saved_registers_in_use(); \
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
        ABI_CallFunction(print_cp1_regs); \
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
    } while(0)
#endif

#define PRINT_IM_HERE_BLOCK_START(addr) \
    do { \
        BitSet32 registers_in_use = caller_saved_registers_in_use(); \
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
        MOV(32, R(ABI_PARAM1), Imm32(addr)); \
        ABI_CallFunction(ImHereBlockStart); \
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
    } while(0)

static void print_flush(int j)
{
    fprintf(stderr, "FLUSH: %d\n", j);
}

#define PRINT_FLUSH(regs) \
    do { \
        BitSet32 registers_in_use = caller_saved_registers_in_use(); \
        for (int j : regs) { \
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0); \
            ABI_CallFunctionC(print_flush, j); \
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0); \
        } \
    } while(0)


uint32_t vr4300_jitter_translate_address(uint32_t address, int w)
{
    return virtual_to_physical_address(VR4300_Jitter::GetInstance()->GetR4300Core(), address, w);
}

uint32_t vr4300_jitter_translate_address_no_exception(uint32_t address, int w)
{
    u32 address_in = address;
    if (vr4300_jitter_address_needs_translation(address)) {
        address = virtual_to_physical_address_no_exception(VR4300_Jitter::GetInstance()->GetR4300Core(), address, w);
        if (address == 0) {
#ifndef NDEBUG
            DebugMessage(M64MSG_VERBOSE, "FAILED ADDRESS TRANSLATION (noexcept) 0x%08x 0x%08x %d\n", address, address_in, w);
#endif
            return 0;
        }
    }

    address &= UINT32_C(0x1ffffffc);
    return address;
}

uint32_t vr4300_jitter_translate_pc_no_exception(uint32_t pc)
{
    u32 address = pc;
    if (vr4300_jitter_address_needs_translation(address)) {
        address = virtual_to_physical_pc(VR4300_Jitter::GetInstance()->GetR4300Core(), pc, 2);
        if (address == -1) {
#ifndef NDEBUG
            DebugMessage(M64MSG_VERBOSE, "FAILED PC TRANSLATION (noexcept) 0x%08x 0x%08x %d\n", address, pc, 2);
#endif
            return -1;
        }
    }

    return address;
}

void VR4300_Jitter::switch_to_far_code()
{
    assert(!m_in_far_code);
    m_in_far_code = true;
    m_near_code = GetWritableCodePtr();
    m_near_code_end = GetWritableCodeEnd();
    m_near_code_write_failed = HasWriteFailed();
    SetCodePtr(m_far_code.GetWritableCodePtr(), m_far_code.GetWritableCodeEnd(),
        m_far_code.HasWriteFailed());
    AlignCode4();
}

void VR4300_Jitter::switch_to_near_code()
{
    assert(m_in_far_code);
    m_in_far_code = false;
    m_far_code.SetCodePtr(GetWritableCodePtr(), GetWritableCodeEnd(), HasWriteFailed());
    SetCodePtr(m_near_code, m_near_code_end, m_near_code_write_failed);
}

bool VR4300_Jitter::is_in_far_code()
{
    return m_in_far_code;
}

void VR4300_Jitter::enter_farcode()
{
    FixupBranch far_code = J(XEmitter::Jump::Near);
    switch_to_far_code();
    SetJumpTarget(far_code);
}

void VR4300_Jitter::leave_farcode()
{
    FixupBranch near_code = J(XEmitter::Jump::Near);
    switch_to_near_code();
    SetJumpTarget(near_code);
}

u32 VR4300_Jitter::get_address_of_nth_instruction_after(struct jit_instr *op, int n)
{
    if (n < HOT_STATE->instructionsLeft) {
        return HOT_STATE->op[n].address;
    } else {
        return get_address_of_nth_instruction_after(op, n - 1) + 4;
    }
}

void VR4300_Jitter::mov_2(int bits1, const Gen::X64Reg &dest1, int bits2, const Gen::X64Reg &dest2, const RCOpArg &source1, const RCOpArg &source2) {
    assert((bits1 == 32 || bits1 == 64) && (bits2 == 32 || bits2 == 64));
    if (source1.IsSimpleReg() && dest1 == source1.GetSimpleReg() &&
        source2.IsSimpleReg() && dest2 == source2.GetSimpleReg()) {
        // nop
        if (bits1 != 64) {
            AND(bits1, R(dest1), Imm32(0xffffffff));
        }
        if (bits2 != 64) {
            AND(bits2, R(dest2), Imm32(0xffffffff));
        }
    } else {
        if (source1.IsSimpleReg() && source2.IsSimpleReg()) {
            MOVTwo((bits1 == bits2) ? bits1 : 64, dest1, source1.GetSimpleReg(), 0, dest2, source2.GetSimpleReg());
            if (bits1 != bits2) {
                if (bits1 != 64) {
                    AND(bits1, R(dest1), Imm32(0xffffffff));
                }
                if (bits2 != 64) {
                    AND(bits2, R(dest2), Imm32(0xffffffff));
                }
            }
        } else {
            bool done2 = false;
            if (source2.IsSimpleReg(dest1)) {
                MOV(bits2, R(dest2), source2);
                done2 = true;
            }

            if (!source1.IsSimpleReg(dest1)) {
                if (source1.IsImm() && bits1 != 64) {
                    MOV(bits1, R(dest1), Imm32(source1.Imm64()));
                } else {
                    MOV(bits1, R(dest1), source1);
                }
            }

            if (!done2) {
                if (!source2.IsSimpleReg(dest2)) {
                    if (source2.IsImm() && bits2 != 64) {
                        MOV(bits2, R(dest2), Imm32(source2.Imm64()));
                    } else {
                        MOV(bits2, R(dest2), source2);
                    }
                }
            }
        }
    }
}

void VR4300_Jitter::mov_3(int bits, const Gen::X64Reg &dest1, const Gen::X64Reg &dest2, const Gen::X64Reg &dest3, const RCOpArg &source1, const RCOpArg &source2, const RCOpArg &source3) {
    if (source1.IsSimpleReg() && dest1 == source1.GetSimpleReg() &&
        source2.IsSimpleReg() && dest2 == source2.GetSimpleReg() &&
        source3.IsSimpleReg() && dest3 == source3.GetSimpleReg()) {
        // nop
    } else {
        if (source1.IsSimpleReg() && source2.IsSimpleReg() && source3.IsSimpleReg()) {
            Gen::X64Reg source_reg1 = source1.GetSimpleReg(), source_reg2 = source2.GetSimpleReg(), source_reg3 = source3.GetSimpleReg();

            if (dest2 == source_reg1 || dest3 == source_reg1) {
                PUSH(bits, R(source_reg1));
            }

            if (dest2 != source_reg2 && dest3 != source_reg3) {
                MOVTwo(bits, dest2, source_reg2, 0, dest3, source_reg3);
            } else if (dest3 != source_reg3) {
                MOV(bits, R(dest3), R(source_reg3));
            } else if (dest2 != source_reg2) {
                MOV(bits, R(dest2), R(source_reg2));
            }

            if (dest2 == source_reg1 || dest3 == source_reg1) {
                POP(bits, R(source_reg1));
            }
            if (dest1 != source_reg1) {
                MOV(bits, R(dest1), R(source_reg1));
            }
        } else if (source1.IsSimpleReg() && source2.IsSimpleReg()) {
            MOVTwo(bits, dest1, source1.GetSimpleReg(), 0, dest2, source2.GetSimpleReg());
            if (bits == 32 && source3.IsImm()) {
                MOV(bits, R(dest3), Imm32(source3.Imm64()));
            } else {
                MOV(bits, R(dest3), source3);
            }
        }  else if (source2.IsSimpleReg() && source3.IsSimpleReg()) {
            MOVTwo(bits, dest2, source2.GetSimpleReg(), 0, dest3, source3.GetSimpleReg());
            if (bits == 32 && source1.IsImm()) {
                MOV(bits, R(dest1), Imm32(source1.Imm64()));
            } else {
                MOV(bits, R(dest1), source1);
            }
        } else if (source1.IsSimpleReg() && source3.IsSimpleReg()) {
            MOVTwo(bits, dest1, source1.GetSimpleReg(), 0, dest3, source3.GetSimpleReg());
            if (bits == 32 && source2.IsImm()) {
                MOV(bits, R(dest2), Imm32(source2.Imm64()));
            } else {
                MOV(bits, R(dest2), source2);
            }
        } else {
            bool done2 = false, done3 = false;
            // at most 1 parameter is a simple reg.
            if (!source1.IsSimpleReg(dest1)) {
                if (source2.IsSimpleReg(dest1)) {
                    MOV(bits, R(dest2), R(dest1));
                    done2 = true;
                } else if (source3.IsSimpleReg(dest1)) {
                    MOV(bits, R(dest3), R(dest1));
                    done3 = true;
                }
                if (source1.IsImm() && bits != 64) {
                    MOV(bits, R(dest1), Imm32(source1.Imm64()));
                } else {
                    MOV(bits, R(dest1), source1);
                }
            }
            if (!done2 && !source2.IsSimpleReg(dest2)) {
                if (source3.IsSimpleReg(dest2)) {
                    MOV(bits, R(dest3), R(dest2));
                    done3 = true;
                }
                if (source2.IsImm() && bits != 64) {
                    MOV(bits, R(dest2), Imm32(source2.Imm64()));
                } else {
                    MOV(bits, R(dest2), source2);
                }
            }
            if (!done3 && !source3.IsSimpleReg(dest3)) {
                if (source3.IsImm() && bits != 64) {
                    MOV(bits, R(dest3), Imm32(source3.Imm64()));
                } else {
                    MOV(bits, R(dest3), source3);
                }
            }
        }
    }
}

BitSet32 VR4300_Jitter::caller_saved_registers_in_use()
{
  BitSet32 in_use = m_gpr.RegistersInUse() | (m_fpr.RegistersInUse() << 16);
  in_use[RHOTSTATE2] = 1;
  in_use[RHOTSTATE3] = 1;
  return in_use & ABI_ALL_CALLER_SAVED;
}

const void *vr4300_jitter_dispatch(unsigned int addr)
{
    return VR4300_Jitter::GetInstance()->GetBlockCache()->Dispatch(addr);
}

void vr4300_jitter_core_compare(void);

void VR4300_Jitter::emit_core_compare()
{
#if defined(COMPARE_CORE) || PRINT_IM_HERE_DEBUG
    BitSet32 registers_in_use = BitSet32::AllTrue(32);
    // registers_in_use[RHOTSTATE2] = true;
    // registers_in_use[RHOTSTATE3] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunction(vr4300_jitter_core_compare);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    RET();
#endif
}

static void vr4300_jitter_mispredicted_return(uint32_t m, int i);
static void vr4300_jitter_successfully_predicted_return(int m);

void VR4300_Jitter::generate_asm()
{
    BitSet32 registers_in_use = BitSet32{};
    registers_in_use[RHOTSTATE2] = true;
    registers_in_use[RHOTSTATE3] = true;

    m_enter_code = AlignCode16();
    ABI_PushRegistersAndAdjustStack(ABI_ALL_CALLEE_SAVED, 8, /*frame*/ 16);

    MOV(64, R(RHOTSTATE), Imm64((u64)HOT_STATE + 0x80));
    MOV(64, R(RHOTSTATE2), Imm64((u64)HOT_STATE + 0x180));
    MOV(64, R(RHOTSTATE3), Imm64((u64)HOT_STATE + 0x280));

#if DISABLE_FASTMEM
#if !DISABLE_RDRAM_OPTIMIZATION
    MOV(64, R(RDRAM), ImmPtr(m_rdram_ptr));
#endif
#else
    MOV(64, R(RDRAM), ImmPtr(m_physical_base));
#endif
#if USE_REG_FOR_STORED_PC
    MOV(64, R(RSTOREDPC), Imm32(0));
#endif

    MOV(64, HOTSTATE_VAR(stored_stack_pointer), R(RSP));

    MOV(64, MDisp(RSP, 8), Imm32((u32)-1));

    m_dispatcher_checkstop = GetCodePtr();

    MOV(32, R(RSCRATCH), HOTSTATE_VAR(stop));
    TEST(32, R(RSCRATCH), Imm32(0xFFFFFFFF));
    FixupBranch stopjit = J_CC(CC_NZ, XEmitter::Jump::Near);

    m_dispatcher_start = GetCodePtr();
    MOV(32, R(RSCRATCH_PC), HOTSTATE_VAR(pc));
    MOV(64, R(RSCRATCH_EXTRA), ImmPtr(&m_r4300->cp0.tlb.LUT_r[0]));
    MOV(64, R(RSCRATCH_EXTRA2), ImmPtr(m_block_cache.GetEntryPoints()));
    MOV(32, R(RSCRATCH_EXTRA3), HOTSTATE_VAR(fr_is_set));
#if !HUGE_MAP_FOR_ENTRY_POINTS
    SHL(64, R(RSCRATCH_EXTRA3), Imm8(32));
#else
    SHL(32, R(RSCRATCH_EXTRA3), Imm8(12 + 11));
#endif

#if !HUGE_MAP_FOR_ENTRY_POINTS
    MOV(32, R(RSCRATCH), R(RSCRATCH_PC));
    MOV(32, R(RSCRATCH2), R(RSCRATCH_PC));
    AND(32, R(RSCRATCH), Imm32(0xC0000000));
    CMP(32, R(RSCRATCH), Imm32(0x80000000));

    FixupBranch does_not_need_translation = J_CC(CC_E);

    SHR(32, R(RSCRATCH2), Imm8(12));
    MOV(32, R(RSCRATCH2), MComplex(RSCRATCH_EXTRA, RSCRATCH2, SCALE_4, 0));

    TEST(32, R(RSCRATCH2), R(RSCRATCH2));
    FixupBranch needs_slow_dispatcher = J_CC(CC_Z, XEmitter::Jump::Near);

    // AddressToLookupIndex
    MOV(32, R(RSCRATCH2), R(RSCRATCH_PC));
    OR(64, R(RSCRATCH2), R(RSCRATCH_EXTRA3));
    SetJumpTarget(does_not_need_translation);
    MOV(64, R(RSCRATCH), MComplex(RSCRATCH_EXTRA2, RSCRATCH2, SCALE_2, 0));

    TEST(64, R(RSCRATCH), R(RSCRATCH));
    FixupBranch fast_dispatcher_did_not_find_block = J_CC(CC_Z);
    JMPptr(R(RSCRATCH));
    SetJumpTarget(fast_dispatcher_did_not_find_block);


#else
    MOV(32, R(RSCRATCH), R(RSCRATCH_PC));

#if !FAST_DISPATCHER_ALWAYS_TRANSLATE
    AND(32, R(RSCRATCH), Imm32(0xC0000000));
    CMP(32, R(RSCRATCH), Imm32(0x80000000));

    FixupBranch does_need_translation = J_CC(CC_NE, XEmitter::Jump::Near);
    MOV(32, R(RSCRATCH), R(RSCRATCH_PC));
    MOV(32, R(RSCRATCH2), R(RSCRATCH_PC));
    // AddressToLookupIndex:
    AND(32, R(RSCRATCH), Imm32(0x7FF000));
    OR(32, R(RSCRATCH), R(RSCRATCH_EXTRA3));
    SHL(64, R(RSCRATCH), Imm8(32 - 12));
    OR(64, R(RSCRATCH2), R(RSCRATCH));

    MOV(64, R(RSCRATCH), MComplex(RSCRATCH_EXTRA2, RSCRATCH2, SCALE_2, 0));

    TEST(64, R(RSCRATCH), R(RSCRATCH));
    FixupBranch fast_dispatcher_did_not_find_block = J_CC(CC_Z, XEmitter::Jump::Near);
    JMPptr(R(RSCRATCH));
    SetJumpTarget(does_need_translation);
    MOV(32, R(RSCRATCH), R(RSCRATCH_PC));
#endif
    // translate PC
    MOV(32, R(RSCRATCH2), R(RSCRATCH_PC));
    SHR(32, R(RSCRATCH2), Imm8(12));
    MOV(32, R(RSCRATCH2), MComplex(RSCRATCH_EXTRA, RSCRATCH2, SCALE_4, 0));
#if FAST_DISPATCHER_ALWAYS_TRANSLATE
    TEST(32, R(RSCRATCH2), R(RSCRATCH2));
    CMOVcc(32, RSCRATCH2, R(RSCRATCH_PC), CC_Z);
#endif
#if 0 // lower bits not needed, see below.
    AND(32, R(RSCRATCH2), Imm32(0xFFFFF000));
    MOV(32, R(RSCRATCH), R(RSCRATCH_PC));
    AND(32, R(RSCRATCH), Imm32(0xFFF));
    OR(32, R(RSCRATCH), R(RSCRATCH2));
#else
    MOV(32, R(RSCRATCH), R(RSCRATCH2));
#endif

    MOV(32, R(RSCRATCH2), R(RSCRATCH_PC));
    // AddressToLookupIndex:
    AND(32, R(RSCRATCH), Imm32(0x7FF000));
    OR(32, R(RSCRATCH), R(RSCRATCH_EXTRA3));
    SHL(64, R(RSCRATCH), Imm8(32 - 12));
    OR(64, R(RSCRATCH2), R(RSCRATCH));

    MOV(64, R(RSCRATCH), MComplex(RSCRATCH_EXTRA2, RSCRATCH2, SCALE_2, 0));

    TEST(64, R(RSCRATCH), R(RSCRATCH));
    FixupBranch fast_dispatcher_did_not_find_block_after_translation = J_CC(CC_Z);
    JMPptr(R(RSCRATCH));
    SetJumpTarget(fast_dispatcher_did_not_find_block_after_translation);
#if !FAST_DISPATCHER_ALWAYS_TRANSLATE
    SetJumpTarget(fast_dispatcher_did_not_find_block);
#endif
#endif

#if !HUGE_MAP_FOR_ENTRY_POINTS
    SetJumpTarget(needs_slow_dispatcher);
#endif
    m_jitcache_dispatcher_start = GetCodePtr();

    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(32, R(ABI_PARAM1), HOTSTATE_VAR(pc));
    ABI_CallFunction(vr4300_jitter_dispatch);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

    TEST(64, R(ABI_RETURN), R(ABI_RETURN));
    FixupBranch no_block_available = J_CC(CC_Z);
    JMPptr(R(ABI_RETURN));
    SetJumpTarget(no_block_available);

    m_recompiler_start = GetCodePtr();

    MOV(64, R(RSP), HOTSTATE_VAR(stored_stack_pointer));

    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(32, R(ABI_PARAM1), HOTSTATE_VAR(pc));
    ABI_CallFunction(vr4300_jitter_recompile_block);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

    TEST(64, R(ABI_RETURN), R(ABI_RETURN));
    FixupBranch recompiler_failed = J_CC(CC_Z);
    JMPptr(R(ABI_RETURN));
    SetJumpTarget(recompiler_failed);

    SetJumpTarget(stopjit);
    MOV(64, R(RSP), HOTSTATE_VAR(stored_stack_pointer));
    m_dispatcher_exit = GetCodePtr();

    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunction(vr4300_jitter_cleanup);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

    ABI_PopRegistersAndAdjustStack(ABI_ALL_CALLEE_SAVED, 8, 16);
    RET();

    m_dispatcher_mispredicted_ret_1 = GetCodePtr();
#if DEBUG_PREDICTIONS
    MOV(64, R(RSCRATCH), MDisp(RSP, 8));
#endif
    MOV(64, R(RSP), HOTSTATE_VAR(stored_stack_pointer));

#if DEBUG_PREDICTIONS
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(32, R(ABI_PARAM2), Imm32(1));
    ABI_CallFunctionR(vr4300_jitter_mispredicted_return, RSCRATCH);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif

    JMP(m_dispatcher_start, XEmitter::Jump::Near);

    m_dispatcher_mispredicted_ret_2 = GetCodePtr();
#if DEBUG_PREDICTIONS
    MOV(64, R(RSCRATCH), MDisp(RSP, 8));
#endif
    MOV(64, R(RSP), HOTSTATE_VAR(stored_stack_pointer));

#if DEBUG_PREDICTIONS
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(32, R(ABI_PARAM2), Imm32(2));
    ABI_CallFunctionR(vr4300_jitter_mispredicted_return, RSCRATCH);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif

    JMP(m_dispatcher_start, XEmitter::Jump::Near);

    m_dispatcher_mispredicted_ret_3 = GetCodePtr();
#if DEBUG_PREDICTIONS
    MOV(64, R(RSCRATCH), MDisp(RSP, 8));
#endif
    MOV(64, R(RSP), HOTSTATE_VAR(stored_stack_pointer));

#if DEBUG_PREDICTIONS
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(32, R(ABI_PARAM2), Imm32(3));
    ABI_CallFunctionR(vr4300_jitter_mispredicted_return, RSCRATCH);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif

    JMP(m_dispatcher_start, XEmitter::Jump::Near);
    m_core_compare = GetCodePtr();
    emit_core_compare();
}

void vr4300_jitter_init(void)
{
    if (!gJitterInstance) gJitterInstance = new VR4300_Jitter();
    VR4300_Jitter::GetInstance()->Init();
}

void VR4300_Jitter::Init(void)
{
  DebugMessage(M64MSG_INFO, "Init vr4300 jitter");
  Common::JitRegister::Init("/tmp");

#if DISABLE_GPR_REG_CACHE
  m_r4300->recompiler_hot_state.gprs_tmp_ptr = m_r4300->recompiler_hot_state.regs;
#else
  m_r4300->recompiler_hot_state.gprs_tmp_ptr = m_r4300->recompiler_hot_state.gprs_tmp;
#endif

#if DISABLE_FLOAT_REG_CACHE
  m_r4300->recompiler_hot_state.fprs_tmp_ptr = (int64_t*)m_r4300->recompiler_hot_state.cp1_regs;
#else
  m_r4300->recompiler_hot_state.fprs_tmp_ptr = HOT_STATE->fprs_tmp;
#endif

  memset(&m_tlb_mappings, 0, 0x100000 * sizeof(m_tlb_mappings[0]));
  memset(&m_valid_virtual_block, 1, 0x100000 * sizeof(m_valid_virtual_block[0]));

  m_r4300->recompiler_hot_state.rdram_generate_slowcode = 0;
  m_r4300->recompiler_hot_state.rdram_corruption_changed = 0;

#if defined(COMPARE_CORE)
  m_r4300->recompiler_hot_state.last_idx = 0;
#endif

  AllocCodeSpace(HOST_CODE_SIZE + m_const_pool.CONST_POOL_SIZE);
  AddChildCodeSpace(&m_far_code, HOST_CODE_SIZE / 2);

  const size_t constpool_size = m_const_pool.CONST_POOL_SIZE;
  m_const_pool.Init(AllocChildCodeSpace(constpool_size), constpool_size);

  m_block_cache.Init();

  m_gpr.SetEmitter(this);
  m_gpr.SetReg0Unusable();
  m_fpr.SetEmitter(this);

  m_valid_block_ptr = reinterpret_cast<u8*>(m_valid_block_arena.Create(VALID_BLOCK_ARENA_SIZE));
  m_valid_block_arena.Clear();

  ResetCodePtr();

  generate_asm();
  Common::JitRegister::Register(m_enter_code, GetCodePtr(), "JIT_START");

  AlignCode16();
  m_free_code_start = GetWritableCodePtr();

  m_free_ranges_near.clear();
  m_free_ranges_near.insert(m_free_code_start, region + region_size);
  m_free_ranges_far.clear();
  m_free_ranges_far.insert(m_far_code.GetWritableCodePtr(), m_far_code.GetWritableCodeEnd());

  m_r4300->recompiler_hot_state.pc = 0xa4000040;
  m_r4300->recompiler_hot_state.dbg_pc = 0xa4000040;

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "CPU INFO: %s\n", cpu_info.Summarize().c_str());
    DebugMessage(M64MSG_VERBOSE, "pc = %x %x %d %d\n", HOTSTATE_OFF(pc), HOTSTATE2_OFF(pc), HOTSTATE_OFF(pc), HOTSTATE2_OFF(pc));
    DebugMessage(M64MSG_VERBOSE, "stored_pc = %x %x %d %d\n", HOTSTATE_OFF(stored_pc), HOTSTATE2_OFF(stored_pc), HOTSTATE_OFF(stored_pc), HOTSTATE2_OFF(stored_pc));
    DebugMessage(M64MSG_VERBOSE, "regs = %x %x %d %d\n", HOTSTATE_OFF(regs), HOTSTATE2_OFF(regs), HOTSTATE_OFF(regs), HOTSTATE2_OFF(regs));
    DebugMessage(M64MSG_VERBOSE, "cp1_regs_simple = %x %x %d %d\n", HOTSTATE_OFF(cp1_regs_simple), HOTSTATE2_OFF(cp1_regs_simple), HOTSTATE_OFF(cp1_regs_simple), HOTSTATE2_OFF(cp1_regs_simple));
    DebugMessage(M64MSG_VERBOSE, "cp1_regs = %x %x %d %d\n", HOTSTATE_OFF(cp1_regs), HOTSTATE2_OFF(cp1_regs), HOTSTATE_OFF(cp1_regs), HOTSTATE2_OFF(cp1_regs));
    DebugMessage(M64MSG_VERBOSE, "cycles_added = %x %x %d %d\n", HOTSTATE_OFF(cycles_added), HOTSTATE2_OFF(cycles_added), HOTSTATE_OFF(cycles_added), HOTSTATE2_OFF(cycles_added));
    DebugMessage(M64MSG_VERBOSE, "hot_cycles = %x %x %d %d\n", HOTSTATE_OFF(hot_cycles), HOTSTATE2_OFF(hot_cycles), HOTSTATE_OFF(hot_cycles), HOTSTATE2_OFF(hot_cycles));
    DebugMessage(M64MSG_VERBOSE, "cycle_count = %x %x %d %d\n", HOTSTATE_OFF(cycle_count), HOTSTATE2_OFF(cycle_count), HOTSTATE_OFF(cycle_count), HOTSTATE2_OFF(cycle_count));
    DebugMessage(M64MSG_VERBOSE, "next_interrupt = %x %x %d %d\n", HOTSTATE_OFF(next_interrupt), HOTSTATE2_OFF(next_interrupt), HOTSTATE_OFF(next_interrupt), HOTSTATE2_OFF(next_interrupt));
    DebugMessage(M64MSG_VERBOSE, "cp1_fcr0 = %x %x %d %d\n", HOTSTATE_OFF(cp1_fcr0), HOTSTATE2_OFF(cp1_fcr0), HOTSTATE_OFF(cp1_fcr0), HOTSTATE2_OFF(cp1_fcr0));
    DebugMessage(M64MSG_VERBOSE, "cp1_fcr31 = %x %x %d %d\n", HOTSTATE_OFF(cp1_fcr31), HOTSTATE2_OFF(cp1_fcr31), HOTSTATE_OFF(cp1_fcr31), HOTSTATE2_OFF(cp1_fcr31));
    DebugMessage(M64MSG_VERBOSE, "hi = %x %x %d %d\n", HOTSTATE_OFF(hi), HOTSTATE2_OFF(hi), HOTSTATE_OFF(hi), HOTSTATE2_OFF(hi));
    DebugMessage(M64MSG_VERBOSE, "lo = %x %x %d %d\n", HOTSTATE_OFF(lo), HOTSTATE2_OFF(lo), HOTSTATE_OFF(lo), HOTSTATE2_OFF(lo));
    DebugMessage(M64MSG_VERBOSE, "cp0_regs = %x %x %d %d\n", HOTSTATE_OFF(cp0_regs), HOTSTATE2_OFF(cp0_regs), HOTSTATE_OFF(cp0_regs), HOTSTATE2_OFF(cp0_regs));
    DebugMessage(M64MSG_VERBOSE, "cycle_count = %x %x %d %d\n", HOTSTATE_OFF(cycle_count), HOTSTATE2_OFF(cycle_count), HOTSTATE_OFF(cycle_count), HOTSTATE2_OFF(cycle_count));
    DebugMessage(M64MSG_VERBOSE, "compiler_cycles = %x %x %d %d\n", HOTSTATE_OFF(compiler_cycles), HOTSTATE2_OFF(compiler_cycles), HOTSTATE_OFF(compiler_cycles), HOTSTATE2_OFF(compiler_cycles));
    DebugMessage(M64MSG_VERBOSE, "stop = %x %x %d %d\n", HOTSTATE_OFF(stop), HOTSTATE2_OFF(stop), HOTSTATE_OFF(stop), HOTSTATE2_OFF(stop));
    DebugMessage(M64MSG_VERBOSE, "last_block_broken = %x %x %d %d\n", HOTSTATE_OFF(last_block_broken), HOTSTATE2_OFF(last_block_broken), HOTSTATE_OFF(last_block_broken), HOTSTATE2_OFF(last_block_broken));
    DebugMessage(M64MSG_VERBOSE, "pc_changed = %x %x %d %d\n", HOTSTATE_OFF(pc_changed), HOTSTATE2_OFF(pc_changed), HOTSTATE_OFF(pc_changed), HOTSTATE2_OFF(pc_changed));
    DebugMessage(M64MSG_VERBOSE, "curBlock = %x %x %d %d\n", HOTSTATE_OFF(curBlock), HOTSTATE2_OFF(curBlock), HOTSTATE_OFF(curBlock), HOTSTATE2_OFF(curBlock));
    DebugMessage(M64MSG_VERBOSE, "llbit = %x %x %d %d\n", HOTSTATE_OFF(llbit), HOTSTATE2_OFF(llbit), HOTSTATE_OFF(llbit), HOTSTATE2_OFF(llbit));
    DebugMessage(M64MSG_VERBOSE, "cp0_latch = %x %x %d %d\n", HOTSTATE_OFF(cp0_latch), HOTSTATE2_OFF(cp0_latch), HOTSTATE_OFF(cp0_latch), HOTSTATE2_OFF(cp0_latch));
    DebugMessage(M64MSG_VERBOSE, "stored_stack_pointer = %x %x %d %d\n", HOTSTATE_OFF(stored_stack_pointer), HOTSTATE2_OFF(stored_stack_pointer), HOTSTATE_OFF(stored_stack_pointer), HOTSTATE2_OFF(stored_stack_pointer));
    DebugMessage(M64MSG_VERBOSE, "cp1_regs_double = %x %x %d %d\n", HOTSTATE_OFF(cp1_regs_double), HOTSTATE2_OFF(cp1_regs_double), HOTSTATE_OFF(cp1_regs_double), HOTSTATE2_OFF(cp1_regs_double));
    DebugMessage(M64MSG_VERBOSE, "rounding_modes = %x %x %d %d\n", HOTSTATE_OFF(rounding_modes), HOTSTATE2_OFF(rounding_modes), HOTSTATE_OFF(rounding_modes), HOTSTATE2_OFF(rounding_modes));
    DebugMessage(M64MSG_VERBOSE, "gprs_tmp_ptr = %x %x %d %d\n", HOTSTATE_OFF(gprs_tmp_ptr), HOTSTATE2_OFF(gprs_tmp_ptr), HOTSTATE_OFF(gprs_tmp_ptr), HOTSTATE2_OFF(gprs_tmp_ptr));
    DebugMessage(M64MSG_VERBOSE, "fprs_tmp_ptr = %x %x %d %d\n", HOTSTATE_OFF(fprs_tmp_ptr), HOTSTATE2_OFF(fprs_tmp_ptr), HOTSTATE_OFF(fprs_tmp_ptr), HOTSTATE2_OFF(fprs_tmp_ptr));
    DebugMessage(M64MSG_VERBOSE, "instructionsLeft = %x %x %d %d\n", HOTSTATE_OFF(instructionsLeft), HOTSTATE2_OFF(instructionsLeft), HOTSTATE_OFF(instructionsLeft), HOTSTATE2_OFF(instructionsLeft));
    DebugMessage(M64MSG_VERBOSE, "isLastInstruction = %x %x %d %d\n", HOTSTATE_OFF(isLastInstruction), HOTSTATE2_OFF(isLastInstruction), HOTSTATE_OFF(isLastInstruction), HOTSTATE2_OFF(isLastInstruction));
    DebugMessage(M64MSG_VERBOSE, "inDelaySlot = %x %x %d %d\n", HOTSTATE_OFF(inDelaySlot), HOTSTATE2_OFF(inDelaySlot), HOTSTATE_OFF(inDelaySlot), HOTSTATE2_OFF(inDelaySlot));
    DebugMessage(M64MSG_VERBOSE, "op = %x %x %d %d\n", HOTSTATE_OFF(op), HOTSTATE2_OFF(op), HOTSTATE_OFF(op), HOTSTATE2_OFF(op));
    DebugMessage(M64MSG_VERBOSE, "modifies_count_reg = %x %x %d %d\n", HOTSTATE_OFF(modifies_count_reg), HOTSTATE2_OFF(modifies_count_reg), HOTSTATE_OFF(modifies_count_reg), HOTSTATE2_OFF(modifies_count_reg));
    DebugMessage(M64MSG_VERBOSE, "modifies_status_reg = %x %x %d %d\n", HOTSTATE_OFF(modifies_status_reg), HOTSTATE2_OFF(modifies_status_reg), HOTSTATE_OFF(modifies_status_reg), HOTSTATE2_OFF(modifies_status_reg));
    for (int i = 0; i < 32; i++) {
        DebugMessage(M64MSG_VERBOSE, "cp1_regfr(%d) = %d %x\n", i, HOTSTATE_OFF_CP1REG32FR(i), HOTSTATE_OFF_CP1REG32FR(i));
        DebugMessage(M64MSG_VERBOSE, "cp1_regnofr(%d) = %d %x\n", i, HOTSTATE_OFF_CP1REG32NOFR(i), HOTSTATE_OFF_CP1REG32NOFR(i));
    }
#endif

    m_initialized = true;
}

void vr4300_jitter_start(void)
{
    void (*enter)(void) = (void(*)())VR4300_Jitter::GetInstance()->GetEnterCode();
    enter();
}

#define PRINT_ARG(instr, h) if (instr.has_ ##h) fprintf(stderr, "" #h "(%d) ", instr.h)

void vr4300_jitter_core_compare(void)
{
#if PRINT_IM_HERE_DEBUG
    skip_log();
    if (!is_skip_log()) {
        DebugMessage(M64MSG_VERBOSE, "CALLING CORE COMPARE CALLBACK 0x%x\n", HOT_STATE->pc);
    }
#endif
#if defined(COMPARE_CORE)
    CoreCompareCallback();
    if (HOT_STATE->stop) {
        fprintf(stderr, "LAST INSTRUCTIONS (FR: %d):\n", HOT_STATE->fr_is_set);
        for (int i = HOT_STATE->last_idx, j = 0; i < NUM_DBG_INSTRUCTIONS + HOT_STATE->last_idx; i++) {
            int k = i % NUM_DBG_INSTRUCTIONS;
            fprintf(stderr, "%d, %x, %s %x\n", j++, HOT_STATE->last_addresses[k], HOT_STATE->last_names[k], HOT_STATE->last_instructions[k]);
            struct jit_instr instr;
            VR4300_Jitter::GetInstance()->AnalyzeInstruction(&instr, HOT_STATE->last_instructions[k], HOT_STATE->last_addresses[k]);
            PRINT_ARG(instr, a);
            PRINT_ARG(instr, b);
            PRINT_ARG(instr, c);
            PRINT_ARG(instr, d);
            PRINT_ARG(instr, f);
            PRINT_ARG(instr, k);
            PRINT_ARG(instr, s);
            PRINT_ARG(instr, t);
            PRINT_ARG(instr, x);
            fprintf(stderr, "\n");
        }
    }
#endif
}

bool VR4300_Jitter::AnalyzeInstruction(jit_instr *instr, uint32_t instruction, unsigned int instr_address)
{
    vr4300_instruction inst = vr4300_instruction(instruction);

    instr->address = instr_address;
    instr->instruction = instruction;

    return decode_instruction(inst, instr);
};

void VR4300_Jitter::set_instruction_stats(struct jit_instr *instr)
{
    bool is_float = false;
    int found = 0;

    if (instr->has_x) {
        is_float = (instr->x == 1);
    }

    if (instr->has_a) {
        is_float = true;
    }

    if (!instr->is_branch_or_jump) {
        if (instr->has_t && instr->has_s && !instr->has_d && !instr->has_k) {
            if (is_float) {
                // C_cond_fmt
                assert(instr->has_a);
                if (instr->a == 0x10 || instr->a == 0x14) {
                   instr->fregsIn32[instr->s] = 1;
                   instr->fregsIn32[instr->t] = 1;
                } else {
                    instr->fregsIn[instr->s] = 1;
                    instr->fregsIn[instr->t] = 1;
                }
            } else {
                // MULT(U)...
                instr->regsIn[instr->s] = 1;
                instr->regsIn[instr->t] = 1;
            }
            found++;
        }

        if (instr->has_t && instr->has_s && !instr->has_d && instr->has_k) {
            // ADDI...
            instr->regsIn[instr->s] = 1;
            instr->regsOut[instr->t] = 1;
            found++;
        }

        if (instr->has_t && instr->has_s && instr->has_d) {
            if (is_float) {
                assert(instr->has_a);
                if (instr->a == 0x10 || instr->a == 0x14)  {
                    instr->fregsOut32[instr->d] = 1;
                    instr->fregsIn32[instr->s] = 1;
                    instr->fregsIn32[instr->t] = 1;
                } else if (instr->a == 0x11 || instr->a == 0x15)  {
                    instr->fregsOut[instr->d] = 1;
                    instr->fregsIn[instr->s] = 1;
                    instr->fregsIn[instr->t] = 1;
                } else abort();
            } else {
                instr->regsOut[instr->d] = 1;
                instr->regsIn[instr->s] = 1;
                instr->regsIn[instr->t] = 1;
            }
            found++;
        }

        if (instr->has_t && instr->has_d && !instr->has_s && !instr->has_k) {
            if (is_float) {
                if (strncmp(instr->name, "MT", 2) == 0 || strncmp(instr->name, "CT", 2) == 0 || strncmp(instr->name, "DMT", 3) == 0) {
                    // MTC1, CTC1
                    instr->regsIn[instr->t] = 1;
                    if (strncmp(instr->name, "DMT", 3) == 0) {
                        if (!HOT_STATE->fr_is_set) {
                            instr->fregsOut[instr->d & ~1] = 1;
                        } else {
                            instr->fregsOut[instr->d] = 1;
                        }
                    } else if (strncmp(instr->name, "MT", 2) == 0) {
                        instr->fregsOut32[instr->d] = 1;
                    }
                } else {
                    // MFC1, CFC1
                    if (strncmp(instr->name, "MF", 2) == 0 || strncmp(instr->name, "CF", 2) == 0) {
                        instr->fregsIn32[instr->d] = 1;
                    } else if (strncmp(instr->name, "DMF", 3) == 0 || strncmp(instr->name, "DCF", 3) == 0) {
                        if (!HOT_STATE->fr_is_set) {
                            instr->fregsIn[instr->d & ~1] = 1;
                        } else {
                            instr->fregsIn[instr->d] = 1;
                        }
                    }
                    instr->regsOut[instr->t] = 1;
                }
            } else {
                if (strncmp(instr->name, "MF", 2) == 0) {
                    instr->regsOut[instr->t] = 1;
                } else if (strncmp(instr->name, "DMF", 3) == 0) {
                    instr->regsOut[instr->t] = 1;
                } else {
                    // MTC0...
                    instr->regsIn[instr->t] = 1;
                }
            }

            found++;
        }

        if (instr->has_t && instr->has_d && !instr->has_s && instr->has_k) {
            if (is_float) {
                found = 1600;
            } else {
                // SLL...
                instr->regsOut[instr->d] = 1;
                instr->regsIn[instr->t] = 1;
            }

            found++;
        }

        if (instr->has_t && !instr->has_d && !instr->has_s && !instr->has_b) {
            if (is_float) {
                found = 400;
            } else {
                instr->regsOut[instr->t] = 1;
            }

            found++;
        }

        if (instr->has_t && instr->has_b) {
            bool is_load = false;
            instr->regsIn[instr->b] = 1;
            if (instr->name[0] == 'L') {
                is_load = true;
            } else if (instr->name[0] == 'S') {
                is_load = false;
            } else found = 500;

            if (is_float) {
                if (is_load) {
                    if (instr->name[1] == 'D') {
                        if (!HOT_STATE->fr_is_set) {
                            instr->fregsOut[instr->t & ~1] = 1;
                        } else {
                            instr->fregsOut[instr->t] = 1;
                        }
                    } else if (instr->name[1] == 'W') {
                        instr->fregsOut32[instr->t] = 1;
                    } else found = 555;
                } else {
                    if (instr->name[1] == 'D') {
                        if (!HOT_STATE->fr_is_set) {
                            instr->fregsIn[instr->t & ~1] = 1;
                        } else {
                            instr->fregsIn[instr->t] = 1;
                        }
                    } else if (instr->name[1] == 'W') {
                        instr->fregsIn32[instr->t] = 1;
                    } else found = 556;
                }
            } else {
                if (is_load) {
                    // TODO: what is this?
                    if (strcmp(instr->name, "LL") == 0) {
                        instr->regsOut[instr->t] = 1;
                    }
                    // we use ReadWrite
                    if (strcmp(instr->name, "LWL") == 0 || strcmp(instr->name, "LWR") == 0) {
                        instr->regsIn[instr->t] = 1;
                    }
                    instr->regsOut[instr->t] = 1;
                } else {
                    if (strcmp(instr->name, "SC") == 0) {
                        instr->regsOut[instr->t] = 1;
                    }
                    instr->regsIn[instr->t] = 1;
                }
            }
            found++;
        }

        if (instr->has_s && !instr->has_d && !instr->has_t && !instr->has_b && !instr->has_k) {
            if (is_float) {
                found = 600;
            } else {
               instr->regsIn[instr->s] = 1;
            }

            found++;
        }

        if (instr->has_d && !instr->has_s && !instr->has_t && !instr->has_b && !instr->has_k) {
            if (is_float) {
                found = 700;
            } else {
               instr->regsOut[instr->d] = 1;
            }

            found++;
        }

        if (instr->has_k && instr->has_b) {
            // CACHE...
            if (is_float) {
                found = 800;
            } else {
                instr->regsIn[instr->b] = 1;
            }

            found++;
        }

        if (instr->has_s && instr->has_d && !instr->has_t && instr->has_a) {
            if (is_float) {
                assert(instr->has_a);
                bool out_32 = false;
                if (strlen(instr->name) >= 4 && instr->name[strlen(instr->name) - 2] == '_' && instr->name[strlen(instr->name) - 4] == '_') {
                    if (instr->name[strlen(instr->name) - 3] == 'L' || instr->name[strlen(instr->name) - 3] == 'D') {
                        out_32 = false;
                    } else if (instr->name[strlen(instr->name) - 3] == 'S' || instr->name[strlen(instr->name) - 3] == 'W') {
                        out_32 = true;
                    } else abort();
                } else {
                    if (instr->a == 0x10 || instr->a == 0x14) {
                        out_32 = true;
                    } else if (instr->a == 0x11 || instr->a == 0x15) {
                        out_32 = false;
                    } else abort();
                }
                if (instr->a == 0x10 || instr->a == 0x14) {
                    instr->fregsIn32[instr->s] = 1;
                    if (out_32) instr->fregsOut32[instr->d] = 1;
                    else instr->fregsOut[instr->d] = 1;
                } else if (instr->a == 0x11 || instr->a == 0x15) {
                    instr->fregsIn[instr->s] = 1;
                    if (!out_32) instr->fregsOut[instr->d] = 1;
                    else instr->fregsOut32[instr->d] = 1;
                } else abort();
                found++;
            } else {
                found = 1400;
            }
        }

        if (!instr->has_a && !instr->has_b && !instr->has_c && !instr->has_d && !instr->has_f && !instr->has_k && !instr->has_s && !instr->has_t && !instr->has_x) {
            // TLBWI...
            found++;
        }
    } else {
        if (instr->has_s && instr->has_d && !instr->has_t) {
            if (is_float) {
                found = 900;
            } else {
               instr->regsIn[instr->s] = 1;
               instr->regsOut[instr->d] = 1;
            }

            found++;
        }

        if (instr->has_s && !instr->has_d && !instr->has_t && instr->has_f) {
            if (is_float) {
                found = 1000;
            } else {
               instr->regsIn[instr->s] = 1;
            }

            found++;
        }

        if (!instr->has_s && !instr->has_d && !instr->has_t && !instr->has_b && instr->has_k) {
            // JAL...
            if (is_float) found = 1100;
            found++;
        }

        if (instr->has_t && instr->has_s && instr->has_f) {
            if (is_float) {
                found = 1200;
            } else {
                instr->regsIn[instr->s] = 1;
                instr->regsIn[instr->t] = 1;
            }

            found++;
        }

        if (instr->has_s && !instr->has_t && !instr->has_d && !instr->has_f && !instr->has_k) {
            // JR...
            if (is_float) {
                found = 1300;
            } else {
                instr->regsIn[instr->s] = 1;
            }

            found++;
        }

        if (instr->has_f && !instr->has_a && !instr->has_b && !instr->has_c && !instr->has_d && !instr->has_k && !instr->has_s && !instr->has_t) {
            if (is_float) {
                found++;
            } else {
                found = 1500;
            }
        }
    }

    // gpr reg0 is always 0 so consider it unused.
    instr->regsOut[0] = 0;
    instr->regsIn[0] = 0;

    if (found != 1) {
        DebugMessage(M64MSG_ERROR, "Failed to set instruction stats for: %s, %d\n", instr->name, found);
    }
}

void VR4300_Jitter::get_source_and_pagelimit(uint32_t addr, uint32_t **source, unsigned int *pagelimit, unsigned int *hard_pagelimit)
{
    unsigned int start = addr & ~3;
    if (addr >= 0xa4000000 && addr < 0xa4001000) {
        *source = (unsigned int *)((uintptr_t)g_dev.sp.mem+start-0xa4000000);
        *pagelimit = 0xa4001000;
        *hard_pagelimit = 0xa4001000;
    } else if (addr >= 0x80000000 && addr < 0x80800000) {
        *source = (unsigned int *)((uintptr_t)g_dev.rdram.dram+start-0x80000000);
        *pagelimit = 0x80800000;
        *hard_pagelimit = 0x80800000;
    } else if ((signed int)addr >= (signed int)0xC0000000) {
        *pagelimit = ((start+4096)&0xFFFFF000);
        *hard_pagelimit = ((start+4096)&0xFFFFF000);
        for (int i = 0; i < 4096; i++) {
            u32 paddr = vr4300_jitter_translate_address_no_exception(start + i * 4096, 2);
            if (paddr == 0) {
                break;
            }
            *hard_pagelimit = (start + ((i+1) * 4096))&0xFFFFF000;
        }
    } else {
        DebugMessage(M64MSG_ERROR, "Compile at bogus memory address: %x", addr);
        abort();
    }
}

bool VR4300_Jitter::is_linking_branch(struct jit_instr *op)
{
    bool does_link = false;

    switch (op->operation) {
        // branches/jumps without return (no link)
        case VR4300_OP_SYSCALL:
        case VR4300_OP_J:
        case VR4300_OP_JR:
        case VR4300_OP_BC0F:
        case VR4300_OP_BC0FL:
        case VR4300_OP_BC0T:
        case VR4300_OP_BC0TL:
        case VR4300_OP_BC1F:
        case VR4300_OP_BC1FL:
        case VR4300_OP_BC1T:
        case VR4300_OP_BC1TL:
        case VR4300_OP_BEQ:
        case VR4300_OP_BEQL:
        case VR4300_OP_BGEZ:
        case VR4300_OP_BGEZL:
        case VR4300_OP_BGTZ:
        case VR4300_OP_BGTZL:
        case VR4300_OP_BLEZ:
        case VR4300_OP_BLEZL:
        case VR4300_OP_BLTZ:
        case VR4300_OP_BLTZL:
        case VR4300_OP_BNE:
        case VR4300_OP_BNEL:
            break;
        default:
            does_link = op->is_branch_or_jump;
            break;
    }

    return does_link;
}

bool VR4300_Jitter::is_likely_branch(struct jit_instr *op)
{
    switch (op->operation) {
        case VR4300_OP_BC0FL:
        case VR4300_OP_BC0TL:
        case VR4300_OP_BC1FL:
        case VR4300_OP_BC1TL:
        case VR4300_OP_BEQL:
        case VR4300_OP_BGEZL:
        case VR4300_OP_BGTZL:
        case VR4300_OP_BLEZL:
        case VR4300_OP_BLTZL:
        case VR4300_OP_BNEL:
        case VR4300_OP_BGEZALL:
        case VR4300_OP_BLTZALL:
            return true;
            break;
        default:
            return false;
            break;
    }
}

bool VR4300_Jitter::is_nop(struct jit_instr *op) {
    switch (op->operation) {
        case VR4300_OP_SLL:
        case VR4300_OP_SRL:
        case VR4300_OP_SRA:
        case VR4300_OP_SLLV:
        case VR4300_OP_SRLV:
        case VR4300_OP_SRAV: {
            assert(op->has_d);

            // nop
            if (op->d == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_MFHI:
        case VR4300_OP_MFLO:
        case VR4300_OP_DSLLV:
        case VR4300_OP_DSRLV:
        case VR4300_OP_DSRAV: {
            assert(op->has_d);

            // nop
            if (op->d == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_ADD:
        case VR4300_OP_ADDU:
        case VR4300_OP_SUB:
        case VR4300_OP_SUBU:
        case VR4300_OP_AND:
        case VR4300_OP_OR:
        case VR4300_OP_XOR:
        case VR4300_OP_NOR:
        case VR4300_OP_SLT:
        case VR4300_OP_SLTU:
        case VR4300_OP_DADD:
        case VR4300_OP_DADDU:
        case VR4300_OP_DSUB:
        case VR4300_OP_DSUBU: {
            assert(op->has_d);

            // nop
            if (op->d == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_DSLL:
        case VR4300_OP_DSRL:
        case VR4300_OP_DSRA:
        case VR4300_OP_DSLL32:
        case VR4300_OP_DSRL32:
        case VR4300_OP_DSRA32: {
            assert(op->has_d);

            // nop
            if (op->d == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_ADDI:
        case VR4300_OP_ADDIU:
        case VR4300_OP_SLTI:
        case VR4300_OP_SLTIU:
        case VR4300_OP_ANDI:
        case VR4300_OP_ORI:
        case VR4300_OP_XORI:
        case VR4300_OP_LUI: {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_MFC0:
        case VR4300_OP_DMFC0: {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_MFC1:
        case VR4300_OP_DMFC1:
        case VR4300_OP_CFC1:
        /*case VR4300_OP_DCFC1:*/ {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_MFC2:
        case VR4300_OP_DMFC2: {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_DADDI:
        case VR4300_OP_DADDIU:
        case VR4300_OP_LDL:
        case VR4300_OP_LDR:
        case VR4300_OP_LB:
        case VR4300_OP_LH:
        case VR4300_OP_LWL:
        case VR4300_OP_LW:
        case VR4300_OP_LBU:
        case VR4300_OP_LHU:
        case VR4300_OP_LWR:
        case VR4300_OP_LWU: {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_LL: {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_LD: {
            assert(op->has_t);

            // nop
            if (op->t == 0) {
                return true;
            }
            break;
        }
        case VR4300_OP_SC: {
            // ???
            break;
        }
    }
    return false;
}

// returns true if the branch is for sure going
// to be taken
bool VR4300_Jitter::will_branch(struct jit_instr *op) {
    bool will_branch = false;
    switch (op->operation) {
        case VR4300_OP_BC0F:
            // TODO
            break;
        case VR4300_OP_BC0FL:
            // TODO
            break;
        case VR4300_OP_BC0T:
            // TODO
            break;
        case VR4300_OP_BC0TL:
            // TODO
            break;
        case VR4300_OP_BC1F:
            // TODO
            break;
        case VR4300_OP_BC1FL:
            // TODO
            break;
        case VR4300_OP_BC1T:
            // TODO
            break;
        case VR4300_OP_BC1TL:
            // TODO
            break;
        case VR4300_OP_BEQ:
            will_branch = op->s == op->t;
            break;
        case VR4300_OP_BEQL:
            will_branch = op->s == op->t;
            break;
        case VR4300_OP_BGEZ:
            will_branch = !op->s;
            break;
        case VR4300_OP_BGEZAL:
            will_branch = !op->s;
            break;
        case VR4300_OP_BGEZALL:
            will_branch = !op->s;
            break;
        case VR4300_OP_BGEZL:
            will_branch = !op->s;
            break;
        case VR4300_OP_BGTZ:
            break;
        case VR4300_OP_BGTZL:
            break;
        case VR4300_OP_BLEZ:
            will_branch = !op->s;
            break;
        case VR4300_OP_BLEZL:
            will_branch = !op->s;
            break;
        case VR4300_OP_BLTZ:
            break;
        case VR4300_OP_BLTZAL:
            break;
        case VR4300_OP_BLTZALL:
            break;
        case VR4300_OP_BLTZL:
            break;
        case VR4300_OP_BNE:
            break;
        case VR4300_OP_BNEL:
            break;
        case VR4300_OP_J:
            will_branch = true;
            break;
        case VR4300_OP_JAL:
            will_branch = true;
            break;
        case VR4300_OP_JALR:
            will_branch = true;
            break;
        case VR4300_OP_JR:
            will_branch = true;
            break;
    };

    return will_branch;
}

void VR4300_Jitter::set_cp0_register_modification_stats(struct prepared_code_block *code_block, int i)
{
    if (code_block->instr[i].operation == VR4300_OP_MTC0) {
        if (code_block->instr[i].d == CP0_COUNT_REG) {
            code_block->modifies_count_reg = true;
            code_block->instr[i].modifies_count_reg = true;
        } else if (code_block->instr[i].d == CP0_STATUS_REG) {
            code_block->modifies_status_reg = true;
            code_block->instr[i].modifies_status_reg = true;
        }
    }
}

bool VR4300_Jitter::is_jump(struct jit_instr *op)
{
    switch (op->operation) {
        case VR4300_OP_J:
        case VR4300_OP_JR:
        case VR4300_OP_JAL:
        case VR4300_OP_JALR:
            return true;
            break;
        default:
            return false;
            break;
    }
}

unsigned int VR4300_Jitter::Analyze(unsigned int addr, struct prepared_code_block *code_block, int max_instr)
{
    unsigned int start = addr & ~3;
    uint32_t *source;
    unsigned int pagelimit = 0;
    unsigned int hard_pagelimit = 0;

    unsigned int analyzerPC = start;
    code_block->stopped_early = true;
    code_block->exception = false;
    code_block->is_idle_wait_loop = false;
    code_block->branch_to = 0;
    bool is_extended = false;
    bool last_was_follow = false;
    last_was_follow = false;

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "ANALYZING NEW BLOCK\n");
#endif

    get_source_and_pagelimit(addr, &source, &pagelimit, &hard_pagelimit);

    int extensions = 0;
    int idx = 0;
    uint32_t follows = 0;
    uint32_t link_continues = 0;

    // for restoring after rewinding from follow
    uint32_t last_analyzerPC = 0;
    unsigned int last_num_instructions = 0;

    for (int i = 0; i < max_instr - 1; i++, idx++, analyzerPC += 4) {
        if (vr4300_jitter_address_needs_translation(analyzerPC)) {
            u32 paddr = vr4300_jitter_translate_address_no_exception(analyzerPC, 2);
            source = mem_base_u32(m_r4300->mem->base, paddr);
            idx = 0;
            if (paddr == 0) {
                if (is_extended) {
                    analyzerPC -= 4;
                    break;
                }
                code_block->exception = true;
#ifndef NDEBUG
                fprintf(stderr, "EXCEPTION BLOCK!\n");
#endif
                analyzerPC -= 4;
                break;
            }
        }

        if (analyzerPC >= pagelimit) {
            code_block->crosses_page = true;
        }

        if (analyzerPC >= hard_pagelimit) {
#ifndef NDEBUG
            fprintf(stderr, "reached pagelimit %x %x %d\n", start, pagelimit, i);
#endif
            if (is_extended) {
                analyzerPC -= 4;
                break;
            }

            code_block->exception = true;
            break;
        }

        if (!AnalyzeInstruction(&code_block->instr[i], source[idx], analyzerPC)) {
            analyzerPC -= 4;
            break;
        }

#ifndef NDEBUG
        DebugMessage(M64MSG_VERBOSE, "0x%x@0x%x: %s\n", source[idx], analyzerPC, code_block->instr[i].name);
#endif

        bool do_extend = false;
        bool does_link = is_linking_branch(&code_block->instr[i]);
        if (code_block->instr[i].is_branch_or_jump && !does_link) {
            // if the branch is unconditional, there might not be valid
            // code after it.
            do_extend = !will_branch(&code_block->instr[i]);
        }

        set_cp0_register_modification_stats(code_block, i);

        code_block->instr[i].is_delay_slot = false;
        code_block->instr[i].regsIn = BitSet32{};
        code_block->instr[i].fregsIn = BitSet32{};
        code_block->instr[i].fregsIn32 = BitSet32{};
        code_block->instr[i].regsInUse = BitSet32{};
        code_block->instr[i].fregsInUse = BitSet32{};
        code_block->instr[i].fregsInUse32 = BitSet32{};
        code_block->instr[i].regsOut = BitSet32{};
        code_block->instr[i].fregsOut = BitSet32{};
        code_block->instr[i].fregsOut32 = BitSet32{};

        code_block->num_instructions++;

        if (code_block->instr[i].modifies_status_reg) {// || code_block->instr[i].modifies_count_reg) {
            break;
        }

        if (code_block->instr[i].operation == VR4300_OP_ERET) {
            break;
        }

        if (code_block->instr[i].operation == VR4300_OP_BREAK || code_block->instr[i].operation == VR4300_OP_SYSCALL) {
            break;
        }

        if (code_block->instr[i].is_branch_or_jump) {
            if (vr4300_jitter_address_needs_translation(analyzerPC + 4)) {
                u32 paddr = vr4300_jitter_translate_address_no_exception(analyzerPC + 4, 2);
                source = mem_base_u32(m_r4300->mem->base, paddr);
                idx = -1;

                if (paddr == 0) {
                    if (is_extended) {
                        break;
                    }
                    code_block->exception = true;
#ifndef NDEBUG
                    fprintf(stderr, "EXCEPTION BLOCK DS!\n");
#endif
                    analyzerPC += 4;
                    break;
                }
            }

            if (analyzerPC + 4 >= pagelimit) {
                code_block->crosses_page = true;
            }

            if ((analyzerPC + 4) >= hard_pagelimit) {
#ifndef NDEBUG
                fprintf(stderr, "reached pagelimit ds %x %x %d\n", start, pagelimit, i);
#endif

                if (is_extended) {
                    break;
                }

                analyzerPC += 4;
                code_block->exception = true;
                break;
            }

            if (i + 1 < max_instr) {
                // delay slot
                if (!AnalyzeInstruction(&code_block->instr[i + 1], source[idx + 1], analyzerPC + 4)) {
                    break;
                }

                if (code_block->instr[i + 1].is_branch_or_jump) {
                    // bad extension?
                    break;
                }

                set_cp0_register_modification_stats(code_block, i + 1);

                if (code_block->instr[i + 1].modifies_status_reg) {// || code_block->instr[i + 1].modifies_count_reg) {
                    break;
                }

                if (code_block->instr[i + 1].operation == VR4300_OP_ERET) {
                    break;
                }

                if (code_block->instr[i + 1].operation == VR4300_OP_BREAK || code_block->instr[i + 1].operation == VR4300_OP_SYSCALL) {
                    break;
                }


#ifndef NDEBUG
                DebugMessage(M64MSG_VERBOSE, "SKIP (delay slot): 0x%x@0x%x: %s\n", source[idx + 1], analyzerPC + 4, code_block->instr[i + 1].name);
#endif

                code_block->instr[i+1].is_delay_slot = true;
                code_block->instr[i+1].regsIn = BitSet32{};
                code_block->instr[i+1].fregsIn = BitSet32{};
                code_block->instr[i+1].fregsIn32 = BitSet32{};
                code_block->instr[i+1].regsInUse = BitSet32{};
                code_block->instr[i+1].fregsInUse = BitSet32{};
                code_block->instr[i+1].fregsInUse32 = BitSet32{};
                code_block->instr[i+1].regsOut = BitSet32{};
                code_block->instr[i+1].fregsOut = BitSet32{};
                code_block->instr[i+1].fregsOut32 = BitSet32{};

                code_block->num_instructions++;
            } else {
                break;
            }

            if (link_continues < MAX_LINK_CONTINUE && (does_link && !is_jump(&code_block->instr[i])) && i < LINK_CONTINUE_INSTRUCTION_THRESHOLD) {
                i++;
                if (i < max_instr - 1) {
                    code_block->instr[i].next_is_link_continue = true;
                    idx++;
                    last_was_follow = false;
                    is_extended = true;
                    analyzerPC += 4;
                    link_continues++;
                } else abort();
            } else if (do_extend && extensions < MAX_BLOCK_EXTENSIONS && i < EXTEND_INSTRUCTION_THRESHOLD) {
                i++;
                if (i < max_instr - 1) {
                    code_block->instr[i].next_is_extend = true;
                    idx++;
                    extensions++;
                    last_was_follow = false;
                    is_extended = true;
                    analyzerPC += 4;
                } else abort();
            } else if (follows < MAX_BRANCH_FOLLOWS && i < FOLLOW_INSTRUCTION_THRESHOLD && will_branch(&code_block->instr[i]) && !is_linking_branch(&code_block->instr[i]) && (code_block->instr[i].operation != VR4300_OP_JR) && !is_nop(&code_block->instr[i + 1])) {
                uint32_t target;
                if (code_block->instr[i].operation == VR4300_OP_J) {
                    assert(code_block->instr[i].has_k);
                    if (!code_block->instr[i].has_k) abort();
                    uint32_t high_order_bits = ((code_block->instr[i].address + 4) & 0xf0000000);
                    target = high_order_bits | ((u32)(code_block->instr[i].k & 0x3FFFFFF) << 2);
                } else {
                    assert(code_block->instr[i].has_f);
                    if (!code_block->instr[i].has_f) abort();
                    target = code_block->instr[i].address + (s32)((s16)(code_block->instr[i].f + 1) * 4);
                }

                // TODO: instead of not following, embed a valid block check in that spot?
                bool dont_follow = ((target < (addr & 0xFFFFF000)) || (target >= ((addr & 0xFFFFF000) + 0x1000)));
                if (!dont_follow) {
                    for (int j = 0; j < i; j++) {
                        if (code_block->instr[j].address == target) {
                            dont_follow = true;
                            break;
                        }
                    }
                }
                if (dont_follow) {
                    code_block->stopped_early = false;
                    break;
                }

                last_analyzerPC = analyzerPC + 4;
                last_num_instructions = code_block->num_instructions;
                i++;

                if (i < max_instr - 1) {
                    code_block->instr[i].next_is_follow = true;
                    analyzerPC = target - 4;
                    last_was_follow = true;
                    is_extended = true;

                    get_source_and_pagelimit(analyzerPC, &source, &pagelimit, &hard_pagelimit);
                    idx = 0;
                    follows++;
                } else abort();
            } else {
                code_block->stopped_early = false;
                break;
            }
        }
    }

    struct jit_instr *last_instruction = &code_block->instr[code_block->num_instructions - 1];
#if !DISABLE_IDLE_SKIPPING
    struct jit_instr *second_to_last_instruction = &code_block->instr[code_block->num_instructions - 2];
    if (code_block->num_instructions >= 2 && second_to_last_instruction->is_branch_or_jump) {
        if (is_nop(last_instruction)) {
            if (second_to_last_instruction->name[0] == 'B') {
                assert(second_to_last_instruction->has_f);
                code_block->branch_to = second_to_last_instruction->address + (s32)((s16)(second_to_last_instruction->f + 1) * 4);
            } else if (second_to_last_instruction->operation == VR4300_OP_J) {
                // JAL is excluded here.
                assert(second_to_last_instruction->has_k);
                uint32_t high_order_bits = ((second_to_last_instruction->address + 4) & 0xf0000000);
                uint32_t target = high_order_bits | ((u32)(second_to_last_instruction->k & 0x3FFFFFF) << 2);
                code_block->branch_to = target;
            }

            if (code_block->branch_to && code_block->branch_to == second_to_last_instruction->address) {
                if (code_block->num_instructions == 2) {
#ifndef NDEBUG
                    DebugMessage(M64MSG_VERBOSE, "FOUND IDLE WAIT LOOP AT 0x%08x\n", code_block->instr[0].address);
#endif
                    code_block->is_idle_wait_loop = true;
                } else if (is_extended) {
#ifndef NDEBUG
                    DebugMessage(M64MSG_VERBOSE, "FOUND IDLE WAIT LOOP AT END OF 0x%08x 0x%08x\n", code_block->instr[0].address, second_to_last_instruction->address);
#endif
                    if (!last_was_follow) {
                        code_block->num_instructions -= 2;
                        analyzerPC -= 8;
                    } else {
                        code_block->num_instructions = last_num_instructions;
                        analyzerPC = last_analyzerPC;
                    }
                } else {
                    code_block->num_instructions -= 2;
                    analyzerPC -= 4;
                    code_block->stopped_early = true;
                }
            }
        }
    }
#endif

    if (is_extended) {
        // rewind a bit

        if (!last_was_follow) {
            while (!code_block->instr[code_block->num_instructions - 1].is_delay_slot) {
                code_block->num_instructions--;
                analyzerPC -= 4;
            }
        } else {
            if (!code_block->instr[code_block->num_instructions - 1].is_delay_slot) {
                code_block->num_instructions = last_num_instructions;
                analyzerPC = last_analyzerPC;
                if (!code_block->instr[code_block->num_instructions - 1].is_delay_slot) {
                    abort();
                }
            }
        }
    }

    assert(code_block->num_instructions);

    BitSet32 regsInUse = BitSet32{};
    BitSet32 fregsInUse = BitSet32{};
    BitSet32 regsDiscardable = BitSet32{};
    BitSet32 fregsDiscardable = BitSet32{};

    bool first_float_instruction = true;
    bool first_ctc2_instruction = true;

    for (int i = 0; i < code_block->num_instructions; i++) {
        struct jit_instr *instr = &code_block->instr[i];

        if ((instr->has_x && instr->x == 1) || instr->has_a) {
            instr->is_first_float_instruction = first_float_instruction;
            first_float_instruction = false;
        }

        if (instr->is_delay_slot) {
            first_float_instruction = true;
        }

        if (instr->has_x && instr->x == 2) {
            instr->is_first_ctc2_instruction = first_ctc2_instruction;
            first_ctc2_instruction = false;
        }

        if (instr->is_delay_slot) {
            first_ctc2_instruction = true;
        }
    }

    for (int i = code_block->num_instructions - 1; i >= 0; i--) {
        struct jit_instr *instr_real = &code_block->instr[i];
        struct jit_instr *instr = NULL;
        if (instr_real->is_delay_slot && i > 0) {
            instr = &code_block->instr[i - 1];
            if (!instr->is_branch_or_jump) abort();
        }
        if (instr_real->is_branch_or_jump && i + 1 < code_block->num_instructions) {
            instr = &code_block->instr[i + 1];
            if (!instr->is_delay_slot) abort();
        }
        if (!instr_real->is_branch_or_jump && !instr_real->is_delay_slot) {
            instr = instr_real;
        }

        if (!instr && instr_real->is_branch_or_jump) {
            // missing delay slot..
            instr = instr_real;
        }
        if (!instr) abort();

        set_instruction_stats(instr);
        if (instr != instr_real) set_instruction_stats(instr_real);
        instr->may_cause_exception = false;

        instr->regsInUse = regsInUse;
        instr->fregsInUse = fregsInUse;
        instr->regsDiscardable = regsDiscardable;
        instr->fregsDiscardable = fregsDiscardable;

        regsInUse |= instr->regsIn | instr->regsOut;
        fregsInUse |= instr->fregsIn | instr->fregsOut | instr->fregsIn32 | instr->fregsOut32;

        if ((((instr->has_x && (instr->x == 1)) || instr->has_a) && instr->is_first_float_instruction)
                || instr->is_first_ctc2_instruction
                || code_block->modifies_status_reg
                || code_block->modifies_count_reg
                || instr->operation == VR4300_OP_ERET
                || instr->operation == VR4300_OP_MTC0
                || instr->operation == VR4300_OP_DMTC0
                || instr->operation == VR4300_OP_SYSCALL
                || instr->operation == VR4300_OP_TLBWR // for exceptionless_block opt
                || instr->is_branch_or_jump
                || instr->has_b /* loadstore */
                || instr->operation == VR4300_OP_DCFC1
                || instr->operation == VR4300_OP_DCTC1
                // We always insert a check at page boundaries.
                || (vr4300_jitter_address_needs_translation(instr->address) && ((instr->address & 0xFFF) == 0))
                // || (i > 1 && code_block->instr[i - 1].is_delay_slot && is_likely_branch(&code_block->instr[i - 2]) && ((instr->address & 0xFFF) == 4))
                ) {
            instr->may_cause_exception = true;
        }

        if (instr->may_cause_exception || instr->is_delay_slot) {
            regsDiscardable = BitSet32{};
            fregsDiscardable = BitSet32{};
        } else {
            regsDiscardable |= instr->regsOut;
            regsDiscardable &= ~instr->regsIn;

            if (!HOT_STATE->fr_is_set) {
                fregsDiscardable |= instr->fregsOut;
                for (preg_t p : BitSet32::AllTrue(32)) {
                    if ((fregsDiscardable[(p & ~1) + 1] && instr->fregsOut32[(p & ~1)]) || (fregsDiscardable[(p & ~1)] && instr->fregsOut32[(p & ~1) + 1])) {
                        fregsDiscardable[p & ~1] = true;
                    }
                }
                for (preg_t p : BitSet32::AllTrue(32)) {
                    if (instr->fregsIn32[(p & ~1)] || instr->fregsIn32[(p & ~1) + 1] || instr->fregsIn[(p & ~1)]) {
                        fregsDiscardable[p & ~1] = false;
                    }
                }
            } else {
                // TODO: fregsOut32?
                fregsDiscardable |= instr->fregsOut;
                fregsDiscardable &= ~(instr->fregsIn | instr->fregsIn32);
            }
        }
    }

    BitSet32 scanRegs = BitSet32{}, scanRegs32 = BitSet32{};
    code_block->exceptionless_block = true;

    for (int i = 0; i < code_block->num_instructions; i++) {
        struct jit_instr *instr = &code_block->instr[i];

        if (!instr->is_branch_or_jump) {
            code_block->exceptionless_block &= !instr->may_cause_exception;
        }

        BitSet32 fregsIncompatible = BitSet32::AllTrue(0);
        BitSet32 fregsIncompatible32 = BitSet32::AllTrue(0);

        scanRegs &= ~instr->fregsDiscardable;
        scanRegs32 &= ~instr->fregsDiscardable;

        // for each of those float reg that are used as 64
        for (preg_t p : instr->fregsOut | scanRegs) {
            // check if the next instruction that uses it is using it in a different mode
            for (int j = i + 1; j < code_block->num_instructions; j++) {
                struct jit_instr *instr2 = &code_block->instr[j];
                preg_t p2 = p;
                if (!HOT_STATE->fr_is_set) {
                    if (p & 1) {
                        p2 = p2 & ~1;
                    } else {
                        p2 = (p2 & ~1) + 1;
                    }
                }
                if (instr2->fregsDiscardable[p] || instr2->fregsDiscardable[p2]) {
                    scanRegs[p] = false;
                    scanRegs[p2] = false;
                    break;
                }
                if (instr2->fregsIn32[p] || instr2->fregsOut32[p] || instr2->fregsIn32[p2] || instr2->fregsOut32[p2]) {
                    scanRegs[p] = false;
                    fregsIncompatible32[p] = true;
                    break;
                }
                if (instr2->fregsIn[p] || instr2->fregsIn[p2]) {
                    scanRegs[p] = true;
                    break;
                }
                if (instr2->fregsOut[p] || instr2->fregsOut[p2]) {
                    scanRegs[p] = false;
                    break;
                }
            }
        }

        BitSet32 regsToCheck32 = BitSet32{};
        if (!HOT_STATE->fr_is_set) {
            for (preg_t p : instr->fregsOut32 | scanRegs32) {
                regsToCheck32[p & ~1] = true;
                regsToCheck32[(p & ~1) + 1] = true;
            }
        } else {
            regsToCheck32 = instr->fregsOut32 | scanRegs32;
        }
        // for each of those float regs that are used as 32
        for (preg_t p : regsToCheck32) {
            // check if the next instruction that uses it is using it in a different mode
            for (int j = i + 1; j < code_block->num_instructions; j++) {
                struct jit_instr *instr2 = &code_block->instr[j];
                preg_t p2 = p;
                if (!HOT_STATE->fr_is_set) {
                    if (p & 1) {
                        p2 = p2 & ~1;
                    } else {
                        p2 = (p2 & ~1) + 1;
                    }
                }
                if (instr2->fregsDiscardable[p] || instr2->fregsDiscardable[p2]) {
                    scanRegs32[p] = false;
                    scanRegs32[p2] = false;
                    break;
                }
                if (instr2->fregsIn[p] || instr2->fregsOut[p] || instr2->fregsIn[p2] || instr2->fregsOut[p2]) {
                    scanRegs32[p] = false;
                    scanRegs32[p2] = false;
                    fregsIncompatible[p] = true;
                    fregsIncompatible[p2] = true;
                    break;
                }
                if (instr2->fregsIn32[p] || instr2->fregsIn32[p2]) {
                    scanRegs32[p] = true;
                    scanRegs32[p2] = true;
                    break;
                }
                if (instr2->fregsOut32[p] || instr2->fregsOut32[p2]) {
                    scanRegs32[p] = false;
                    scanRegs32[p2] = false;
                    break;
                }
            }
        }

        instr->fregsIncompatible = fregsIncompatible;
        instr->fregsIncompatible32 = fregsIncompatible32;
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "FOLLOWS: %d LINKCONTINUES %d EXTS: %d\n", follows, link_continues, extensions);
#endif

    if (code_block->stopped_early) {
        return analyzerPC + 4;
    } else {
        return analyzerPC + 4 + 4;
    }
}

void VR4300_Jitter::zero_hot_cycles(void)
{
    MOV(32, HOTSTATE_VAR(hot_cycles), Imm32(0));
}

void VR4300_Jitter::update_hot_cycles(struct jit_instr *op, bool store_pc)
{
    if (store_pc) {
        MOV(32, HOTSTATE_VAR(pc), Imm32(op->address + 4));
#ifdef COMPARE_CORE
        MOV(32, HOTSTATE_VAR(dbg_pc), Imm32(op->address + 4));
#endif
    }
    MOV(32, HOTSTATE_VAR(hot_cycles), Imm32(HOT_STATE->compiler_cycles + m_r4300->cp0.count_per_op));
}

void vr4300_jitter_fix_hot_cycles(void)
{
    VR4300_Jitter::GetInstance()->FixHotCycles();
}

void VR4300_Jitter::FixHotCycles(void)
{
#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "UPDATING CYCLES: hs %d ca %d cc %d ds %d\n", HOT_STATE->hot_cycles, HOT_STATE->cycles_added, HOT_STATE->cycle_count, HOT_STATE->delay_slot);
#endif
    if (!HOT_STATE->hot_cycles) return;
    HOT_STATE->cycles_added += HOT_STATE->hot_cycles;
    HOT_STATE->cycle_count += HOT_STATE->cycles_added;
    HOT_STATE->hot_cycles = 0;
}

void VR4300_Jitter::update_count_reg(void)
{
    MOV(32, R(RSCRATCH), HOTSTATE_VAR(next_interrupt));
    ADD(32, R(RSCRATCH), HOTSTATE_VAR(cycle_count));
    MOV(32, HOTSTATE_CP0REG(CP0_COUNT_REG), R(RSCRATCH));
}

void vr4300_jitter_core_compare_copy_registers_to_tmp_now()
{
#ifdef COMPARE_CORE
    memcpy(&HOT_STATE->fprs_tmp[0], &HOT_STATE->cp1_regs[0], 32 * 8);
    memcpy(&HOT_STATE->gprs_tmp[0], &HOT_STATE->regs[0], 32 * 8);
#endif
}

static void PrintPC(u32 pc, const char *s, bool cc_checks, bool idle_loop)
{
    if (!skipped_log) {
        DebugMessage(M64MSG_VERBOSE, "PC: 0x%x, %s %s %s\n", pc, s, cc_checks ? "CC_CHECKS" : "", idle_loop ? "IDLE" : "");
    }
}

static void CopyRegisterRangeFPR(int from, int count)
{
    memcpy(&HOT_STATE->fprs_tmp[from], &HOT_STATE->cp1_regs[from], count * 8);
}

static void CopyRegisterRangeGPR(int from, int count)
{
    memcpy(&HOT_STATE->gprs_tmp[from], &HOT_STATE->regs[from], count * 8);
}

void VR4300_Jitter::core_compare_copy_registers_to_tmp()
{
    int last_discarded_gpr = -1;
    int last_discarded_fpr = -1;
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (m_fpr.IsDiscarded(k)) {
                int from = last_discarded_fpr + 1;
                int count = k - from;
                ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                MOV(64, R(ABI_PARAM1), Imm32(from));
                MOV(64, R(ABI_PARAM2), Imm32(count));
                ABI_CallFunction(CopyRegisterRangeFPR);
                ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
                last_discarded_fpr = k;
            }
            if (m_gpr.IsDiscarded(k)) {
                int from = last_discarded_gpr + 1;
                int count = k - from;
                ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                MOV(64, R(ABI_PARAM1), Imm32(from));
                MOV(64, R(ABI_PARAM2), Imm32(count));
                ABI_CallFunction(CopyRegisterRangeGPR);
                ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
                last_discarded_gpr = k;
            }
        }
    }
    if (last_discarded_fpr == -1) {
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        MOV(64, R(ABI_PARAM1), Imm32(0));
        MOV(64, R(ABI_PARAM2), Imm32(32));
        ABI_CallFunction(CopyRegisterRangeFPR);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    } else {
        int from = last_discarded_fpr + 1;
        if (from != 32) {
            int count = 32 - from;
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV(64, R(ABI_PARAM1), Imm32(from));
            MOV(64, R(ABI_PARAM2), Imm32(count));
            ABI_CallFunction(CopyRegisterRangeFPR);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
        }
    }

    if (last_discarded_gpr == -1) {
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        MOV(64, R(ABI_PARAM1), Imm32(0));
        MOV(64, R(ABI_PARAM2), Imm32(32));
        ABI_CallFunction(CopyRegisterRangeGPR);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    } else {
        int from = last_discarded_gpr + 1;
        if (from != 32) {
            int count = 32 - from;
            ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
            MOV(64, R(ABI_PARAM1), Imm32(from));
            MOV(64, R(ABI_PARAM2), Imm32(count));
            ABI_CallFunction(CopyRegisterRangeGPR);
            ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
        }
    }
}

void VR4300_Jitter::save_discarded_registers_for_core_compare(struct jit_instr *op)
{
    for (preg_t p : op->regsDiscardable) {
        if (m_gpr.IsBound(p) && p && m_gpr.IsDirty(p)) {
            RCOpArg reg = (p != 0) ? m_gpr.Use(p, RCMode::Read) : RCOpArg::Imm64(0);
            RegCache::Realize(reg);

            MOV(64, R(RSCRATCH2), reg);
            MOV(64, HOTSTATE_ARRAY(gprs_tmp, p), R(RSCRATCH2));
        } else if (p && !m_gpr.IsDiscarded(p) && m_gpr.IsImm(p)) {
            MOV(64, R(RSCRATCH2), Imm64(m_gpr.Imm64(p)));
            MOV(64, HOTSTATE_ARRAY(gprs_tmp, p), R(RSCRATCH2));
        }
    }
}

void VR4300_Jitter::save_discarded_registers_for_core_compare_float(struct jit_instr *op)
{
    for (preg_t k : op->fregsDiscardable) {
        if (m_fpr.IsBound(k) && m_fpr.IsDirty(k)) {
            RCX64Reg reg = m_fpr.Bind(k, RCMode::Read, m_fpr.Is32BitOnly(k));
            RegCache::Realize(reg);

            if (m_fpr.Is32BitOnly(k)) {
                if (HOT_STATE->fr_is_set) {
                    MOVSS(HOTSTATE_CP1REG32FR_TMP(k), reg);
                } else {
                    MOVSS(HOTSTATE_CP1REG32NOFR_TMP(k), reg);
                }
            } else {
                MOVSD(HOTSTATE_ARRAY(fprs_tmp, k), reg);
            }
        }
    }
}

void VR4300_Jitter::do_core_compare(struct jit_instr *op, u32 pc)
{
#ifdef COMPARE_CORE
    if (pc) {
        MOV(32, HOTSTATE_VAR(dbg_pc), Imm32(pc));
    } else {
        MOV(32, R(RSCRATCH), HOTSTATE_VAR(pc));
        MOV(32, HOTSTATE_VAR(dbg_pc), R(RSCRATCH));
    }
    core_compare_copy_registers_to_tmp();
#if DISABLE_FLOAT_REG_CACHE
    m_fpr.Flush();
#else
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (m_fpr.IsBound(k) && m_fpr.IsDirty(k)) {
                RCX64Reg reg = m_fpr.Bind(k, RCMode::Read, m_fpr.Is32BitOnly(k));
                RegCache::Realize(reg);

                if (m_fpr.Is32BitOnly(k)) {
                    if (HOT_STATE->fr_is_set) {
                        MOVSS(HOTSTATE_CP1REG32FR_TMP(k), reg);
                    } else {
                        MOVSS(HOTSTATE_CP1REG32NOFR_TMP(k), reg);
                    }
                } else {
                    MOVSD(HOTSTATE_ARRAY(fprs_tmp, k), reg);
                }
            }
        }
    }
#endif
#if DISABLE_GPR_REG_CACHE
    m_gpr.Flush();
#else
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            int k = i * 8 + j;
            if (m_gpr.IsBound(k) && k && m_gpr.IsDirty(k)) {
                RCX64Reg reg = m_gpr.Bind(k, RCMode::Read);
                RegCache::Realize(reg);
                MOV(64, HOTSTATE_ARRAY(gprs_tmp, k), reg);
            } else if (k && !m_gpr.IsDiscarded(k) && m_gpr.IsImm(k)) {
                MOV(64, R(RSCRATCH2), Imm64(m_gpr.Imm64(k)));
                MOV(64, HOTSTATE_ARRAY(gprs_tmp, k), R(RSCRATCH2));
            }
        }
    }
#endif
#endif
#if defined(COMPARE_CORE) || PRINT_IM_HERE_DEBUG
    PUSH(RAX);
    CALL(m_core_compare);
    POP(RAX);
#endif
}

void VR4300_Jitter::update_cycle_count(struct jit_instr *op, bool no_add)
{
    if (HOT_STATE->exceptionless_block) {
        ADD(32, HOTSTATE_VAR(cycle_count), Imm32(HOT_STATE->compiler_cycles + ((no_add ? 1 : 2) * m_r4300->cp0.count_per_op)));
    } else {
        MOV(32, R(RSCRATCH), Imm32(HOT_STATE->compiler_cycles + ((no_add ? 1 : 2) * m_r4300->cp0.count_per_op)));
        SUB(32, R(RSCRATCH), HOTSTATE_VAR(cycles_added));
        ADD(32, HOTSTATE_VAR(cycle_count), R(RSCRATCH));
        MOV(32, HOTSTATE_VAR(cycles_added), Imm32(0));
    }

    update_count_reg();
}

void vr4300_jitter_invalidate_cached_code(struct r4300_core* r4300, uint32_t address, size_t length)
{
    if (address == 0 && length == 0) {
        DebugMessage(M64MSG_VERBOSE, "CLEARING ALL BLOCKS 2\n");
        VR4300_Jitter::GetInstance()->ClearCache();
        return;
    }

    VR4300_Jitter::GetInstance()->InvalidateCachedCode(address, length);
}

void vr4300_jitter_invalidate_cached_code_just_erase(struct r4300_core* r4300, uint32_t address, size_t length)
{
    VR4300_Jitter::GetInstance()->InvalidateCachedCodeJustErase(address, length);
}

void VR4300_Jitter::compile_invalidate_code_constaddress(uint32_t address, uint32_t length)
{
    if (!vr4300_jitter_is_rdram_address(address)) return;

    address &= MEMORY_MASK;

    MOV(64, R(RSCRATCH), ImmPtr(m_valid_block_ptr));
    TEST(8, MDisp(RSCRATCH, address >> 2), Imm8(1));

    FixupBranch invalidate_needed = J_CC(CC_NZ, XEmitter::Jump::Near);
    switch_to_far_code();
    SetJumpTarget(invalidate_needed);

    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    mov_3(64, ABI_PARAM1, ABI_PARAM2, ABI_PARAM3, RCOpArg::Imm64((u64)m_r4300), RCOpArg::Imm64(address), RCOpArg::Imm64(length));
    ABI_CallFunction(vr4300_jitter_invalidate_cached_code_just_erase);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    FixupBranch exit = J(XEmitter::Jump::Near);
    switch_to_near_code();
    SetJumpTarget(exit);
}

void VR4300_Jitter::compile_invalidate_code(const RCOpArg &addr, uint32_t addr_offset, const RCX64Reg &scratch)
{
    if (addr.IsImm()) {
        u32 address = (u32)addr.Imm64() + addr_offset;
        if (vr4300_jitter_address_needs_translation(address)) {
            address = vr4300_jitter_translate_address_no_exception(address, 2) & MEMORY_MASK;
        }

        compile_invalidate_code_constaddress(address);
    } else {
        BitSet32 registers_in_use;

        // Don't use RSCRATCH here, some callers pass it in as address.

        MOV(64, R(scratch), ImmPtr(m_valid_block_ptr));
        MOV_sum(32, RSCRATCH2, addr, Imm32(addr_offset));

        // Is it an rdram address?
        TEST(32, R(RSCRATCH2), Imm32(0x5f800000));
        FixupBranch not_rdram_address = J_CC(CC_NZ);

        SHR(32, R(RSCRATCH2), Imm8(2));
        // NOTE: Speedup: check the virtual addresses directly, see JitCache.
        // In theory we should first convert them to a physical address
        // instead.
        TEST(8, MRegSum(scratch, RSCRATCH2), Imm8(1));
        FixupBranch invalidate_needed_virtual = J_CC(CC_NZ, XEmitter::Jump::Near);
        switch_to_far_code();
        SetJumpTarget(invalidate_needed_virtual);
        SHL(32, R(RSCRATCH2), Imm8(2));

        // Address translation
        MOV(32, R(scratch), R(RSCRATCH2));
        AND(32, R(scratch), Imm32(0xC0000000));
        CMP(32, R(scratch), Imm32(0x80000000));

        FixupBranch dont_need_translation = J_CC(CC_E);

        MOV(64, R(scratch), ImmPtr(&m_r4300->cp0.tlb.LUT_r[0]));
        SHR(32, R(RSCRATCH2), Imm8(12));
        MOV(32, R(RSCRATCH2), MComplex(scratch, RSCRATCH2, SCALE_4, 0));
        AND(32, R(RSCRATCH2), Imm32(0xFFFFF000));
        MOV_sum(32, scratch, addr, Imm32(addr_offset));
        AND(32, R(scratch), Imm32(0xFFF));
        OR(32, R(RSCRATCH2), R(scratch));

        SetJumpTarget(dont_need_translation);

        MOV(64, R(scratch), ImmPtr(m_valid_block_ptr));
        AND(32, R(RSCRATCH2), Imm32(MEMORY_MASK));
        SHR(32, R(RSCRATCH2), Imm8(2));

        // Actual test:
        TEST(8, MRegSum(scratch, RSCRATCH2), Imm8(1));
        FixupBranch no_invalidate_needed = J_CC(CC_Z, XEmitter::Jump::Near);

        registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_3(64, ABI_PARAM1, ABI_PARAM2, ABI_PARAM3, RCOpArg::Imm64((u64)m_r4300), addr, RCOpArg::Imm64(4));
        if (addr_offset) ADD(32, R(ABI_PARAM2), Imm32(addr_offset));
        AND(32, R(ABI_PARAM2), Imm32(MEMORY_MASK));
        ABI_CallFunction(vr4300_jitter_invalidate_cached_code_just_erase);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

        FixupBranch exit = J(XEmitter::Jump::Near);
        switch_to_near_code();
        SetJumpTarget(exit);
        SetJumpTarget(not_rdram_address);
        SetJumpTarget(no_invalidate_needed);
    }
}

void VR4300_Jitter::compile_goto_dispatcher_destinhotstate_ret()
{
#if DISABLE_CALLRET_OPTIMIZATION
    RCForkGuard gpr_guard = m_gpr.Fork();
    RCForkGuard fpr_guard = m_fpr.Fork();

    compile_goto_dispatcher_destinhotstate(NULL, false);
#else
    m_gpr.Flush();
    m_fpr.Flush();

    MOV(32, R(RSCRATCH), HOTSTATE_VAR(pc));
    CMP(32, R(RSCRATCH), MDisp(RSP, 8));

    FixupBranch no_predict = J_CC(CC_NE, XEmitter::Jump::Near);
#if DEBUG_PREDICTIONS
    BitSet32 registers_in_use = BitSet32{};
    registers_in_use[RHOTSTATE2] = true;
    registers_in_use[RHOTSTATE3] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionC(vr4300_jitter_successfully_predicted_return, 1);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
    RET();
    SetJumpTarget(no_predict);

    embed_valid_block_check_pc();
    JMP(m_dispatcher_mispredicted_ret_1, XEmitter::Jump::Near);
#endif
}

void VR4300_Jitter::compile_goto_dispatcher_ret(struct jit_instr *op, u32 address)
{
#if DISABLE_CALLRET_OPTIMIZATION
    RCForkGuard gpr_guard = m_gpr.Fork();
    RCForkGuard fpr_guard = m_fpr.Fork();

    compile_goto_dispatcher(NULL, address, false);
#else
    m_gpr.Flush();
    m_fpr.Flush();

    CMP(32, MDisp(RSP, 8), Imm32(address));
    FixupBranch no_predict = J_CC(CC_NE, XEmitter::Jump::Near);
#if DEBUG_PREDICTIONS
    BitSet32 registers_in_use = BitSet32{};
    registers_in_use[RHOTSTATE2] = true;
    registers_in_use[RHOTSTATE3] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionC(vr4300_jitter_successfully_predicted_return, 3);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
    RET();
    SetJumpTarget(no_predict);
    embed_valid_block_check_pc();
    JMP(m_dispatcher_mispredicted_ret_3, XEmitter::Jump::Near);
#endif
}

void VR4300_Jitter::compile_goto_dispatcher(struct jit_instr *op, u32 address, bool call, bool flush)
{
#if DISABLE_CALLRET_OPTIMIZATION
    call = false;
#endif

    MOV(32, HOTSTATE_VAR(pc), Imm32(address));

    JitBlock::LinkData linkData;
    linkData.exitAddress = address;
    if (vr4300_jitter_address_needs_translation(address)) {
        linkData.exitAddressPhysical = vr4300_jitter_translate_address_no_exception(address, 2) & MEMORY_MASK;
    } else {
        linkData.exitAddressPhysical = address & MEMORY_MASK;
    }
    linkData.linkStatus = false;
    linkData.call = call;

    if (flush) {
        m_gpr.Flush();
        m_fpr.Flush();
    }

    // If the target is in a different block.
    if (!op || ((address & ~0xFFF) != (op->address & ~0xFFF))) {
        embed_valid_block_check(address, true);
    }

    if (call) {
        PUSH(64, Imm32(get_address_of_nth_instruction_after(op, 2)));
        linkData.exitPtrs = GetWritableCodePtr();
        CALL(m_dispatcher_start);
        POP(RSCRATCH);

        compile_goto_dispatcher(NULL /* force a block check */, get_address_of_nth_instruction_after(op, 2), false, false);
    } else {
        linkData.exitPtrs = GetWritableCodePtr();
        JMP(m_dispatcher_start, XEmitter::Jump::Near);
    }

    JitBlock *block = (JitBlock*)HOT_STATE->curBlock;

    if (block) {
        block->linkData.push_back(linkData);
    }
}

FixupBranch VR4300_Jitter::check_pc_differs(uint32_t expected_pc)
{
    CMP(32, HOTSTATE_VAR(pc), Imm32(expected_pc));
    FixupBranch different = J_CC(CC_NE, XEmitter::Jump::Near);
    return different;
}

FixupBranch VR4300_Jitter::check_pc_equals(uint32_t expected_pc)
{
    CMP(32, HOTSTATE_VAR(pc), Imm32(expected_pc));
    FixupBranch unchanged = J_CC(CC_E, XEmitter::Jump::Near);
    return unchanged;
}

void VR4300_Jitter::compile_goto_dispatcher_destinhotstate(struct jit_instr *op, bool call, bool checkstop, bool flush, u32 after)
{
#if DISABLE_CALLRET_OPTIMIZATION
    call = false;
#endif

    if (flush) {
        m_gpr.Flush();
        m_fpr.Flush();
    }

    if (call) {
        PUSH(64, Imm32(after ? after : get_address_of_nth_instruction_after(op, 2)));
        CALL(m_dispatcher_start);
        POP(RSCRATCH);

        compile_goto_dispatcher(NULL /* force a block check */, after ? after : get_address_of_nth_instruction_after(op, 2), false, false);
    } else {
        const u8 *address = checkstop ? m_dispatcher_checkstop : m_dispatcher_start;
        JMP(address, XEmitter::Jump::Near);
    }
}

extern "C" void InterpretOpcode(struct r4300_core* r4300);

void VR4300_Jitter::compile_INTERPRETER_FALLBACK(struct jit_instr *op, u32 address)
{
#ifdef COMPARE_CORE
#if !DISABLE_FLOAT_REG_CACHE
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 8; j++) {
                int k = i * 8 + j;
                if (m_fpr.IsBound(k) && m_fpr.IsDirty(k)) {
                    RCX64Reg reg = m_fpr.Bind(k, RCMode::Read, m_fpr.Is32BitOnly(k));
                    RegCache::Realize(reg);

                    if (m_fpr.Is32BitOnly(k)) {
                        if (HOT_STATE->fr_is_set) {
                            MOVSS(HOTSTATE_CP1REG32FR_TMP(k), reg);
                        } else {
                            MOVSS(HOTSTATE_CP1REG32NOFR_TMP(k), reg);
                        }
                    } else {
                        MOVSD(HOTSTATE_ARRAY(fprs_tmp, k), reg);
                    }
                }
            }
        }
#endif
#if !DISABLE_GPR_REG_CACHE
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 8; j++) {
                int k = i * 8 + j;
                if (m_gpr.IsBound(k) && k && m_gpr.IsDirty(k)) {
                    RCX64Reg reg = m_gpr.Bind(k, RCMode::Read);
                    RegCache::Realize(reg);
                    MOV(64, HOTSTATE_ARRAY(gprs_tmp, k), reg);
                } else if (k && !m_gpr.IsDiscarded(k) && m_gpr.IsImm(k)) {
                    MOV(64, R(RSCRATCH2), Imm64(m_gpr.Imm64(k)));
                    MOV(64, HOTSTATE_ARRAY(gprs_tmp, k), R(RSCRATCH2));
                }
            }
        }
    }
#endif
#endif

    m_gpr.Flush();
    m_fpr.Flush();

    MOV(64, R(RSCRATCH), ImmPtr(&m_r4300->interp_PC.addr));
    MOV(32, MatR(RSCRATCH), Imm32(address));
    MOV(32, HOTSTATE_VAR(pc), Imm32(address));

    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionP(InterpretOpcode, m_r4300);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

    if (op) {
        m_gpr.Reset(op->regsOut);
        m_fpr.Reset(op->fregsOut | op->fregsOut32);
    }

    if (HOT_STATE->inDelaySlot) {
        MOV(32, HOTSTATE_VAR(delay_slot), Imm32(0));
    }

    MOV(64, R(RSCRATCH), ImmPtr(&m_r4300->interp_PC.addr));
    CMP(32, MatR(RSCRATCH), Imm32(address + 4));
    FixupBranch pc_as_expected = J_CC(CC_E);

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        compile_goto_dispatcher_destinhotstate(NULL, false);
    }

    SetJumpTarget(pc_as_expected);
    MOV(32, HOTSTATE_VAR(pc), Imm32(address + 4));
}

static void vr4300_jitter_tlb_write(unsigned int idx)
{
    VR4300_Jitter::GetInstance()->TLBWrite(idx);
}

void VR4300_Jitter::TLBWrite(unsigned int idx)
{
    struct r4300_core *r4300 = m_r4300;

    uint32_t* cp0_regs = r4300_cp0_regs(&r4300->cp0);

    if (HOT_STATE->pc >= r4300->cp0.tlb.entries[idx].start_even && HOT_STATE->pc < r4300->cp0.tlb.entries[idx].end_even && r4300->cp0.tlb.entries[idx].v_even)
        return;
    if (HOT_STATE->pc >= r4300->cp0.tlb.entries[idx].start_odd && HOT_STATE->pc < r4300->cp0.tlb.entries[idx].end_odd && r4300->cp0.tlb.entries[idx].v_odd)
        return;

    if (r4300->cp0.tlb.entries[idx].v_even) {
        for (unsigned int i = r4300->cp0.tlb.entries[idx].start_even>>12; i <= r4300->cp0.tlb.entries[idx].end_even>>12; i++) {
            u32 virtual_address = i << 12;

            ValidBlockUnsetVirtual(virtual_address);

#if MEMMAP_TLB_REGIONS
            if (r4300->cp0.tlb.LUT_r[i]) {
                Common::ReadProtectMemory(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000);
            }
#endif
        }
    }

    if (r4300->cp0.tlb.entries[idx].v_odd) {
        for (unsigned int i = r4300->cp0.tlb.entries[idx].start_odd>>12; i <= r4300->cp0.tlb.entries[idx].end_odd>>12; i++) {
            u32 virtual_address = i << 12;

            ValidBlockUnsetVirtual(virtual_address);

#if MEMMAP_TLB_REGIONS
            if (r4300->cp0.tlb.LUT_r[i]) {
                Common::ReadProtectMemory(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000);
            }
#endif
        }
    }

    tlb_unmap(&r4300->cp0.tlb, idx);

    r4300->cp0.tlb.entries[idx].g = (cp0_regs[CP0_ENTRYLO0_REG] & cp0_regs[CP0_ENTRYLO1_REG] & 1);
    r4300->cp0.tlb.entries[idx].pfn_even = (cp0_regs[CP0_ENTRYLO0_REG] & UINT32_C(0x3FFFFFC0)) >> 6;
    r4300->cp0.tlb.entries[idx].pfn_odd = (cp0_regs[CP0_ENTRYLO1_REG] & UINT32_C(0x3FFFFFC0)) >> 6;
    r4300->cp0.tlb.entries[idx].c_even = (cp0_regs[CP0_ENTRYLO0_REG] & UINT32_C(0x38)) >> 3;
    r4300->cp0.tlb.entries[idx].c_odd = (cp0_regs[CP0_ENTRYLO1_REG] & UINT32_C(0x38)) >> 3;
    r4300->cp0.tlb.entries[idx].d_even = (cp0_regs[CP0_ENTRYLO0_REG] & UINT32_C(0x4)) >> 2;
    r4300->cp0.tlb.entries[idx].d_odd = (cp0_regs[CP0_ENTRYLO1_REG] & UINT32_C(0x4)) >> 2;
    r4300->cp0.tlb.entries[idx].v_even = (cp0_regs[CP0_ENTRYLO0_REG] & UINT32_C(0x2)) >> 1;
    r4300->cp0.tlb.entries[idx].v_odd = (cp0_regs[CP0_ENTRYLO1_REG] & UINT32_C(0x2)) >> 1;
    r4300->cp0.tlb.entries[idx].asid = (cp0_regs[CP0_ENTRYHI_REG] & UINT32_C(0xFF));
    r4300->cp0.tlb.entries[idx].vpn2 = (cp0_regs[CP0_ENTRYHI_REG] & UINT32_C(0xFFFFE000)) >> 13;
    //r4300->cp0.tlb.entries[idx].r = (cp0_regs[CP0_ENTRYHI_REG] & 0xC000000000000000LL) >> 62;
    r4300->cp0.tlb.entries[idx].mask = (cp0_regs[CP0_PAGEMASK_REG] & UINT32_C(0x1FFE000)) >> 13;

    r4300->cp0.tlb.entries[idx].start_even = r4300->cp0.tlb.entries[idx].vpn2 << 13;
    r4300->cp0.tlb.entries[idx].end_even = r4300->cp0.tlb.entries[idx].start_even+
        (r4300->cp0.tlb.entries[idx].mask << 12) + UINT32_C(0xFFF);
    r4300->cp0.tlb.entries[idx].phys_even = r4300->cp0.tlb.entries[idx].pfn_even << 12;


    r4300->cp0.tlb.entries[idx].start_odd = r4300->cp0.tlb.entries[idx].end_even+1;
    r4300->cp0.tlb.entries[idx].end_odd = r4300->cp0.tlb.entries[idx].start_odd+
        (r4300->cp0.tlb.entries[idx].mask << 12) + UINT32_C(0xFFF);
    r4300->cp0.tlb.entries[idx].phys_odd = r4300->cp0.tlb.entries[idx].pfn_odd << 12;

    tlb_map(&r4300->cp0.tlb, idx);

     if (r4300->cp0.tlb.entries[idx].v_even) {
         for (unsigned int i = r4300->cp0.tlb.entries[idx].start_even>>12; i <= r4300->cp0.tlb.entries[idx].end_even>>12; i++) {
            u32 virtual_address = i << 12;
            u32 physical_address = vr4300_jitter_translate_address_no_exception(virtual_address, 2) & MEMORY_MASK;

            if (m_tlb_mappings[i] != physical_address) {
                if (m_tlb_mappings[i]) m_block_cache.EraseVirtualRange(virtual_address, m_tlb_mappings[i], 0x1000);

#if !DISABLE_FASTMEM
#if MEMMAP_TLB_REGIONS
                Common::UnWriteProtectMemory(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000, false);
                m_arena.UnmapFromMemoryRegion(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000);

                if (r4300->cp0.tlb.LUT_r[i]) {
                    if (m_arena.MapInMemoryRegion(m_rdram_position + physical_address, 0x1000, m_physical_base + MM_RDRAM_DRAM + virtual_address) != m_physical_base + MM_RDRAM_DRAM + virtual_address) {
                        abort();
                    }
                }
#endif
#endif
            } else {
                ValidBlockSetVirtual(virtual_address);
#if !DISABLE_FASTMEM
#if MEMMAP_TLB_REGIONS
                Common::UnWriteProtectMemory(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000, false);
#endif
#endif
            }

            m_tlb_mappings[i] = physical_address;
        }
     }

    if (r4300->cp0.tlb.entries[idx].v_odd) {
        for (unsigned int i = r4300->cp0.tlb.entries[idx].start_odd>>12; i <= r4300->cp0.tlb.entries[idx].end_odd>>12; i++) {
            u32 virtual_address = i << 12;
            u32 physical_address = vr4300_jitter_translate_address_no_exception(virtual_address, 2) & MEMORY_MASK;

            if (m_tlb_mappings[i] != physical_address) {
                if (m_tlb_mappings[i]) m_block_cache.EraseVirtualRange(virtual_address, m_tlb_mappings[i], 0x1000);

#if !DISABLE_FASTMEM
#if MEMMAP_TLB_REGIONS
                Common::UnWriteProtectMemory(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000, false);
                m_arena.UnmapFromMemoryRegion(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000);

                if (r4300->cp0.tlb.LUT_r[i]) {
                    if (m_arena.MapInMemoryRegion(m_rdram_position + physical_address, 0x1000, m_physical_base + MM_RDRAM_DRAM + virtual_address) != m_physical_base + MM_RDRAM_DRAM + virtual_address) {
                        abort();
                    }
                }
#endif
#endif
            } else {
                ValidBlockSetVirtual(virtual_address);
#if !DISABLE_FASTMEM
#if MEMMAP_TLB_REGIONS
                Common::UnWriteProtectMemory(m_physical_base + MM_RDRAM_DRAM + virtual_address, 0x1000, false);
#endif
#endif
            }

            m_tlb_mappings[i] = physical_address;
        }
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

void VR4300_Jitter::recompile_TEQ(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, s);
    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt, Rs);

        if (Rt.IsSimpleReg()) {
            if (!Rs.IsImm()) {
                CMP(64, Rt, Rs);
            } else {
                MOV(64, R(RSCRATCH), Rs);
                CMP(64, Rt, R(RSCRATCH));
            }
        } else if (Rs.IsSimpleReg()) {
            if (!Rt.IsImm()) {
                CMP(64, Rs, Rt);
            } else {
                MOV(64, R(RSCRATCH), Rt);
                CMP(64, Rs, R(RSCRATCH));
            }
        }
    }
    FixupBranch neq = J_CC(CC_NE, XEmitter::Jump::Near);
    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_TR));
    compile_exception_general(op);
    SetJumpTarget(neq);
}

void VR4300_Jitter::recompile_RESERVED_COP2(struct jit_instr *op)
{
    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_RI | CP0_CAUSE_CE2));

    compile_exception_general(op);
}

void VR4300_Jitter::recompile_RESERVED(struct jit_instr *op)
{
    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_RI));

    compile_exception_general(op);
}

void VR4300_Jitter::recompile_TLBWI(struct jit_instr *op)
{
    MOV(32, HOTSTATE_VAR(pc), Imm32(op->address));

    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(64, R(ABI_PARAM1), (HOTSTATE_CP0REG(CP0_INDEX_REG)));
    AND(32, R(ABI_PARAM1), Imm32(UINT32_C(0x3F)));
    ABI_CallFunction(vr4300_jitter_tlb_write);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
}

void VR4300_Jitter::update_random_reg()
{
    RCX64Reg edx = m_gpr.Scratch(EDX);
    RCX64Reg eax = m_gpr.Scratch(EAX);
    RCX64Reg scratch = m_gpr.Scratch();
    RegCache::Realize(edx, eax, scratch);

    MOV(32, R(edx), Imm32(0));
    MOV(32, R(eax), (HOTSTATE_CP0REG(CP0_COUNT_REG)));

    MOV(32, R(scratch), Imm32(m_r4300->cp0.count_per_op));
    DIV(32, R(scratch));

    MOV(32, R(edx), Imm32(0));

    MOV(32, R(scratch), Imm32(32));
    SUB(32, R(scratch), (HOTSTATE_CP0REG(CP0_WIRED_REG)));

    DIV(32, R(scratch));

    ADD(32, R(edx), (HOTSTATE_CP0REG(CP0_WIRED_REG)));
    MOV(32, HOTSTATE_CP0REG(CP0_RANDOM_REG), R(edx));
}

void VR4300_Jitter::recompile_TLBWR(struct jit_instr *op)
{
    MOV(32, HOTSTATE_VAR(pc), Imm32(op->address));

    update_count_reg();

    // in TLBWR mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

    MOV(32, R(RSCRATCH), Imm32(HOT_STATE->compiler_cycles));
    SUB(32, R(RSCRATCH), HOTSTATE_VAR(cycles_added));
    ADD(32, HOTSTATE_CP0REG(CP0_COUNT_REG), R(RSCRATCH));
    ADD(32, HOTSTATE_VAR(cycle_count), Imm32(m_r4300->cp0.count_per_op));
    ADD(32, HOTSTATE_VAR(cycles_added), Imm32(m_r4300->cp0.count_per_op));

    update_random_reg();

    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(64, R(ABI_PARAM1), (HOTSTATE_CP0REG(CP0_RANDOM_REG)));
    ABI_CallFunction(vr4300_jitter_tlb_write);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
}

void tlb_tlbp(struct r4300_core *r4300)
{
    uint32_t* cp0_regs = r4300_cp0_regs(&r4300->cp0);

    int i;

    cp0_regs[CP0_INDEX_REG] |= UINT32_C(0x80000000);
    for (i = 0; i < 32; ++i)
    {
        if (((r4300->cp0.tlb.entries[i].vpn2 & (~r4300->cp0.tlb.entries[i].mask)) ==
                    (((cp0_regs[CP0_ENTRYHI_REG] & UINT32_C(0xFFFFE000)) >> 13) & (~r4300->cp0.tlb.entries[i].mask))) &&
                ((r4300->cp0.tlb.entries[i].g) ||
                 (r4300->cp0.tlb.entries[i].asid == (cp0_regs[CP0_ENTRYHI_REG] & UINT32_C(0xFF)))))
        {
            cp0_regs[CP0_INDEX_REG] = i;
            break;
        }
    }
}

void tlb_tlbr(struct r4300_core *r4300)
{
    uint32_t* cp0_regs = r4300_cp0_regs(&r4300->cp0);

    int index;
    index = cp0_regs[CP0_INDEX_REG] & UINT32_C(0x1F);
    cp0_regs[CP0_PAGEMASK_REG] = r4300->cp0.tlb.entries[index].mask << 13;
    cp0_regs[CP0_ENTRYHI_REG] = ((r4300->cp0.tlb.entries[index].vpn2 << 13) | r4300->cp0.tlb.entries[index].asid);
    cp0_regs[CP0_ENTRYLO0_REG] = (r4300->cp0.tlb.entries[index].pfn_even << 6) | (r4300->cp0.tlb.entries[index].c_even << 3)
        | (r4300->cp0.tlb.entries[index].d_even << 2) | (r4300->cp0.tlb.entries[index].v_even << 1)
        | r4300->cp0.tlb.entries[index].g;
    cp0_regs[CP0_ENTRYLO1_REG] = (r4300->cp0.tlb.entries[index].pfn_odd << 6) | (r4300->cp0.tlb.entries[index].c_odd << 3)
        | (r4300->cp0.tlb.entries[index].d_odd << 2) | (r4300->cp0.tlb.entries[index].v_odd << 1)
        | r4300->cp0.tlb.entries[index].g;
}

void VR4300_Jitter::recompile_TLBP(struct jit_instr *op)
{
    // TODO: asm version?
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionP(tlb_tlbp, &g_dev.r4300);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
}

void VR4300_Jitter::recompile_TLBR(struct jit_instr *op)
{
    // TODO: asm version?
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionP(tlb_tlbr, &g_dev.r4300);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
}

void VR4300_Jitter::compile_tlb_exception_check(struct jit_instr *op, bool update_cycles, uint32_t expected_pc)
{
    if (!is_in_far_code()) {
        FixupBranch exc = check_pc_differs(expected_pc);

        switch_to_far_code();
        SetJumpTarget(exc);

        if (update_cycles) {
            update_cycle_count(op, true);
        }

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            m_gpr.Revert();
            m_fpr.Revert();

            compile_goto_dispatcher_destinhotstate(op, false);
        }

        FixupBranch near_code = J(XEmitter::Jump::Near);
        switch_to_near_code();
        SetJumpTarget(near_code);
    } else {
        FixupBranch no_exc = check_pc_equals(expected_pc);

        if (update_cycles) {
            update_cycle_count(op, true);
        }

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            m_gpr.Revert();
            m_fpr.Revert();

            compile_goto_dispatcher_destinhotstate(op, false);
        }

        SetJumpTarget(no_exc);
    }
}

void VR4300_Jitter::recompile_ERET(struct jit_instr *op)
{
    update_hot_cycles(op, false);
    update_count_reg();

    // in ERET mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

    AND(32, HOTSTATE_CP0REG(CP0_STATUS_REG), Imm32(~CP0_STATUS_EXL));

    MOV(32, R(RSCRATCH), (HOTSTATE_CP0REG(CP0_EPC_REG)));
    MOV(32, HOTSTATE_VAR(pc), R(RSCRATCH));

    MOV(32, HOTSTATE_VAR(llbit), Imm32(0));

    MOV(64, R(RSCRATCH2), ImmPtr(&g_dev.r4300.mi->regs[MI_INTR_REG]));
    MOV(32, R(RSCRATCH), MatR(RSCRATCH2));
    MOV(64, R(RSCRATCH2), ImmPtr(&g_dev.r4300.mi->regs[MI_INTR_MASK_REG]));
    AND(32, R(RSCRATCH), MatR(RSCRATCH2));

    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    MOV(64, R(ABI_PARAM1), Imm64((u64)m_r4300));
    MOV(32, R(ABI_PARAM2), Imm32(CP0_CAUSE_IP2));
    if (ABI_PARAM3 != RSCRATCH) {
        MOV(32, R(ABI_PARAM3), R(RSCRATCH));
    }
    ABI_CallFunction(r4300_check_interrupt);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

    update_hot_cycles(op, false);

    RCForkGuard gpr_guard = m_gpr.Fork();
    RCForkGuard fpr_guard = m_fpr.Fork();

    MOV(32, R(RSCRATCH), Imm32(HOT_STATE->compiler_cycles));
    SUB(32, R(RSCRATCH), HOTSTATE_VAR(cycles_added));
    ADD(32, HOTSTATE_VAR(cycle_count), R(RSCRATCH));
    MOV(32, HOTSTATE_VAR(cycles_added), Imm32(-HOT_STATE->compiler_cycles - g_dev.r4300.cp0.count_per_op));

    update_count_reg();
    if (HOT_STATE->inDelaySlot) {
        do_core_compare(op, op->address);
    }

    compile_check_cycle_count(op, 0);

    compile_goto_dispatcher_destinhotstate(op, false, true);
}

void VR4300_Jitter::recompile_MFC0(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    assert(op->has_d);

    if (op->t) {
        RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
        RegCache::Realize(Rt);

        switch(op->d) {
            case CP0_RANDOM_REG: {
                    update_count_reg();
                    // in MFC0 mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
                    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

                    update_random_reg();

                    MOVSX(64, 32, Rt, (HOTSTATE_CP0REG(op->d)));
                }
                break;
            case CP0_COUNT_REG:
                {
                    update_count_reg();
                    // in MFC0 mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
                    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

                    MOVSX(64, 32, Rt, (HOTSTATE_CP0REG(op->d)));
                }
                break;
            case CP0_UNUSED_7:
            case CP0_UNUSED_21:
            case CP0_UNUSED_22:
            case CP0_UNUSED_23:
            case CP0_UNUSED_24:
            case CP0_UNUSED_25:
            case CP0_UNUSED_31:
                MOV(32, Rt, HOTSTATE_VAR(cp0_latch));
                break;
            default:
                MOVSX(64, 32, Rt, (HOTSTATE_CP0REG(op->d)));
        };
    }
}

void VR4300_Jitter::recompile_DMFC0(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    assert(op->has_d);

    if (op->t) {
        RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
        RegCache::Realize(Rt);

        switch(op->d) {
            case CP0_RANDOM_REG: {
                    update_count_reg();
                    // in DMFC0 mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
                    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);
                    update_random_reg();

                    MOV(32, Rt, (HOTSTATE_CP0REG(op->d)));
                }
                break;
            case CP0_COUNT_REG:
                {
                    update_count_reg();
                    // in DMFC0 mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
                    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

                    MOV(32, Rt, (HOTSTATE_CP0REG(op->d)));
                }
                break;
            case CP0_EPC_REG:
                {
                    MOVSX(64, 32, Rt, (HOTSTATE_CP0REG(op->d)));
                }
                break;
            case CP0_UNUSED_7:
            case CP0_UNUSED_21:
            case CP0_UNUSED_22:
            case CP0_UNUSED_23:
            case CP0_UNUSED_24:
            case CP0_UNUSED_25:
            case CP0_UNUSED_31:
                MOV(32, Rt, HOTSTATE_VAR(cp0_latch));
                break;
            default:
                MOV(32, Rt, (HOTSTATE_CP0REG(op->d)));
        };
    }
}

void VR4300_Jitter::mtc0_helper(int t, int d, unsigned int mask)
{
    if (!t || m_gpr.IsImm(t)) {
        MOV(32, HOTSTATE_CP0REG(d), Imm32(t ? (m_gpr.Imm64(t) & mask) : 0));
    } else {
        RCOpArg Rt = m_gpr.Use(t, RCMode::Read);
        RegCache::Realize(Rt);

        MOV(32, R(RSCRATCH), Rt);
        if (mask != 0xFFFFFFFF) AND(32, R(RSCRATCH), Imm32(mask));
        MOV(32, HOTSTATE_CP0REG(d), R(RSCRATCH));
    }
}

void *vr4300_jitter_get_interrupt_first_data_count_ptr(void)
{
    return &g_dev.r4300.cp0.q.first->data.count;
}

void vr4300_jitter_remove_event(int event)
{
    remove_event(&g_dev.r4300.cp0.q, event);
}

void VR4300_Jitter::recompile_MTC0(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    assert(op->has_d);

    //DebugMessage(M64MSG_VERBOSE, "MTC0 t: %d d: %d\n", op->t, op->d);

    {
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rt);

        if (Rt.IsSimpleReg()) {
            MOV(32, HOTSTATE_VAR(cp0_latch), Rt);
        } else if (Rt.IsImm()) {
            MOV(32, HOTSTATE_VAR(cp0_latch), Imm32(Rt.Imm64()));
        } else {
            MOV(32, R(RSCRATCH), Rt);
            MOV(32, HOTSTATE_VAR(cp0_latch), R(RSCRATCH));
        }
    }

    switch(op->d) {
        case CP0_INDEX_REG:
            mtc0_helper(op->t, op->d, 0x8000003F);
            break;
        case CP0_RANDOM_REG:
            break;
        case CP0_ENTRYLO0_REG:
            mtc0_helper(op->t, op->d, 0x3FFFFFFF);
            break;
        case CP0_ENTRYLO1_REG:
            mtc0_helper(op->t, op->d, 0x3FFFFFFF);
            break;
        case CP0_CONTEXT_REG: {
                RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                RegCache::Realize(Rt);

                MOV(64, R(RSCRATCH), Rt);
                AND(32, R(RSCRATCH), Imm32(UINT32_C(0xFF800000)));
                MOV(32, R(RSCRATCH2), (HOTSTATE_CP0REG(CP0_CONTEXT_REG)));
                AND(32, R(RSCRATCH2), Imm32(UINT32_C(0x007FFFF0)));
                OR(32, R(RSCRATCH), R(RSCRATCH2));
                MOV(32, HOTSTATE_CP0REG(CP0_CONTEXT_REG), R(RSCRATCH));
            }
            break;
        case CP0_PAGEMASK_REG: {
                mtc0_helper(op->t, op->d, 0x01FFE000);
            }
            break;
        case CP0_WIRED_REG: {
                    mtc0_helper(op->t, op->d, 0x0000003F);
                    MOV(32, HOTSTATE_CP0REG(CP0_RANDOM_REG), Imm32(31));
                }
                break;
            break;
        case CP0_BADVADDR_REG:
            break;
        case CP0_COUNT_REG: {
                update_cycle_count(op, false);
                //update_count_reg();
                // in MTC0 mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
                if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

#if MARK_INTERRUPT_UNSAFE_STATE
                OR(32, M(&g_dev.r4300.cp0.interrupt_unsafe_state), Imm32(INTR_UNSAFE_R4300));
#endif

                CMP_or_TEST(32, HOTSTATE_VAR(cycle_count), Imm32(0));
                FixupBranch no_gen_int = J_CC(CC_L, XEmitter::Jump::Near);

                FixupBranch far_code = J(XEmitter::Jump::Near);
                switch_to_far_code();
                SetJumpTarget(far_code);

                MOV(32, HOTSTATE_VAR(pc), Imm32(op->address));

                zero_hot_cycles();
                BitSet32 registers_in_use = caller_saved_registers_in_use();
                ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                ABI_CallFunctionP(gen_interrupt, &g_dev.r4300);
                ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

                FixupBranch near_code = J(XEmitter::Jump::Near);
                switch_to_near_code();
                SetJumpTarget(near_code);

                SetJumpTarget(no_gen_int);
#if MARK_INTERRUPT_UNSAFE_STATE
                AND(32, M(&g_dev.r4300.cp0.interrupt_unsafe_state), Imm32(~INTR_UNSAFE_R4300));
#endif

                {
                    RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                    RegCache::Realize(Rt);

                    registers_in_use = caller_saved_registers_in_use();
                    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                    mov_2(64, ABI_PARAM1, 64, ABI_PARAM2, RCOpArg::Imm64((u64)&m_r4300->cp0), Rt);
                    ABI_CallFunction(translate_event_queue);
                    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
                }

                // ???
                compile_tlb_exception_check(op, false, op->address - 4);

                // TODO?
                ADD(32, HOTSTATE_VAR(cycle_count), Imm32(m_r4300->cp0.count_per_op));
                ADD(32, HOTSTATE_VAR(cycles_added), Imm32(2 * m_r4300->cp0.count_per_op));
            }
            break;
        case CP0_ENTRYHI_REG:
            mtc0_helper(op->t, op->d, 0xFFFFE0FF);
            break;
        case CP0_COMPARE_REG: {
                update_count_reg();
                if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

                BitSet32 registers_in_use = caller_saved_registers_in_use();
                ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                ABI_CallFunctionC(vr4300_jitter_remove_event, COMPARE_INT);
                ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

                ADD(32, HOTSTATE_CP0REG(CP0_COUNT_REG), Imm32(m_r4300->cp0.count_per_op));
                ADD(32, HOTSTATE_VAR(cycle_count), Imm32(m_r4300->cp0.count_per_op));

                RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                RegCache::Realize(Rt);

                registers_in_use = caller_saved_registers_in_use();
                ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                mov_3(64, ABI_PARAM1, ABI_PARAM2, ABI_PARAM3, RCOpArg::Imm64((u64)&m_r4300->cp0), RCOpArg::Imm64(COMPARE_INT), Rt);
                ABI_CallFunction(add_interrupt_event_count);
                ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

                SUB(32, HOTSTATE_CP0REG(CP0_COUNT_REG), Imm32(m_r4300->cp0.count_per_op));


                ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                ABI_CallFunction(vr4300_jitter_get_interrupt_first_data_count_ptr);
                MOV(64, R(RSCRATCH2), R(ABI_RETURN));
                ABI_PopRegistersAndAdjustStack(registers_in_use, 0);


                MOV(32, R(RSCRATCH), (HOTSTATE_CP0REG(CP0_COUNT_REG)));

                SUB(32, R(RSCRATCH), MatR(RSCRATCH2));
                MOV(32, HOTSTATE_VAR(cycle_count), R(RSCRATCH));

                if (Rt.IsSimpleReg()) {
                    MOV(32, HOTSTATE_CP0REG(CP0_COMPARE_REG), Rt);
                } else if (Rt.IsImm()) {
                    MOV(32, HOTSTATE_CP0REG(CP0_COMPARE_REG), Imm32(Rt.Imm64()));
                } else {
                    MOV(32, R(RSCRATCH), Rt);
                    MOV(32, HOTSTATE_CP0REG(CP0_COMPARE_REG), R(RSCRATCH));
                }
                AND(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(~CP0_CAUSE_IP7));

                MOV(32, R(RSCRATCH), Imm32(HOT_STATE->compiler_cycles));
                SUB(32, R(RSCRATCH), HOTSTATE_VAR(cycles_added));
                ADD(32, HOTSTATE_CP0REG(CP0_COUNT_REG), R(RSCRATCH));
            }
            break;
        case CP0_STATUS_REG: {
                {
                    BitSet32 registers_in_use;

                    RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                    RegCache::Realize(Rt);

                    MOV(64, R(RSCRATCH2), Rt);
                    AND(32, R(RSCRATCH2), Imm32(CP0_STATUS_FR));

                    MOV(32, R(RSCRATCH), (HOTSTATE_CP0REG(CP0_STATUS_REG)));
                    AND(32, R(RSCRATCH), Imm32(CP0_STATUS_FR));
                    CMP(32, R(RSCRATCH2), R(RSCRATCH));

                    FixupBranch dont_change_fr_ptrs = J_CC(CC_E, XEmitter::Jump::Near);

                    registers_in_use = caller_saved_registers_in_use();
                    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                    mov_2(64, ABI_PARAM1, 64, ABI_PARAM2, RCOpArg::Imm64((u64)&m_r4300->cp1), Rt);
                    ABI_CallFunction(set_fpr_pointers);
                    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

                    SetJumpTarget(dont_change_fr_ptrs);

                    MOV(64, R(RSCRATCH2), Rt);

                    AND(32, R(RSCRATCH2), Imm32(~UINT32_C(0x080000)));
                    MOV(32, HOTSTATE_CP0REG(CP0_STATUS_REG), R(RSCRATCH2));

                    SUB(32, HOTSTATE_VAR(cycle_count), Imm32(m_r4300->cp0.count_per_op));
                    HOT_STATE->compiler_cycles += g_dev.r4300.cp0.count_per_op;
                    ADD(32, HOTSTATE_VAR(cycles_added), Imm32(m_r4300->cp0.count_per_op));


                    update_hot_cycles(op, false);
                    update_count_reg();
                    // in MTC0 mips_instructions.def calls cp0_update_count which calls CoreCompareCallback:
                    if (HOT_STATE->inDelaySlot) do_core_compare(op, op->address);

                    MOV(32, HOTSTATE_VAR(pc), Imm32(op->address + 4));

                    MOV(64, R(RSCRATCH), ImmPtr(&g_dev.r4300.mi->regs[MI_INTR_REG]));
                    MOV(32, R(RSCRATCH2), MatR(RSCRATCH));
                    MOV(64, R(RSCRATCH), ImmPtr(&g_dev.r4300.mi->regs[MI_INTR_MASK_REG]));
                    AND(32, R(RSCRATCH2), MatR(RSCRATCH));
                    registers_in_use = caller_saved_registers_in_use();
                    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
                    mov_3(64, ABI_PARAM1, ABI_PARAM2, ABI_PARAM3, RCOpArg::Imm64((u64)m_r4300), RCOpArg::Imm64(CP0_CAUSE_IP2), RCOpArg::R(RSCRATCH2));
                    ABI_CallFunction(r4300_check_interrupt);
                    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
                }

                RCForkGuard gpr_guard = m_gpr.Fork();
                RCForkGuard fpr_guard = m_fpr.Fork();

                compile_cycle_count_checks(op, 0, !HOT_STATE->inDelaySlot, false, 0);

                compile_goto_dispatcher(op, op->address + 4, false);
            }
            break;
        case CP0_CAUSE_REG: {
                RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                RegCache::Realize(Rt);

                AND(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(~(CP0_CAUSE_IP0 | CP0_CAUSE_IP1)));
                if (Rt.IsImm()) {
                    OR(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(Rt.Imm64() & (CP0_CAUSE_IP0 | CP0_CAUSE_IP1)));
                } else {
                    MOV(32, R(RSCRATCH), Rt);
                    AND(32, R(RSCRATCH), Imm32(CP0_CAUSE_IP0 | CP0_CAUSE_IP1));

                    OR(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), R(RSCRATCH));
                }
            }
            break;
        case CP0_EPC_REG:
            mtc0_helper(op->t, op->d, 0xFFFFFFFF);
            break;
        case CP0_PREVID_REG:
            break;
        case CP0_CONFIG_REG: {
                RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                RegCache::Realize(Rt);

                MOV(32, R(RSCRATCH), Rt);
                AND(32, R(RSCRATCH), Imm32(0x0000000F));
                MOV(32, R(RSCRATCH2), (HOTSTATE_CP0REG(CP0_CONFIG_REG)));
                AND(32, R(RSCRATCH2), Imm32(0x00008000));
                OR(32, R(RSCRATCH), R(RSCRATCH2));
                MOV(32, R(RSCRATCH2), (HOTSTATE_CP0REG(CP0_CONFIG_REG)));
                AND(32, R(RSCRATCH2), Imm32(0x7FFFFFFF));
                OR(32, R(RSCRATCH), R(RSCRATCH2));
                MOV(32, HOTSTATE_CP0REG(CP0_CONFIG_REG), R(RSCRATCH));
            }
            break;
        case CP0_LLADDR_REG:
            mtc0_helper(op->t, op->d, 0xFFFFFFFF);
            break;
        case CP0_WATCHLO_REG:
            mtc0_helper(op->t, op->d, 0xFFFFFFFF);
            break;
        case CP0_WATCHHI_REG:
            mtc0_helper(op->t, op->d, 0xFFFFFFFF);
            break;
        case CP0_XCONTEXT_REG:
            break;
        case CP0_CACHEERR_REG:
            break;
        case CP0_PARITYERR_REG:
            mtc0_helper(op->t, op->d, 0x000000FF);
            break;
        case CP0_TAGLO_REG:
            mtc0_helper(op->t, op->d, 0x0FFFFFC0);
            break;
        case CP0_TAGHI_REG:
            MOV(32, HOTSTATE_CP0REG(CP0_TAGHI_REG), Imm32(0));
            break;
        case CP0_ERROREPC_REG:
            mtc0_helper(op->t, op->d, 0x0FFFFFFF);
            break;
        default:
            break;
    }
}

void VR4300_Jitter::recompile_DMTC0(struct jit_instr *op)
{
    recompile_MTC0(op);
}

void VR4300_Jitter::compile_check_cycle_count(struct jit_instr *op, uint32_t in_pc)
{
    CMP_or_TEST(32, HOTSTATE_VAR(cycle_count), Imm32(0));
    FixupBranch interrupt = J_CC(CC_GE, XEmitter::Jump::Near);

    switch_to_far_code();
    SetJumpTarget(interrupt);

    if (in_pc) {
        MOV(32, HOTSTATE_VAR(pc), Imm32(in_pc));
    } else {
        MOV(32, R(RSCRATCH), HOTSTATE_VAR(pc));
        MOV(32, HOTSTATE_VAR(stored_pc), R(RSCRATCH));
    }

    zero_hot_cycles();
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
#if MARK_INTERRUPT_UNSAFE_STATE
    OR(32, M(&m_r4300->cp0.interrupt_unsafe_state), Imm32(INTR_UNSAFE_R4300));
#endif
    ABI_CallFunctionP(gen_interrupt, m_r4300);
#if MARK_INTERRUPT_UNSAFE_STATE
    AND(32, M(&m_r4300->cp0.interrupt_unsafe_state), Imm32(~INTR_UNSAFE_R4300));
#endif
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

    if (in_pc) {
        FixupBranch no_exc = check_pc_equals(in_pc);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            compile_goto_dispatcher_destinhotstate(op, false);
        }

        SetJumpTarget(no_exc);
    } else {
        MOV(32, R(RSCRATCH), HOTSTATE_VAR(stored_pc));
        CMP(32, HOTSTATE_VAR(pc), R(RSCRATCH));
        FixupBranch no_exc = J_CC(CC_E, XEmitter::Jump::Near);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            compile_goto_dispatcher_destinhotstate(op, false);

        }

        SetJumpTarget(no_exc);
    }

    FixupBranch near_code = J(XEmitter::Jump::Near);
    switch_to_near_code();
    SetJumpTarget(near_code);
}

void VR4300_Jitter::compile_cycle_count_checks(struct jit_instr *op, uint32_t pc, bool no_compare, bool no_add, u32 compare_pc)
{
    update_cycle_count(op, no_add);

    if (!no_compare) {
        do_core_compare(op, compare_pc);
    }

    compile_check_cycle_count(op, pc);
}

void VR4300_Jitter::recompile_SYNC(struct jit_instr *op)
{
}

void VR4300_Jitter::recompile_BREAK(struct jit_instr *op)
{
    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_BP));
    compile_exception_general(op);
}

void VR4300_Jitter::recompile_CTC2(struct jit_instr *op)
{
    RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
    RegCache::Realize(Rt);
    if (!Rt.IsSimpleReg() && !Rt.IsImm()) {
        MOV(64, R(RSCRATCH), Rt);
        MOV(64, HOTSTATE_VAR(cp2_latch), R(RSCRATCH));
    } else {
        MOV(64, HOTSTATE_VAR(cp2_latch), Rt);
    }
}

void VR4300_Jitter::recompile_MTC2(struct jit_instr *op)
{
    RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
    RegCache::Realize(Rt);
    if (!Rt.IsSimpleReg() && !Rt.IsImm()) {
        MOV(64, R(RSCRATCH), Rt);
        MOV(64, HOTSTATE_VAR(cp2_latch), R(RSCRATCH));
    } else {
        MOV(64, HOTSTATE_VAR(cp2_latch), Rt);
    }
}

void VR4300_Jitter::recompile_DMTC2(struct jit_instr *op)
{
    RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
    RegCache::Realize(Rt);
    if (!Rt.IsSimpleReg() && !Rt.IsImm()) {
        MOV(64, R(RSCRATCH), Rt);
        MOV(64, HOTSTATE_VAR(cp2_latch), R(RSCRATCH));
    } else {
        MOV(64, HOTSTATE_VAR(cp2_latch), Rt);
    }
}

void VR4300_Jitter::recompile_CFC2(struct jit_instr *op)
{
    if (op->t) {
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Write);
        RegCache::Realize(Rt);
        if (!Rt.IsSimpleReg()) {
            MOV(32, R(RSCRATCH), HOTSTATE_VAR(cp2_latch));
            MOV(64, Rt, R(RSCRATCH));
        } else {
            MOV(32, Rt, HOTSTATE_VAR(cp2_latch));
        }
    }
}

void VR4300_Jitter::recompile_MFC2(struct jit_instr *op)
{
    if (op->t) {
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Write);
        RegCache::Realize(Rt);
        if (!Rt.IsSimpleReg()) {
            MOV(32, R(RSCRATCH), HOTSTATE_VAR(cp2_latch));
            MOV(64, Rt, R(RSCRATCH));
        } else {
            MOV(32, Rt, HOTSTATE_VAR(cp2_latch));
        }
    }
}

void VR4300_Jitter::recompile_DMFC2(struct jit_instr *op)
{
    if (op->t) {
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Write);
        RegCache::Realize(Rt);
        if (!Rt.IsSimpleReg()) {
            MOV(64, R(RSCRATCH), HOTSTATE_VAR(cp2_latch));
            MOV(64, Rt, R(RSCRATCH));
        } else {
            MOV(64, Rt, HOTSTATE_VAR(cp2_latch));
        }
    }
}

void VR4300_Jitter::recompile_SYSCALL(struct jit_instr *op)
{
    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_SYS));
    compile_exception_general(op);
}

#define DEBUG_BLOCK 0
#define DEBUG_BLOCK_ADDRESS 0x80000180

void VR4300_Jitter::embed_valid_block_check(u32 address, bool force_check, bool delay_slot, bool update_cc)
{
    if (vr4300_jitter_address_needs_translation(address) && (((address & 0xFFF) == 0) || force_check)) {
        // Check if the block is still valid.
        MOV(64, R(RSCRATCH), ImmPtr(m_valid_virtual_block));
        TEST(8, MDisp(RSCRATCH, address >> 12), Imm8(1));

        FixupBranch invalid_block = J_CC(CC_Z, XEmitter::Jump::Near);
        switch_to_far_code();
        SetJumpTarget(invalid_block);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            m_gpr.Flush();
            m_fpr.Flush();

            if (update_cc) {
                update_cycle_count(NULL, true);
            }

            if (delay_slot) {
                MOV(32, HOTSTATE_VAR(delay_slot), Imm32(1));
            }
            MOV(32, HOTSTATE_VAR(pc), Imm32(address));
#if defined(COMPARE_CORE)
            MOV(32, HOTSTATE_VAR(dbg_pc), Imm32(address));
#endif
            // Don't use compile_goto_dispatcher as we don't want this to be
            // used by the block linker.
            JMP(VR4300_Jitter::GetInstance()->GetJitCacheDispatcherStart(), XEmitter::Jump::Near);
        }
        switch_to_near_code();
    }
}

void VR4300_Jitter::embed_valid_block_check_pc()
{
    MOV(32, R(RSCRATCH), HOTSTATE_VAR(pc));
    AND(32, R(RSCRATCH), Imm32(0xC0000000));
    CMP(32, R(RSCRATCH), Imm32(0x80000000));

    FixupBranch translation_needed = J_CC(CC_NE, XEmitter::Jump::Near);
    switch_to_far_code();
    SetJumpTarget(translation_needed);

    MOV(32, R(RSCRATCH), HOTSTATE_VAR(pc));
    SHR(32, R(RSCRATCH), Imm8(12));

    MOV(64, R(RSCRATCH2), ImmPtr(m_valid_virtual_block));
    TEST(8, MRegSum(RSCRATCH2, RSCRATCH), Imm8(1));

    FixupBranch valid_block = J_CC(CC_NZ, XEmitter::Jump::Near);

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        m_gpr.Flush();
        m_fpr.Flush();

        // Don't use compile_goto_dispatcher as we don't want this to be
        // used by the block linker.
        JMP(m_jitcache_dispatcher_start, XEmitter::Jump::Near);
    }
    switch_to_near_code();
    SetJumpTarget(valid_block);
}

#define COMPARE_CORE_SAVE_INSTRUCTIONS_FOR_DBG(op) \
    MOV(32, R(RSCRATCH), HOTSTATE_VAR(last_idx)); \
    SHL(32, R(RSCRATCH), Imm8(2)); \
    MOV(64, R(RSCRATCH2), ImmPtr(HOT_STATE->last_addresses)); \
    ADD(64, R(RSCRATCH2), R(RSCRATCH)); \
    MOV(32, MatR(RSCRATCH2), Imm32(op.address)); \
    MOV(64, R(RSCRATCH2), ImmPtr(HOT_STATE->last_instructions)); \
    ADD(64, R(RSCRATCH2), R(RSCRATCH)); \
    MOV(32, MatR(RSCRATCH2), Imm32(op.instruction)); \
    SHL(32, R(RSCRATCH), Imm8(1)); \
    MOV(64, R(RSCRATCH2), ImmPtr(HOT_STATE->last_names)); \
    ADD(64, R(RSCRATCH2), R(RSCRATCH)); \
    MOV(64, R(RSCRATCH), ImmPtr(op.name)); \
    MOV(64, MatR(RSCRATCH2), R(RSCRATCH)); \
    ADD(32, HOTSTATE_VAR(last_idx), Imm32(1)); \
    CMP(32, HOTSTATE_VAR(last_idx), Imm32(NUM_DBG_INSTRUCTIONS)); \
    FixupBranch neq = J_CC(CC_NE); \
    MOV(32, HOTSTATE_VAR(last_idx), Imm32(0)); \
    SetJumpTarget(neq);

void VR4300_Jitter::recompile_instruction(struct jit_instr *op)
{
#if PROFILE_INSTRUCTIONS
    const void *instruction_start = GetCodePtr();
#endif
#if DEBUG_BLOCK
    if (HOT_STATE->pc == DEBUG_BLOCK_ADDRESS) {
        struct jit_instr instr;
        AnalyzeInstruction(&instr, op->instruction, op->address);
        fprintf(stderr, "%s: ", op->name);
        PRINT_ARG(instr, a);
        PRINT_ARG(instr, b);
        PRINT_ARG(instr, c);
        PRINT_ARG(instr, d);
        PRINT_ARG(instr, f);
        PRINT_ARG(instr, k);
        PRINT_ARG(instr, s);
        PRINT_ARG(instr, t);
        PRINT_ARG(instr, x);
        NOP();
        NOP();
        NOP();
        fprintf(stderr, "\n");
    }
#endif
#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "RECOMPILING: %s@0x%x\n", op->name, op->address);
    {
        struct jit_instr instr;
        AnalyzeInstruction(&instr, op->instruction, op->address);
        DebugMessage(M64MSG_VERBOSE, "%s: ", op->name);
        PRINT_ARG(instr, a);
        PRINT_ARG(instr, b);
        PRINT_ARG(instr, c);
        PRINT_ARG(instr, d);
        PRINT_ARG(instr, f);
        PRINT_ARG(instr, k);
        PRINT_ARG(instr, s);
        PRINT_ARG(instr, t);
        PRINT_ARG(instr, x);
        fprintf(stderr, "\n");
    }
#endif

#if COUNT_INSTRUCTIONS
    ADD(64, M(&instrcount), Imm32(1));
#endif
#if defined(COMPARE_CORE)
    COMPARE_CORE_SAVE_INSTRUCTIONS_FOR_DBG(op[0]);
#endif
#if PRINT_IM_HERE_DEBUG
    PRINT_IM_HERE(Imm32(op->address), Imm32(op->instruction), Imm64((u64)(is_nop(op) ? "NOP" : op->name)), Imm32(vr4300_jitter_translate_address_no_exception(op->address, 2)));
#endif

    bool is_float = false;

    if ((op->has_x && op->x == 1) || op->has_a) {
        if (!HOT_STATE->float_check_compiled) {
            // float operation
            compile_cop1_usable_check(op);
            HOT_STATE->float_check_compiled = true;
        }
        is_float = true;
    }

    if (op->has_x && op->x == 2) {
        if (!HOT_STATE->ctc2_check_compiled) {
            compile_cop2_usable_check(op);
            HOT_STATE->ctc2_check_compiled = true;
        }
    }

    bool interpret_float = false;

    if (is_float && interpret_float && !op->is_branch_or_jump) {
        DebugMessage(M64MSG_VERBOSE, "INTERPRETER FALLBACK@0x%08x: %d INSTRUCTION: %x, %s\n", op->address, op->operation, op->instruction, op->name);
        compile_INTERPRETER_FALLBACK(op, op->address);
    } else {
        const u8* before = GetCodePtr();
        switch (op->operation) {
            case VR4300_OP_ADD:
                recompile_ADD(op);
                break;
            case VR4300_OP_ADDI:
                recompile_ADDI(op);
                break;
            case VR4300_OP_SUB:
                recompile_SUB(op);
                break;
            case VR4300_OP_DSUB:
                recompile_DSUB(op);
                break;
            case VR4300_OP_DSUBU:
                recompile_DSUBU(op);
                break;
            case VR4300_OP_SUBU:
                recompile_SUBU(op);
                break;
            case VR4300_OP_ADDU:
                recompile_ADDU(op);
                break;
            case VR4300_OP_ORI:
                recompile_ORI(op);
                break;
            case VR4300_OP_OR:
                recompile_OR(op);
                break;
            case VR4300_OP_NOR:
                recompile_NOR(op);
                break;
            case VR4300_OP_ADDIU:
                recompile_ADDIU(op);
                break;
            case VR4300_OP_DADDIU:
                recompile_DADDIU(op);
                break;
            case VR4300_OP_DADDI:
                recompile_DADDI(op);
                break;
            case VR4300_OP_DADD:
                recompile_DADD(op);
                break;
            case VR4300_OP_DADDU:
                recompile_DADDU(op);
                break;
            case VR4300_OP_SLTI:
                recompile_SLTI(op);
                break;
            case VR4300_OP_SLTIU:
                recompile_SLTIU(op);
                break;
            case VR4300_OP_CACHE:
                recompile_CACHE(op);
                break;
            case VR4300_OP_SLT:
                recompile_SLT(op);
                break;
            case VR4300_OP_SLTU:
                recompile_SLTU(op);
                break;
            case VR4300_OP_AND:
                recompile_AND(op);
                break;
            case VR4300_OP_MFLO:
                recompile_MFLO(op);
                break;
            case VR4300_OP_MFHI:
                recompile_MFHI(op);
                break;
            case VR4300_OP_MTLO:
                recompile_MTLO(op);
                break;
            case VR4300_OP_MTHI:
                recompile_MTHI(op);
                break;
            case VR4300_OP_DDIVU:
                recompile_DDIVU(op);
                break;
            case VR4300_OP_DDIV:
                recompile_DDIV(op);
                break;
            case VR4300_OP_DIVU:
                recompile_DIVU(op);
                break;
            case VR4300_OP_DIV:
                recompile_DIV(op);
                break;
            case VR4300_OP_MULTU:
                recompile_MULTU(op);
                break;
            case VR4300_OP_MULT:
                recompile_MULT(op);
                break;
            case VR4300_OP_DMULTU:
                recompile_DMULTU(op);
                break;
            case VR4300_OP_DMULT:
                recompile_DMULT(op);
                break;
            case VR4300_OP_DSLL32:
                recompile_DSLL32(op);
                break;
            case VR4300_OP_DSRA32:
                recompile_DSRA32(op);
                break;
            case VR4300_OP_DSRA:
                recompile_DSRA(op);
                break;
            case VR4300_OP_DSLL:
                recompile_DSLL(op);
                break;
            case VR4300_OP_DSLLV:
                recompile_DSLLV(op);
                break;
            case VR4300_OP_DSRL:
                recompile_DSRL(op);
                break;
            case VR4300_OP_DSRLV:
                recompile_DSRLV(op);
                break;
            case VR4300_OP_DSRL32:
                recompile_DSRL32(op);
                break;
            case VR4300_OP_ANDI:
                recompile_ANDI(op);
                break;
            case VR4300_OP_LW:
                recompile_LW(op);
                break;
            case VR4300_OP_LWU:
                recompile_LWU(op);
                break;
            case VR4300_OP_LL:
                recompile_LL(op);
                break;
            case VR4300_OP_LWL:
                recompile_LWL(op);
                break;
            case VR4300_OP_LDL:
                recompile_LDL(op);
                break;
            case VR4300_OP_LWR:
                recompile_LWR(op);
                break;
            case VR4300_OP_LDR:
                recompile_LDR(op);
                break;
            case VR4300_OP_LD:
                recompile_LD(op);
                break;
            case VR4300_OP_LH:
                recompile_LH(op);
                break;
            case VR4300_OP_LHU:
                recompile_LHU(op);
                break;
            case VR4300_OP_TLBWI:
                recompile_TLBWI(op);
                break;
            case VR4300_OP_TLBWR:
                recompile_TLBWR(op);
                break;
            case VR4300_OP_TLBP:
                recompile_TLBP(op);
                break;
            case VR4300_OP_TLBR:
                recompile_TLBR(op);
                break;
            case VR4300_OP_LBU:
                recompile_LBU(op);
                break;
            case VR4300_OP_LB:
                recompile_LB(op);
                break;
            case VR4300_OP_LWC1:
                recompile_LWC1(op);
                break;
            case VR4300_OP_SW:
                recompile_SW(op);
                break;
            case VR4300_OP_SC:
                recompile_SC(op);
                break;
            case VR4300_OP_SH:
                recompile_SH(op);
                break;
            case VR4300_OP_SWL:
                recompile_SWL(op);
                break;
            case VR4300_OP_SDL:
                recompile_SDL(op);
                break;
            case VR4300_OP_SDR:
                recompile_SDR(op);
                break;
            case VR4300_OP_SWR:
                recompile_SWR(op);
                break;
            case VR4300_OP_SWC1:
                recompile_SWC1(op);
                break;
            case VR4300_OP_SYSCALL:
                recompile_SYSCALL(op);
                break;
            case VR4300_OP_TRUNC_W_S:
                recompile_TRUNC_W_S(op);
                break;
            case VR4300_OP_FLOOR_W_S:
                recompile_FLOOR_W_S(op);
                break;
            case VR4300_OP_FLOOR_W_D:
                recompile_FLOOR_W_D(op);
                break;
            case VR4300_OP_FLOOR_L_D:
                recompile_FLOOR_L_D(op);
                break;
            case VR4300_OP_FLOOR_L_S:
                recompile_FLOOR_L_S(op);
                break;
            case VR4300_OP_TRUNC_W_D:
                recompile_TRUNC_W_D(op);
                break;
            case VR4300_OP_TRUNC_L_S:
                recompile_TRUNC_L_S(op);
                break;
            case VR4300_OP_TRUNC_L_D:
                recompile_TRUNC_L_D(op);
                break;
            case VR4300_OP_ROUND_W_S:
                recompile_ROUND_W_S(op);
                break;
            case VR4300_OP_ROUND_L_D:
                recompile_ROUND_L_D(op);
                break;
            case VR4300_OP_ROUND_L_S:
                recompile_ROUND_L_S(op);
                break;
            case VR4300_OP_ROUND_W_D:
                recompile_ROUND_W_D(op);
                break;
            case VR4300_OP_CVT_S_D:
                recompile_CVT_S_D(op);
                break;
            case VR4300_OP_CVT_S_L:
                recompile_CVT_S_L(op);
                break;
            case VR4300_OP_CVT_S_W:
                recompile_CVT_S_W(op);
                break;
            case VR4300_OP_CVT_W_D:
                recompile_CVT_W_D(op);
                break;
            case VR4300_OP_CVT_W_S:
                recompile_CVT_W_S(op);
                break;
            case VR4300_OP_CVT_D_S:
                recompile_CVT_D_S(op);
                break;
            case VR4300_OP_CVT_D_L:
                recompile_CVT_D_L(op);
                break;
            case VR4300_OP_CVT_D_W:
                recompile_CVT_D_W(op);
                break;
            case VR4300_OP_CVT_L_S:
                recompile_CVT_L_S(op);
                break;
            case VR4300_OP_CVT_L_D:
                recompile_CVT_L_D(op);
                break;
            case VR4300_OP_MUL_S:
                recompile_MUL_S(op);
                break;
            case VR4300_OP_MUL_D:
                recompile_MUL_D(op);
                break;
            case VR4300_OP_DIV_S:
                recompile_DIV_S(op);
                break;
            case VR4300_OP_DIV_D:
                recompile_DIV_D(op);
                break;
            case VR4300_OP_ADD_S:
                recompile_ADD_S(op);
                break;
            case VR4300_OP_ABS_S:
                recompile_ABS_S(op);
                break;
            case VR4300_OP_ABS_D:
                recompile_ABS_D(op);
                break;
            case VR4300_OP_SUB_S:
                recompile_SUB_S(op);
                break;
            case VR4300_OP_SUB_D:
                recompile_SUB_D(op);
                break;
            case VR4300_OP_MOV_S:
                recompile_MOV_S(op);
                break;
            case VR4300_OP_SQRT_S:
                recompile_SQRT_S(op);
                break;
            case VR4300_OP_SQRT_D:
                recompile_SQRT_D(op);
                break;
            case VR4300_OP_MOV_D:
                recompile_MOV_D(op);
                break;
            case VR4300_OP_ADD_D:
                recompile_ADD_D(op);
                break;
            case VR4300_OP_C_cond_S:
                recompile_C_cond_S(op);
                break;
            case VR4300_OP_NEG_S:
                recompile_NEG_S(op);
                break;
            case VR4300_OP_NEG_D:
                recompile_NEG_D(op);
                break;
            case VR4300_OP_C_cond_D:
                recompile_C_cond_D(op);
                break;
            case VR4300_OP_LDC1:
                recompile_LDC1(op);
                break;
            case VR4300_OP_SDC1:
                recompile_SDC1(op);
                break;
            case VR4300_OP_SD:
                recompile_SD(op);
                break;
            case VR4300_OP_SB:
                recompile_SB(op);
                break;
            case VR4300_OP_XOR:
                recompile_XOR(op);
                break;
            case VR4300_OP_XORI:
                recompile_XORI(op);
                break;
            case VR4300_OP_BNE:
                recompile_BNE(op);
                break;
            case VR4300_OP_BNEL:
                recompile_BNEL(op);
                break;
            case VR4300_OP_BLEZL:
                recompile_BLEZL(op);
                break;
            case VR4300_OP_BLEZ:
                recompile_BLEZ(op);
                break;
            case VR4300_OP_BGTZL:
                recompile_BGTZL(op);
                break;
            case VR4300_OP_BGTZ:
                recompile_BGTZ(op);
                break;
            case VR4300_OP_BEQ:
                recompile_BEQ(op);
                break;
            case VR4300_OP_BEQL:
                recompile_BEQL(op);
                break;
            case VR4300_OP_BLTZ:
                recompile_BLTZ(op);
                break;
            case VR4300_OP_BLTZL:
                recompile_BLTZL(op);
                break;
            case VR4300_OP_BLTZAL:
                recompile_BLTZAL(op);
                break;
            case VR4300_OP_BLTZALL:
                recompile_BLTZALL(op);
                break;
            case VR4300_OP_SLL:
                recompile_SLL(op);
                break;
            case VR4300_OP_SRL:
                recompile_SRL(op);
                break;
            case VR4300_OP_SRAV:
                recompile_SRAV(op);
                break;
            case VR4300_OP_SRA:
                recompile_SRA(op);
                break;
            case VR4300_OP_SRLV:
                recompile_SRLV(op);
                break;
            case VR4300_OP_SLLV:
                recompile_SLLV(op);
                break;
            case VR4300_OP_DSRAV:
                recompile_DSRAV(op);
                break;
            case VR4300_OP_LUI:
                recompile_LUI(op);
                break;
            case VR4300_OP_MTC0:
                recompile_MTC0(op);
                break;
            case VR4300_OP_DMTC0:
                recompile_DMTC0(op);
                break;
            case VR4300_OP_MTC2:
                recompile_MTC2(op);
                break;
            case VR4300_OP_DMTC2:
                recompile_DMTC2(op);
                break;
            case VR4300_OP_MFC0:
                recompile_MFC0(op);
                break;
            case VR4300_OP_DMFC0:
                recompile_DMFC0(op);
                break;
            case VR4300_OP_MFC1:
                recompile_MFC1(op);
                break;
            case VR4300_OP_DMFC1:
                recompile_DMFC1(op);
                break;
            case VR4300_OP_MTC1:
                recompile_MTC1(op);
                break;
            case VR4300_OP_DMTC1:
                recompile_DMTC1(op);
                break;
            case VR4300_OP_CTC1:
                recompile_CTC1(op);
                break;
            case VR4300_OP_CFC1:
                recompile_CFC1(op);
                break;
            case VR4300_OP_DCFC1:
                recompile_DCFC1(op);
                break;
            case VR4300_OP_DCTC1:
                recompile_DCTC1(op);
                break;
            case VR4300_OP_CTC2:
                recompile_CTC2(op);
                break;
            case VR4300_OP_CFC2:
                recompile_CFC2(op);
                break;
            case VR4300_OP_MFC2:
                recompile_MFC2(op);
                break;
            case VR4300_OP_DMFC2:
                recompile_DMFC2(op);
                break;
            case VR4300_OP_JAL:
                recompile_JAL(op);
                break;
            case VR4300_OP_JALR:
                recompile_JALR(op);
                break;
            case VR4300_OP_JR:
                recompile_JR(op);
                break;
            case VR4300_OP_J:
                recompile_J(op);
                break;
            case VR4300_OP_BGEZL:
                recompile_BGEZL(op);
                break;
            case VR4300_OP_BGEZ:
                recompile_BGEZ(op);
                break;
            case VR4300_OP_BGEZAL:
                recompile_BGEZAL(op);
                break;
            case VR4300_OP_BGEZALL:
                recompile_BGEZALL(op);
                break;
            case VR4300_OP_ERET:
                recompile_ERET(op);
                break;
            case VR4300_OP_BC1T:
                recompile_BC1T(op);
                break;
            case VR4300_OP_BC1TL:
                recompile_BC1TL(op);
                break;
            case VR4300_OP_BC1F:
                recompile_BC1F(op);
                break;
            case VR4300_OP_BC1FL:
                recompile_BC1FL(op);
                break;
            case VR4300_OP_SYNC:
                recompile_SYNC(op);
                break;
            case VR4300_OP_BREAK:
                recompile_BREAK(op);
                break;
            case VR4300_OP_CEIL_W_D:
                recompile_CEIL_W_D(op);
                break;
            case VR4300_OP_CEIL_W_S:
                recompile_CEIL_W_S(op);
                break;
            case VR4300_OP_CEIL_L_D:
                recompile_CEIL_L_D(op);
                break;
            case VR4300_OP_CEIL_L_S:
                recompile_CEIL_L_S(op);
                break;
            case VR4300_OP_TEQ:
                recompile_TEQ(op);
                break;
            case VR4300_OP_DCFC2:
            case VR4300_OP_LDC2:
            case VR4300_OP_DCTC2:
            case VR4300_OP_LWC2:
            case VR4300_OP_SDC2:
            case VR4300_OP_SWC2:
                recompile_RESERVED_COP2(op);
                break;
            case VR4300_OP_CVT_W_W:
            case VR4300_OP_CVT_L_L:
            case VR4300_OP_CVT_S_S:
            case VR4300_OP_CVT_D_D:
                recompile_RESERVED(op);
                break;
            default:
                DebugMessage(M64MSG_VERBOSE, "UNIMPLEMENTED OPERATION@0x%08x: %d INSTRUCTION: %x, %s\n", op->address, op->operation, op->instruction, op->name);
                compile_INTERPRETER_FALLBACK(op, op->address);
                break;
        }
        const u8 *after = GetCodePtr();
#ifndef NDEBUG
        DebugMessage(M64MSG_VERBOSE, "%s: %d bytes\n", op->name, after - before);
#endif
    }

    // we reach here if the branch was not taken
    if (op->is_branch_or_jump && !is_linking_branch(op)) {
        compile_cycle_count_checks(op, op[0].address + 8, is_likely_branch(op), false, op[0].address + 8);
    }

#if PROFILE_INSTRUCTIONS
    std::stringstream name_for_profiling;
    name_for_profiling << "JIT_VR4300_" << op->name << "_" << std::hex << std::setfill('0') << std::setw(sizeof(u32) * 2) << op->address;
    if (HOT_STATE->inDelaySlot) {
        name_for_profiling << "_DS";
    }
    Common::JitRegister::Register(instruction_start, GetCodePtr(), name_for_profiling.str());
#endif
}

void VR4300_Jitter::recompile_delay_slot(struct jit_instr *op, bool skip_instruction)
{
    bool was_last_instruction = HOT_STATE->isLastInstruction;
    assert(!HOT_STATE->inDelaySlot);

    struct jit_instr *old_op = HOT_STATE->op;
    HOT_STATE->instructionsLeft--;
    HOT_STATE->op = op;

    HOT_STATE->compiler_cycles += m_r4300->cp0.count_per_op;

    HOT_STATE->inDelaySlot = true;

    if (!skip_instruction && (was_last_instruction || (op->may_cause_exception && !is_nop(op)))) {
        MOV(32, HOTSTATE_VAR(delay_slot), Imm32(1));
    }

    if (was_last_instruction && !skip_instruction) {
        MOV(32, HOTSTATE_VAR(pc), Imm32(op[-1].address + 8));

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_3(64, ABI_PARAM1, ABI_PARAM2, ABI_PARAM3, RCOpArg::Imm64((u64)m_r4300), RCOpArg::Imm64(op[-1].address + 4), RCOpArg::Imm64(2));
        ABI_CallFunction(TLB_refill_exception);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

        do_core_compare(op, 0);

        if (!skip_instruction && (was_last_instruction || (op->may_cause_exception && !is_nop(op)))) {
            MOV(32, HOTSTATE_VAR(delay_slot), Imm32(0));
        }

        compile_goto_dispatcher_destinhotstate(&op[-1], false);
    } else if (!op->is_branch_or_jump) {
        embed_valid_block_check(skip_instruction ? op->address + 4 : op->address, (op->address & 0xFFF) == 0, !skip_instruction, skip_instruction);
        if (!skip_instruction) {
            recompile_instruction(op);
        }

        m_gpr.Commit();
        m_fpr.Commit();

#if !DISABLE_GPR_REG_CACHE
#if defined(COMPARE_CORE) || PRINT_IM_HERE_DEBUG
        save_discarded_registers_for_core_compare(op);
#endif
#endif
#if !DISABLE_FLOAT_REG_CACHE
#if defined(COMPARE_CORE) || PRINT_IM_HERE_DEBUG
        save_discarded_registers_for_core_compare_float(op);
#endif
#endif

        m_fpr.ConvertTo64(op->fregsIncompatible);
        m_fpr.ConvertTo32(op->fregsIncompatible32);

        m_gpr.Flush(~op->regsInUse & (op->regsIn | op->regsOut));
        m_fpr.Flush(~op->fregsInUse & (op->fregsIn | op->fregsOut | op->fregsIn32 | op->fregsOut32));

#ifndef NDEBUG
        BitSet32 flushfregs = ~op->fregsInUse & (op->fregsIn | op->fregsOut | op->fregsIn32 | op->fregsOut32);
        if (flushfregs != BitSet32{}) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR FLUSHDS: ", op->name);
            for (preg_t p : flushfregs) {
                fprintf(stderr, "%d ", p);
            }
            fprintf(stderr, "\n");
        }

        if (op->fregsIncompatible != BitSet32{}) {
            DebugMessage(M64MSG_VERBOSE, "%s INCOMPATIBLEDS: ", op->name);
            for (preg_t p : op->fregsIncompatible) {
                fprintf(stderr, "%d ", p);
            }
            fprintf(stderr, "\n");
        }
        if (op->fregsIncompatible32 != BitSet32{}) {
            DebugMessage(M64MSG_VERBOSE, "%s INCOMPATIBLEDS32: ", op->name);
            for (preg_t p : op->fregsIncompatible32) {
                fprintf(stderr, "%d ", p);
            }
            fprintf(stderr, "\n");
        }

        for (preg_t i : op->fregsOut) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR OUTDS %d\n", op->name, i);
        }

        for (preg_t i : op->fregsOut32) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR OUTDS32 %d\n", op->name, i);
        }

        for (preg_t i : op->fregsIn) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR INDS %d\n", op->name, i);
        }

        for (preg_t i : op->fregsIn32) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR INDS32 %d\n", op->name, i);
        }
#endif

    } else abort();

    if (!skip_instruction && (was_last_instruction || (op->may_cause_exception && !is_nop(op)))) {
        MOV(32, HOTSTATE_VAR(delay_slot), Imm32(0));
    }

    if (!m_gpr.SanityCheck() || !m_fpr.SanityCheck())
    {
        DebugMessage(M64MSG_VERBOSE, "SANITY CHECK FAILED\n");
        abort();
    }

    HOT_STATE->compiler_cycles -= m_r4300->cp0.count_per_op;

    HOT_STATE->inDelaySlot = false;

    HOT_STATE->instructionsLeft++;
    HOT_STATE->op = old_op;
}

void VR4300_Jitter::clear_ranges_to_free()
{
    m_ranges_to_free_on_next_codegen_near.clear();
    m_ranges_to_free_on_next_codegen_far.clear();
}

void *vr4300_jitter_recompile_block(unsigned int addr)
{
    return VR4300_Jitter::GetInstance()->RecompileBlock(addr);
}

void *VR4300_Jitter::RecompileBlock(unsigned int addr)
{
    if (!HOT_STATE->rdram_corruption_changed) {
        HOT_STATE->rdram_corruption_changed = 0;

        VR4300_Jitter::GetInstance()->ClearCache();
    }

    for (auto range : m_ranges_to_free_on_next_codegen_near)
        m_free_ranges_near.insert(range.first, range.second);
    for (auto range : m_ranges_to_free_on_next_codegen_far)
        m_free_ranges_far.insert(range.first, range.second);
    clear_ranges_to_free();

    // set emitter state to free space
    const auto free_range_near = m_free_ranges_near.by_size_begin();
    if (free_range_near == m_free_ranges_near.by_size_end())
    {
        DebugMessage(M64MSG_VERBOSE, "Failed to find free memory region in the code region.");
        vr4300_jitter_invalidate_cached_code(&g_dev.r4300, 0, 0);
        // TODO: prevent endless recursion
        return RecompileBlock(addr);
    }
    const auto free_range_far = m_free_ranges_far.by_size_begin();
    if (free_range_far == m_free_ranges_far.by_size_end())
    {
        DebugMessage(M64MSG_VERBOSE, "Failed to find free memory region in the farcode region.");
        vr4300_jitter_invalidate_cached_code(&g_dev.r4300, 0, 0);
        // TODO: prevent endless recursion
        return RecompileBlock(addr);
    }

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "NEAR RANGE: 0x%08x 0x%08x\n", free_range_near.from(), free_range_near.to());
    DebugMessage(M64MSG_VERBOSE, "FAR RANGE: 0x%08x 0x%08x\n", free_range_far.from(), free_range_far.to());
#endif

    SetCodePtr(free_range_near.from(), free_range_near.to());
    m_far_code.SetCodePtr(free_range_far.from(), free_range_far.to());

    bool float_check_compiled_for_retry = HOT_STATE->float_check_compiled;
    bool ctc2_check_compiled_for_retry = HOT_STATE->ctc2_check_compiled;
    bool modifies_count_reg_for_retry = HOT_STATE->modifies_count_reg;
    bool modifies_status_reg_for_retry = HOT_STATE->modifies_status_reg;
    int compiler_cycles_for_retry = HOT_STATE->compiler_cycles;

    HOT_STATE->last_block_broken &= !HOT_STATE->pc_changed;
    HOT_STATE->pc_changed = 0;
    if (!HOT_STATE->last_block_broken) {
        HOT_STATE->modifies_count_reg = false;
        HOT_STATE->modifies_status_reg = false;
        HOT_STATE->float_check_compiled = false;
        HOT_STATE->ctc2_check_compiled = false;
    } else {
#ifndef NDEBUG
        DebugMessage(M64MSG_VERBOSE, "LAST BLOCK BROKEN! %d %d\n", HOT_STATE->compiler_cycles, HOT_STATE->cycles_added);
#endif
    }
#ifndef NDEBUG
#ifdef PRINT_EXTRA_EXTRA_STUFF
        DebugMessage(M64MSG_VERBOSE, "RECOMPILING: 0x%08x\n", addr);
#endif
#endif

    // analyze instructions
    struct prepared_code_block code_block = {0};
    unsigned int nextPC = Analyze(addr, &code_block, MAX_INSTR_PER_BLOCK);
    HOT_STATE->exception = code_block.exception;
    HOT_STATE->exceptionless_block = code_block.exceptionless_block && !HOT_STATE->last_block_broken;

#ifndef NDEBUG
    DebugMessage(M64MSG_VERBOSE, "NUM INSTRUCTIONS: 0x%x %d npc: %x", addr, code_block.num_instructions, nextPC);
#endif

    HOT_STATE->modifies_count_reg |= code_block.modifies_count_reg;
    HOT_STATE->modifies_status_reg |= code_block.modifies_status_reg;

    // setup block
    u8 *start = GetWritableCodePtr();
    u8 *farcode_start = m_far_code.GetWritableCodePtr();
    u8 *host_entry = nullptr;
    std::set<uint32_t> physical_addresses;
    std::set<uint32_t> virtual_addresses;
    JitBlock *b = nullptr;
    if (!HOT_STATE->last_block_broken && !(code_block.stopped_early && code_block.exception)) {
        b = m_block_cache.AllocateBlock(addr, vr4300_jitter_translate_address_no_exception(addr, 2) & MEMORY_MASK);
    }
#ifndef NDEBUG
#ifdef PRINT_EXTRA_EXTRA_STUFF
    fprintf(stderr, "SKIP: ALLOCATED: 0x%08x 0x%08x %d %d %d %d\n", addr, vr4300_jitter_translate_address_no_exception(addr, 2) & MEMORY_MASK, !HOT_STATE->last_block_broken && !(code_block.stopped_early && code_block.exception), HOT_STATE->last_block_broken, code_block.stopped_early, code_block.exception);
#endif
#endif
    HOT_STATE->curBlock = b;

    HOT_STATE->is_idle_wait_loop = code_block.is_idle_wait_loop;

    host_entry = AlignCode4();

    // PRINT_IM_HERE_BLOCK_START(addr);

    // This is needed in case of block linking...
    // embed_valid_block_check(addr, true);

    m_gpr.Start();
    m_fpr.Start();

    if (!HOT_STATE->last_block_broken) {
        HOT_STATE->compiler_cycles = 0;
        MOV(32, HOTSTATE_VAR(cycles_added), Imm32(0));
    } else {
        MOV(32, HOTSTATE_VAR(cycles_added), Imm32(-HOT_STATE->cycles_added));
    }

    bool minus_one = !code_block.stopped_early;

    for (int i = 0; i < code_block.num_instructions - (minus_one ? 1 : 0); i++) {
        struct jit_instr &op = code_block.instr[i];
        HOT_STATE->op = &code_block.instr[i];
        HOT_STATE->branch_to = code_block.branch_to;
        bool skip_compare = false;

        HOT_STATE->isLastInstruction = (i == (code_block.num_instructions - 1));
        HOT_STATE->instructionsLeft = (code_block.num_instructions - 1) - i;

        // Idle skipping
#if !DISABLE_IDLE_SKIPPING
        if (HOT_STATE->is_idle_wait_loop && op.is_branch_or_jump) {
            update_count_reg();
            do_core_compare(&op, op.address);
            skip_compare = true;

            // TODO (this is slightly different than what the pure interpreter does).
            MOV(32, HOTSTATE_CP0REG(CP0_COUNT_REG), Imm32(m_r4300->cp0.count_per_op));
            MOV(32, HOTSTATE_VAR(cycle_count), Imm32(m_r4300->cp0.count_per_op));
            HOT_STATE->compiler_cycles -= g_dev.r4300.cp0.count_per_op;
        }
#endif

        // Branch following
        if (op.is_branch_or_jump && !HOT_STATE->is_idle_wait_loop) {
            if (i + 2 < code_block.num_instructions) {
                if (code_block.instr[i + 2].address != op.address + 8) {
                    // looks like we've followed a jump here, skip it?
                    if (vr4300_jitter_address_needs_translation(op.address)) {
                        physical_addresses.insert(vr4300_jitter_translate_address_no_exception(op.address, 2) & MEMORY_MASK);
                    } else {
                        physical_addresses.insert(op.address & MEMORY_MASK);
                    }
                    virtual_addresses.insert(op.address);

                    do_core_compare(&op, op.address);

#if defined(COMPARE_CORE)
                    COMPARE_CORE_SAVE_INSTRUCTIONS_FOR_DBG(op);
#endif
#if PRINT_IM_HERE_DEBUG
                    PRINT_IM_HERE(Imm32(op.address), Imm32(op.instruction), Imm64((u64)(is_nop(&op) ? "NOP" : op.name)), Imm32(vr4300_jitter_translate_address_no_exception(op.address, 2)));
#endif
                    if (i > 1 && code_block.instr[i - 2].is_branch_or_jump) {
                        HOT_STATE->compiler_cycles = 0;
                        HOT_STATE->cycles_added = 0;
                        MOV(32, HOTSTATE_VAR(cycles_added), Imm32(0));
                    }

                    // Note: JAL is not followed ever.
                    recompile_delay_slot(&code_block.instr[i + 1], false);
                    compile_cycle_count_checks(&op, code_block.instr[i + 2].address, false, false, op.address + 8);
                    continue;
                }
            }
        }

        // Check if we need to generate exception checks again.
        if (op.is_delay_slot) {
            // TODO: checking is_linking_branch is probably not valid here
            if (i > 0 && is_linking_branch(&code_block.instr[i - 1])) {
                HOT_STATE->float_check_compiled = false;
                HOT_STATE->ctc2_check_compiled = false;
            }

            continue;
        }

        // Reset compiler cycle count.
        if (i > 1 && code_block.instr[i - 2].is_branch_or_jump) {
            HOT_STATE->compiler_cycles = 0;
            HOT_STATE->cycles_added = 0;
            MOV(32, HOTSTATE_VAR(cycles_added), Imm32(0));
        }

        HOT_STATE->modifies_count_reg |= op.modifies_count_reg;
        HOT_STATE->modifies_status_reg |= op.modifies_status_reg;

#if DISABLE_GPR_REG_CACHE
        m_gpr.Flush();
#else
        m_gpr.PreloadRegisters(op.regsIn & op.regsInUse & ~op.regsDiscardable, false);
#endif

#if DISABLE_FLOAT_REG_CACHE
        m_fpr.Flush();
#else
        m_fpr.PreloadRegisters(op.fregsIn & op.fregsInUse & ~op.fregsDiscardable, false);
        m_fpr.PreloadRegisters(op.fregsIn32 & op.fregsInUse & ~op.fregsDiscardable, true);
#endif

        embed_valid_block_check(op.address, false);

        if (!skip_compare) do_core_compare(&op, op.address);

        recompile_instruction(&op);

        m_gpr.Commit();
        m_fpr.Commit();

        // delay slot
        if (op.is_branch_or_jump) {
            if (i + 1 < code_block.num_instructions) {
                if (vr4300_jitter_address_needs_translation(code_block.instr[i + 1].address)) {
                    physical_addresses.insert(vr4300_jitter_translate_address_no_exception(code_block.instr[i + 1].address, 2) & MEMORY_MASK);
                } else {
                    physical_addresses.insert(code_block.instr[i + 1].address & MEMORY_MASK);
                }
                virtual_addresses.insert(code_block.instr[i + 1].address);
            }
        }

        if (vr4300_jitter_address_needs_translation(op.address)) {
            physical_addresses.insert(vr4300_jitter_translate_address_no_exception(op.address, 2) & MEMORY_MASK);
        } else {
            physical_addresses.insert(op.address & MEMORY_MASK);
        }
        virtual_addresses.insert(op.address);

        HOT_STATE->compiler_cycles += m_r4300->cp0.count_per_op;

#ifndef NDEBUG
        for (preg_t i : op.regsDiscardable) {
            DebugMessage(M64MSG_VERBOSE, "%s GPR DISCARDABLE %d\n", op.name, i);
        }

        for (preg_t i : op.fregsDiscardable) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR DISCARDABLE %d\n", op.name, i);
        }

        for (preg_t i : op.fregsOut) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR OUT %d\n", op.name, i);
        }

        for (preg_t i : op.fregsOut32) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR OUT32 %d\n", op.name, i);
        }

        for (preg_t i : op.fregsIn) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR IN %d\n", op.name, i);
        }

        for (preg_t i : op.fregsIn32) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR IN32 %d\n", op.name, i);
        }
#endif

#if !DISABLE_GPR_REG_CACHE
#if defined(COMPARE_CORE) || PRINT_IM_HERE_DEBUG
        save_discarded_registers_for_core_compare(&op);
#endif
#endif

#if !DISABLE_FLOAT_REG_CACHE
#if defined(COMPARE_CORE) || PRINT_IM_HERE_DEBUG
        save_discarded_registers_for_core_compare_float(&op);
#endif
#endif

#if !DISABLE_GPR_REG_CACHE
#if !DISABLE_DISCARD
        m_gpr.Discard(op.regsDiscardable);
#endif
#endif

#if !DISABLE_FLOAT_REG_CACHE
#if !DISABLE_DISCARD
        m_fpr.Discard(op.fregsDiscardable);
#endif
#endif

        m_fpr.ConvertTo64(op.fregsIncompatible);
        m_fpr.ConvertTo32(op.fregsIncompatible32);

        m_gpr.Flush(~op.regsInUse & (op.regsIn | op.regsOut));
        m_fpr.Flush(~op.fregsInUse & (op.fregsIn | op.fregsOut | op.fregsIn32 | op.fregsOut32));

#ifndef NDEBUG
        BitSet32 flushfregs = ~op.fregsInUse & (op.fregsIn | op.fregsOut | op.fregsIn32 | op.fregsOut32);
        if (flushfregs != BitSet32{}) {
            DebugMessage(M64MSG_VERBOSE, "%s FPR FLUSH: ", op.name);
            for (preg_t p : flushfregs) {
                fprintf(stderr, "%d ", p);
            }
            fprintf(stderr, "\n");
        }
#endif

#ifndef NDEBUG
        if (op.fregsIncompatible != BitSet32{}) {
            DebugMessage(M64MSG_VERBOSE, "%s INCOMPATIBLE: ", op.name);
            for (preg_t p : op.fregsIncompatible) {
                fprintf(stderr, "%d ", p);
            }
            fprintf(stderr, "\n");
        }
        if (op.fregsIncompatible32 != BitSet32{}) {
            DebugMessage(M64MSG_VERBOSE, "%s INCOMPATIBLE32: ", op.name);
            for (preg_t p : op.fregsIncompatible32) {
                fprintf(stderr, "%d ", p);
            }
            fprintf(stderr, "\n");
        }
#endif

#if PRINT_FLUSHES
        BitSet32 gprs = ~op.regsInUse & (op.regsIn | op.regsOut);
        PRINT_FLUSH(gprs);
#endif

        if (!m_gpr.SanityCheck() || !m_fpr.SanityCheck())
        {
            DebugMessage(M64MSG_VERBOSE, "SANITY CHECK FAILED\n");
            abort();
        }
    }

    if (code_block.stopped_early && code_block.exception) {
        MOV(32, HOTSTATE_VAR(last_block_broken), Imm32(1));
        MOV(32, HOTSTATE_VAR(compiler_cycles), Imm32(HOT_STATE->compiler_cycles));
        MOV(32, HOTSTATE_VAR(modifies_status_reg), Imm32(HOT_STATE->modifies_status_reg));
        MOV(32, HOTSTATE_VAR(float_check_compiled), Imm32(HOT_STATE->float_check_compiled));
        MOV(32, HOTSTATE_VAR(ctc2_check_compiled), Imm32(HOT_STATE->ctc2_check_compiled));
        MOV(32, HOTSTATE_VAR(modifies_count_reg), Imm32(HOT_STATE->modifies_count_reg));
    }

    HOT_STATE->compiler_cycles -= m_r4300->cp0.count_per_op;

    RCForkGuard gpr_guard = m_gpr.Fork();
    RCForkGuard fpr_guard = m_fpr.Fork();

    compile_goto_dispatcher(NULL, nextPC, false);

    AlignCode4();

    if (HasWriteFailed() || m_far_code.HasWriteFailed()) {
        DebugMessage(M64MSG_VERBOSE, "Write failed, clear cache %d %d at 0x%08x.\n", HasWriteFailed(), m_far_code.HasWriteFailed(), addr);
        vr4300_jitter_invalidate_cached_code(&g_dev.r4300, 0, 0);

        HOT_STATE->modifies_count_reg = modifies_count_reg_for_retry;
        HOT_STATE->float_check_compiled = float_check_compiled_for_retry;
        HOT_STATE->ctc2_check_compiled = ctc2_check_compiled_for_retry;
        HOT_STATE->modifies_status_reg = modifies_status_reg_for_retry;
        HOT_STATE->compiler_cycles = compiler_cycles_for_retry;
        return RecompileBlock(addr);
    }

    // Need to set this regardless of whether we save the block or not
    // otherwise we will loop endlessly (see embed_valid_block_check).
    for (u32 addr : virtual_addresses) {
        ValidBlockSetVirtual(addr);
    }

    if (HOT_STATE->last_block_broken) {
        HOT_STATE->last_block_broken = 0;
        // exit without saving the block
        return host_entry;
    }

    if ((code_block.stopped_early && code_block.exception)) {
        // exit without saving the block
        return host_entry;
    } else {
        HOT_STATE->last_block_broken = 0;
    }

    b->code_begin = start;
    b->farcode_begin = farcode_start;
    b->farcode_end = m_far_code.GetWritableCodePtr();
    b->host_entry = host_entry;
    b->code_end = GetWritableCodePtr();
    m_block_cache.FinalizeBlock(*b, virtual_addresses, physical_addresses);

#if !PROFILE_INSTRUCTIONS
    std::stringstream name_for_profiling;

    name_for_profiling << "JIT_VR4300_" << std::hex << std::setfill('0') << std::setw(sizeof(u32) * 2) << b->virtual_address << "_" << b->physical_address;
    Common::JitRegister::Register(b->code_begin, b->code_end, name_for_profiling.str());

    name_for_profiling.str("");
    name_for_profiling.clear();

    name_for_profiling << "JIT_VR4300_FAR_" << std::hex << std::setfill('0') << std::setw(sizeof(u32) * 2) << b->virtual_address << "_" << b->physical_address;
    Common::JitRegister::Register(b->farcode_begin, b->farcode_end, name_for_profiling.str());
#endif

    if (b->code_begin != b->code_end) {
        m_free_ranges_near.erase(b->code_begin, b->code_end);
    }

    if (b->farcode_begin != b->farcode_end) {
        m_free_ranges_far.erase(b->farcode_begin, b->farcode_end);
    }

    return b->host_entry;
}

void vr4300_jitter_cleanup(void)
{
    VR4300_Jitter::GetInstance()->Cleanup();
}

void VR4300_Jitter::Cleanup()
{
#if COUNT_INSTRUCTIONS
    DebugMessage(M64MSG_VERBOSE, "instrcount %d\n", instrcount);
#endif
    m_block_cache.Clear();
    m_valid_block_arena.Clear();
    // m_block_cache.Shutdown();
}

void VR4300_Jitter::ClearCache()
{
    // set_fpr_pointers will call vr4300_jitter_invalidate_cached_code before we are ready
    if (m_initialized) {
        m_block_cache.Clear();
        m_const_pool.Clear();
        ResetCodePtr();

        generate_asm();
        Common::JitRegister::Register(m_enter_code, GetCodePtr(), "JIT_START");

        AlignCode16();
        m_free_code_start = GetWritableCodePtr();

        SetCodePtr(m_free_code_start, region + region_size, false);

        clear_ranges_to_free();

        m_valid_block_arena.Clear();

        m_back_patch_info.clear();

        m_far_code.ResetCodePtr();
        //m_far_code.ClearCodeSpace();

        m_free_ranges_near.clear();
        m_free_ranges_near.insert(m_free_code_start, region + region_size);
        m_free_ranges_far.clear();
        m_free_ranges_far.insert(m_far_code.GetWritableCodePtr(), m_far_code.GetWritableCodeEnd());
    }
}

void VR4300_Jitter::InvalidateCachedCodeJustErase(uint32_t address, size_t length)
{
    if (!vr4300_jitter_is_rdram_address(address)) return;

#if DEBUG_INVALIDATE
    bool have_block = false;
    for (u32 addr = address; addr < address + length; addr += 4) {
        if (m_valid_block_ptr[addr / 4]) {
            have_block = true;
        }
    }
    if (!have_block) {
        DebugMessage(M64MSG_VERBOSE, "INVALID ADDRESS FOR INVALIDATE: %x\n", address);
        abort();
    }
#endif

    for (u32 addr = address & ~3; addr < (address & ~3) + length; addr += 4)
    {
        ValidBlockUnset(addr);
    }

#ifndef NDEBUG
    fprintf(stderr, "INVALIDATING1: 0x%08x %d\n", address, length);
#endif
    m_block_cache.ErasePhysicalRange(address, length);
}

void VR4300_Jitter::InvalidateCachedCode(uint32_t address, size_t length)
{
    if (!vr4300_jitter_is_rdram_address(address)) return;

    address &= MEMORY_MASK;

    bool destroy_block = false;
    if (m_valid_block_ptr) {
        for (u32 addr = address & ~3; addr < (address & ~3) + length; addr += 4) {
            if (m_valid_block_ptr[addr / 4]) {
                destroy_block = true;
                ValidBlockUnset(addr);
            }
        }
    }

    if (destroy_block) {
#ifndef NDEBUG
        fprintf(stderr, "INVALIDATING2: 0x%08x %d\n", address, length);
#endif
        m_block_cache.ErasePhysicalRange(address, length);
    }
}

#define CTX_RAX gregs[REG_RAX]
#define CTX_RBX gregs[REG_RBX]
#define CTX_RCX gregs[REG_RCX]
#define CTX_RDX gregs[REG_RDX]
#define CTX_RDI gregs[REG_RDI]
#define CTX_RSI gregs[REG_RSI]
#define CTX_RBP gregs[REG_RBP]
#define CTX_RSP gregs[REG_RSP]
#define CTX_R8 gregs[REG_R8]
#define CTX_R9 gregs[REG_R9]
#define CTX_R10 gregs[REG_R10]
#define CTX_R11 gregs[REG_R11]
#define CTX_R12 gregs[REG_R12]
#define CTX_R13 gregs[REG_R13]
#define CTX_R14 gregs[REG_R14]
#define CTX_R15 gregs[REG_R15]
#define CTX_RIP gregs[REG_RIP]

static struct sigaction old_sa_segv;
static struct sigaction old_sa_bus;

bool handle_fault(uintptr_t access_address, SContext* ctx);

static void sigsegv_handler(int sig, siginfo_t* info, void* raw_context)
{
  if (sig != SIGSEGV && sig != SIGBUS)
  {
    DebugMessage(M64MSG_VERBOSE, "IGNORING SIGNAL %d\n", sig);
    // We are not interested in other signals - handle it as usual.
    return;
  }
  ucontext_t* context = (ucontext_t*)raw_context;
  int sicode = info->si_code;
  if (sicode != SEGV_MAPERR && sicode != SEGV_ACCERR)
  {
    // Huh? Return.
    DebugMessage(M64MSG_VERBOSE, "UNKNOWN SIGNAL\n");
    return;
  }
  uintptr_t bad_address = (uintptr_t)info->si_addr;

// Get all the information we can out of the context.
#ifdef __OpenBSD__
  ucontext_t* ctx = context;
#else
  mcontext_t* ctx = &context->uc_mcontext;
#endif
  // assume it's not a write
  if (!VR4300_Jitter::GetInstance()->HandleFault(bad_address,
#ifdef __APPLE__
                                                                 *ctx
#else
                                                                 ctx
#endif
                                                                 ))
  {
    // retry and crash
    // According to the sigaction man page, if sa_flags "SA_SIGINFO" is set to the sigaction
    // function pointer, otherwise sa_handler contains one of:
    // SIG_DEF: The 'default' action is performed
    // SIG_IGN: The signal is ignored
    // Any other value is a function pointer to a signal handler

    struct sigaction* old_sa;
    if (sig == SIGSEGV)
    {
      old_sa = &old_sa_segv;
    }
    else
    {
      old_sa = &old_sa_bus;
    }

    if (old_sa->sa_flags & SA_SIGINFO)
    {
      old_sa->sa_sigaction(sig, info, raw_context);
      return;
    }
    if (old_sa->sa_handler == SIG_DFL)
    {
      DebugMessage(M64MSG_VERBOSE, "SIG_DFL\n");
      signal(sig, SIG_DFL);
      return;
    }
    if (old_sa->sa_handler == SIG_IGN)
    {
      DebugMessage(M64MSG_VERBOSE, "IGNORING SIGNAL\n");
      // Ignore signal
      return;
    }
    old_sa->sa_handler(sig);
  }
}

void VR4300_Jitter::install_exception_handler(void)
{
  stack_t signal_stack;
#ifdef __FreeBSD__
  signal_stack.ss_sp = (char*)malloc(SIGSTKSZ);
#else
  signal_stack.ss_sp = malloc(SIGSTKSZ);
#endif
  signal_stack.ss_size = SIGSTKSZ;
  signal_stack.ss_flags = 0;
  if (sigaltstack(&signal_stack, nullptr))
    ASSERT_MSG(DYNA_REC, 0, "sigaltstack failed");
  struct sigaction sa;
  sa.sa_handler = nullptr;
  sa.sa_sigaction = &sigsegv_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGSEGV, &sa, &old_sa_segv);
#ifdef __APPLE__
  sigaction(SIGBUS, &sa, &old_sa_bus);
#endif
}

void *vr4300_jitter_initialize_fastmem(void)
{
    if (!gJitterInstance) gJitterInstance = new VR4300_Jitter();
    return VR4300_Jitter::GetInstance()->InitializeFastmem();
}

void *VR4300_Jitter::InitializeFastmem()
{
    constexpr size_t view_size = 0x1'0000'0000;
    constexpr size_t guard_size = 0x8000'0000;
    constexpr size_t address_space_size = view_size + guard_size * 2;

    uint32_t rom_size = CART_ROM_MAX_SIZE;
    uint32_t memory_size = 0;

    m_arena.GrabSHMSegment((unsigned int)RDRAM_MAX_SIZE + (unsigned int)SP_MEM_SIZE + (unsigned int)DD_ROM_MAX_SIZE + rom_size - 1 + 1 + (unsigned int)PIF_ROM_SIZE + (unsigned int)PIF_RAM_SIZE, "mupen64plus-memory");

    m_rdram_position = memory_size;
    m_rdram_ptr = m_arena.CreateView(memory_size, RDRAM_MAX_SIZE);
    memory_size += RDRAM_MAX_SIZE;

    m_rspmem_position = memory_size;
    m_rspmem_ptr = m_arena.CreateView(memory_size, SP_MEM_SIZE);
    memory_size += SP_MEM_SIZE;

    m_dd_position = memory_size;
    m_dd_ptr = m_arena.CreateView(memory_size, DD_ROM_MAX_SIZE);
    memory_size += DD_ROM_MAX_SIZE;

    m_cart_rom_position = memory_size;
    m_cart_rom_ptr = m_arena.CreateView(memory_size, rom_size-1+1);
    memory_size += rom_size - 1 + 1;

    m_pif_position = memory_size;
    m_pif_ptr = m_arena.CreateView(memory_size, (unsigned int)PIF_ROM_SIZE + (unsigned int)PIF_RAM_SIZE);
    memory_size += (unsigned int)PIF_ROM_SIZE + (unsigned int)PIF_RAM_SIZE;

    m_fastmem_arena = m_arena.ReserveMemoryRegion(address_space_size);
    m_physical_base = m_fastmem_arena + guard_size;
    m_fastmem_arena_size = address_space_size;

    install_exception_handler();

#if !DISABLE_FASTMEM
    if (m_arena.MapInMemoryRegion(m_rdram_position, RDRAM_MAX_SIZE, m_physical_base + MM_RDRAM_DRAM) != m_physical_base + MM_RDRAM_DRAM) {
        abort();
    }

    if (m_arena.MapInMemoryRegion(m_rdram_position, RDRAM_MAX_SIZE, m_physical_base + MM_RDRAM_DRAM + R4300_KSEG0) != m_physical_base + MM_RDRAM_DRAM + R4300_KSEG0) {
        abort();
    }

    if (m_arena.MapInMemoryRegion(m_rdram_position, RDRAM_MAX_SIZE, m_physical_base + MM_RDRAM_DRAM + R4300_KSEG1) != m_physical_base + MM_RDRAM_DRAM + R4300_KSEG1) {
        abort();
    }

    return m_physical_base;
#else
    return m_rdram_ptr;
#endif
}

void vr4300_jitter_map_corrupt_rdram(int corrupt)
{
    VR4300_Jitter::GetInstance()->MapCorruptRdram(corrupt);
}

void VR4300_Jitter::MapCorruptRdram(bool corrupt)
{
    HOT_STATE->rdram_generate_slowcode = corrupt;
    HOT_STATE->rdram_corruption_changed = 1;
#if !DISABLE_FASTMEM
    if (corrupt) {
        m_arena.UnmapFromMemoryRegion(m_physical_base + MM_RDRAM_DRAM, 0x1000);
        m_arena.UnmapFromMemoryRegion(m_physical_base + MM_RDRAM_DRAM + R4300_KSEG0, 0x1000);
        m_arena.UnmapFromMemoryRegion(m_physical_base + MM_RDRAM_DRAM + R4300_KSEG1, 0x1000);
    } else {
        if (m_arena.MapInMemoryRegion(m_rdram_position, RDRAM_MAX_SIZE, m_physical_base + MM_RDRAM_DRAM) != m_physical_base + MM_RDRAM_DRAM) {
            abort();
        }

        if (m_arena.MapInMemoryRegion(m_rdram_position, RDRAM_MAX_SIZE, m_physical_base + MM_RDRAM_DRAM + R4300_KSEG0) != m_physical_base + MM_RDRAM_DRAM + R4300_KSEG0) {
            abort();
        }

        if (m_arena.MapInMemoryRegion(m_rdram_position, RDRAM_MAX_SIZE, m_physical_base + MM_RDRAM_DRAM + R4300_KSEG1) != m_physical_base + MM_RDRAM_DRAM + R4300_KSEG1) {
            abort();
        }
    }
#endif

    // Can't clear the cache here, since we might still be in a block!
    // It happens on the next RecompileBlock.
}

bool VR4300_Jitter::is_address_in_fastmem_arena(const u8* address)
{
  return address >= m_fastmem_arena && address < m_fastmem_arena + m_fastmem_arena_size;
}

bool VR4300_Jitter::HandleFault(uintptr_t access_address, SContext* ctx)
{
    u32 address = ((u64)access_address - (u64)m_physical_base);
    if (is_address_in_fastmem_arena((u8*)access_address)) {
        if ((u64)ctx->CTX_RIP >= (u64)region && (u64)ctx->CTX_RIP < (u64)(region + region_size)) {

            auto info_it = m_back_patch_info.find(reinterpret_cast<u8*>(ctx->CTX_RIP));
            if (info_it == m_back_patch_info.end()) {
                // woops
                DebugMessage(M64MSG_VERBOSE, "Could not find backpatch info.\n");
                return false;
            }

            if (info_it->second.fault_count >= MAX_FAULT_COUNT) {
#ifndef NDEBUG
                DebugMessage(M64MSG_VERBOSE, "MAX FAULTS REACHED %d %p", info_it->second.fault_count, info_it->first);
#endif
                Gen::XEmitter emit(info_it->second.code_before, const_cast<u8*>(info_it->second.code_after));
                emit.JMP(info_it->second.farcode, XEmitter::Jump::Near);
                assert(!emit.HasWriteFailed());
            }

            u32 taddr = vr4300_jitter_translate_address_no_exception(address, 2);
#ifndef NDEBUG
            DebugMessage(M64MSG_VERBOSE, "FAULT %x/%x farcode %p, before: %p, fault %p, after: %p, fcount: %d\n", address, taddr, info_it->second.farcode, info_it->second.code_before, reinterpret_cast<u8*>(ctx->CTX_RIP), info_it->second.code_after, info_it->second.fault_count);
            if (taddr && vr4300_jitter_is_rdram_address(taddr)) {
                DebugMessage(M64MSG_VERBOSE, "FAULT IN RDRAM ADDRESS!\n");
            }
#endif
            ctx->CTX_RIP = reinterpret_cast<greg_t>(info_it->second.farcode);
            if (info_it->second.fault_count >= MAX_FAULT_COUNT) {
                m_back_patch_info.erase(info_it);
            } else {
                info_it->second.fault_count++;
            }
            return true;
        } else {
            DebugMessage(M64MSG_ERROR, "FAULT NOT IN CODE REGION: %p %x\n", (void*)access_address, address);
            return false;
        }
    } else {
#if DISPATCHER_NO_TEST // remove, not functional anymore and no benefit
        if (ctx->CTX_RIP == 0) {
          ctx->CTX_RIP = reinterpret_cast<greg_t>(m_dispatcher_slow);
          return true;
        }
#endif
        DebugMessage(M64MSG_ERROR, "UNKNOWN FAULT AT: %p %p %x\n", (void*)ctx->CTX_RIP, (void*)access_address, address);
        DebugMessage(M64MSG_ERROR, "CTX_RBX %p\n", (void*)ctx->CTX_RBX);
        DebugMessage(M64MSG_ERROR, "CTX_RCX %p\n", (void*)ctx->CTX_RCX);
        DebugMessage(M64MSG_ERROR, "CTX_RDX %p\n", (void*)ctx->CTX_RDX);
        DebugMessage(M64MSG_ERROR, "CTX_RDI %p\n", (void*)ctx->CTX_RDI);
        DebugMessage(M64MSG_ERROR, "CTX_RSI %p\n", (void*)ctx->CTX_RSI);
        DebugMessage(M64MSG_ERROR, "CTX_RBP %p\n", (void*)ctx->CTX_RBP);
        DebugMessage(M64MSG_ERROR, "CTX_RSP %p\n", (void*)ctx->CTX_RSP);
        DebugMessage(M64MSG_ERROR, "CTX_R8 %p\n", (void*)ctx->CTX_R8);
        DebugMessage(M64MSG_ERROR, "CTX_R9 %p\n", (void*)ctx->CTX_R9);
        DebugMessage(M64MSG_ERROR, "CTX_R10 %p\n", (void*)ctx->CTX_R10);
        DebugMessage(M64MSG_ERROR, "CTX_R11 %p\n", (void*)ctx->CTX_R11);
        DebugMessage(M64MSG_ERROR, "CTX_R12 %p\n", (void*)ctx->CTX_R12);
        DebugMessage(M64MSG_ERROR, "CTX_R13 %p\n", (void*)ctx->CTX_R13);
        DebugMessage(M64MSG_ERROR, "CTX_R14 %p\n", (void*)ctx->CTX_R14);
        DebugMessage(M64MSG_ERROR, "CTX_R15 %p\n", (void*)ctx->CTX_R15);
        abort();
        return false;
    }
}

uint8_t *vr4300_jitter_get_logical_memory(int type)
{
    return VR4300_Jitter::GetInstance()->GetLogicalMemory(type);
}

uint8_t *VR4300_Jitter::GetLogicalMemory(int type)
{
    switch (type) {
        case MM_PIF_MEM:
            return (u8*)m_pif_ptr;
        case MM_CART_ROM:
            return (u8*)m_cart_rom_ptr;
        case MM_RSP_MEM:
            return (u8*)m_rspmem_ptr;
        case MM_RDRAM_DRAM:
            return (u8*)m_rdram_ptr;
        case MM_DD_ROM:
            return (u8*)m_dd_ptr;
    };

    DebugMessage(M64MSG_ERROR, "UNKNOWN MEMORY TYPE: %d\n", type);
    abort();
    return 0;
}

void vr4300_jitter_uninitialize_fastmem(void)
{
    // TODO
}

void VR4300_Jitter::ValidBlockSet(uint32_t address)
{
    m_valid_block_ptr[address / 4] = 1;
}

void VR4300_Jitter::ValidBlockUnset(uint32_t address)
{
    m_valid_block_ptr[address / 4] = 0;
}

void VR4300_Jitter::ValidBlockUnsetRange(uint32_t address, u32 length)
{
    memset(&m_valid_block_ptr[address / 4], 0, length);
}

bool VR4300_Jitter::ValidBlockIsSet(uint32_t address)
{
    return m_valid_block_ptr[address / 4];
}

void VR4300_Jitter::ValidBlockSetVirtual(uint32_t address)
{
    m_valid_virtual_block[address >> 12] = 1;
}

void VR4300_Jitter::ValidBlockUnsetVirtual(uint32_t address)
{
    m_valid_virtual_block[address >> 12] = 0;
}

bool VR4300_Jitter::ValidBlockIsSetVirtual(uint32_t address)
{
    return m_valid_virtual_block[address >> 12];
}

bool VR4300_Jitter::ValidBlockIsSetVirtualIdx(uint32_t address_idx)
{
    return m_valid_virtual_block[address_idx];
}

void VR4300_Jitter::ReleaseCodeSpace(uint8_t *near_begin, uint8_t *near_end, uint8_t *far_begin, uint8_t *far_end)
{
    if (near_begin != near_end)
        m_ranges_to_free_on_next_codegen_near.emplace_back(near_begin, near_end);
    if (far_begin != far_end)
        m_ranges_to_free_on_next_codegen_far.emplace_back(far_begin, far_end);
}

void VR4300_Jitter::WriteDestroyBlock(uint8_t *host_entry)
{
    Gen::XEmitter emit(host_entry, host_entry + 1);
    emit.INT3();
}

