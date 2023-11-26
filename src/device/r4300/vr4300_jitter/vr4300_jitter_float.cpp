/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_float.cpp                                   *
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
#include "device/r4300/fpu.h"
#include "ConstantPool.h"
#include <cmath>

using namespace Gen;

alignas(16) static const __m128i double_low_bits = _mm_set_epi64x(0, 0x00000000ffffffff);
alignas(16) static const __m128i double_low_bits_no_sign = _mm_set_epi64x(0, 0x000000007fffffff);
alignas(16) static const __m128i double_high_bits = _mm_set_epi64x(0, 0xffffffff00000000);
alignas(16) static const __m128i double_high_bits_no_sign = _mm_set_epi64x(0, 0x7fffffff00000000);
alignas(16) static const __m128i double_low_only_sign_bit = _mm_set_epi64x(0, 0x0000000080000000);
alignas(16) static const __m128i double_only_sign_bit = _mm_set_epi64x(0, 0x8000000000000000);
alignas(16) static const __m128i double_no_sign = _mm_set_epi64x(0, 0x7fffffffffffffff);

extern std::unordered_map<u8*, BackPatchInfo> m_back_patch_info;

#define PERFORM_FLOAT_OPERATION_WITH_ROUNDING_MODE(round_code, trunc_code, ceil_code, floor_code) \
    do { \
        MOV(64, R(RSCRATCH), (HOTSTATE_VAR(cp1_fcr31))); \
        AND(32, R(RSCRATCH), Imm32(3)); \
        SHL(32, R(RSCRATCH), Imm8(3)); \
        u8 *mov_loc = GetWritableCodePtr(); \
        MOV(64, R(RSCRATCH2), ImmPtr((void*)0x12345678DEADC0DE)); \
        ADD(64, R(RSCRATCH2), R(RSCRATCH)); \
        JMPptr(MatR(RSCRATCH2)); \
        u8 *ptrs_begin = GetWritableCodePtr(); \
        Write64(0); \
        Write64(0); \
        Write64(0); \
        Write64(0); \
        u8 *ptrs_end = GetWritableCodePtr(); \
        u8 *round_loc = GetWritableCodePtr(); \
        round_code \
        FixupBranch exit_from_round = J(); \
        u8 *trunc_loc = GetWritableCodePtr(); \
        trunc_code \
        FixupBranch exit_from_trunc = J(); \
        u8 *ceil_loc = GetWritableCodePtr(); \
        ceil_code \
        FixupBranch exit_from_ceil = J(); \
        u8 *floor_loc = GetWritableCodePtr(); \
        floor_code \
        SetJumpTarget(exit_from_round); \
        SetJumpTarget(exit_from_trunc); \
        SetJumpTarget(exit_from_ceil); \
        Gen::XEmitter emitter(mov_loc, ptrs_end); \
        emitter.MOV(64, R(RSCRATCH2), ImmPtr(ptrs_begin)); \
        emitter.ADD(64, R(RSCRATCH2), R(RSCRATCH)); \
        emitter.JMPptr(MatR(RSCRATCH2)); \
        emitter.Write64((u64)round_loc); \
        emitter.Write64((u64)trunc_loc); \
        emitter.Write64((u64)ceil_loc); \
        emitter.Write64((u64)floor_loc); \
    } while(0)

extern void write_word_from_dynarec(u32 lsaddr, u32 value);
extern void write_dword_from_dynarec(u32 lsaddr, u64 value);
extern u32 read_word_from_dynarec(u32 lsaddr);
extern u64 read_dword_from_dynarec(u32 lsaddr);

void VR4300_Jitter::compile_fpu_reset_cause(struct jit_instr *op)
{
// TODO: Turn ACCURATE_FPU_BEHAVIOR into a runtime option.
#ifdef ACCURATE_FPU_BEHAVIOR
    AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CAUSE_BITS));
#endif
}

void VR4300_Jitter::compile_fpu_reset_exceptions(struct jit_instr *op)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionC(feclearexcept, FE_ALL_EXCEPT);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::compile_fpu_check_exceptions(struct jit_instr *op)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    ABI_CallFunctionC(fetestexcept, FE_ALL_EXCEPT);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    if (RSCRATCH != ABI_RETURN) {
        MOV(32, R(RSCRATCH), R(ABI_RETURN));
    }
    //AND(32, R(RSCRATCH), Imm32(FE_ALL_EXCEPT)); // this is done in glibc
#define TEST_AND_SET_EXCEPTION_2(exception_host, exception_target) \
    do { \
        TEST(32, R(RSCRATCH), Imm32(FE_ ##exception_host)); \
        FixupBranch no_ ##exception_host = J_CC(CC_Z); \
        OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_ ## exception_target ## _BIT)); \
        OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_ ## exception_target ## _BIT)); \
        SetJumpTarget(no_ ##exception_host); \
    } while (0)

#define TEST_AND_SET_EXCEPTION(exception) \
    TEST_AND_SET_EXCEPTION_2(exception, exception)

    TEST_AND_SET_EXCEPTION(DIVBYZERO);
    TEST_AND_SET_EXCEPTION(UNDERFLOW);
    TEST_AND_SET_EXCEPTION(OVERFLOW);
    TEST_AND_SET_EXCEPTION_2(INVALID, INVALIDOP);
    // TODO: exceptions
#endif
}

void vr4300_jitter_check_input_float(uint32_t* fcr31, float value)
{
    switch (fpclassify(value))
    {
    default:
    case FP_SUBNORMAL: // TODO
        return;
    case FP_NAN:
        (*fcr31) |= FCR31_CAUSE_INVALIDOP_BIT;
        (*fcr31) |= FCR31_FLAG_INVALIDOP_BIT;
        break;
    }
}

void vr4300_jitter_check_input_double(uint32_t* fcr31, double value)
{
    switch (fpclassify(value))
    {
    default:
    case FP_SUBNORMAL: // TODO
        return;
    case FP_NAN:
        (*fcr31) |= FCR31_CAUSE_INVALIDOP_BIT;
        (*fcr31) |= FCR31_FLAG_INVALIDOP_BIT;
        break;
    }
}

void vr4300_jitter_check_output_float(uint32_t* fcr31, const float value)
{
    switch (fpclassify(value))
    {
    case FP_SUBNORMAL:
        (*fcr31) |= FCR31_CAUSE_UNDERFLOW_BIT;
        (*fcr31) |= FCR31_FLAG_UNDERFLOW_BIT;
        (*fcr31) |= FCR31_CAUSE_INEXACT_BIT;
        (*fcr31) |= FCR31_FLAG_INEXACT_BIT;
        break;

    case FP_NAN:
        break;

    default:
        break;
    }
}

void vr4300_jitter_check_output_double(uint32_t* fcr31, const double value)
{
    switch (fpclassify(value))
    {
    case FP_SUBNORMAL:
        (*fcr31) |= FCR31_CAUSE_UNDERFLOW_BIT;
        (*fcr31) |= FCR31_FLAG_UNDERFLOW_BIT;
        (*fcr31) |= FCR31_CAUSE_INEXACT_BIT;
        (*fcr31) |= FCR31_FLAG_INEXACT_BIT;
        break;

    case FP_NAN:
        break;

    default:
        break;
    }
}

void VR4300_Jitter::compile_fpu_check_input_float(struct jit_instr *op, const RCX64Reg &reg)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    // TODO: fast implementation using UCOMISD etc
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    registers_in_use[XMM0 + 16] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    LEA(64, ABI_PARAM1, HOTSTATE_VAR(cp1_fcr31));
    MOVD_xmm(R(ABI_PARAM2), reg);
    ABI_CallFunction(vr4300_jitter_check_input_float);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::compile_fpu_check_input_double(struct jit_instr *op, const RCX64Reg &reg)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    // TODO: fast implementation using UCOMISD etc
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    registers_in_use[XMM0 + 16] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    LEA(64, ABI_PARAM1, HOTSTATE_VAR(cp1_fcr31));
    MOVQ_xmm(R(ABI_PARAM2), reg);
    ABI_CallFunction(vr4300_jitter_check_input_double);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::compile_fpu_check_input_float(struct jit_instr *op, const RCOpArg &input)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    // TODO: fast implementation using UCOMISD etc
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    registers_in_use[XMM0 + 16] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    LEA(64, ABI_PARAM1, HOTSTATE_VAR(cp1_fcr31));
    if (!input.IsSimpleReg(XMM0)) MOVSS(XMM0, input);
    MOVD_xmm(R(ABI_PARAM2), XMM0);
    ABI_CallFunction(vr4300_jitter_check_input_float);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::compile_fpu_check_input_double(struct jit_instr *op, const RCOpArg &input)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    // TODO: fast implementation using UCOMISD etc
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    registers_in_use[XMM0 + 16] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    LEA(64, ABI_PARAM1, HOTSTATE_VAR(cp1_fcr31));
    if (!input.IsSimpleReg(XMM0)) MOVSS(XMM0, input);
    MOVQ_xmm(R(ABI_PARAM2), XMM0);
    ABI_CallFunction(vr4300_jitter_check_input_double);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::compile_fpu_check_output_float(struct jit_instr *op, const RCX64Reg &reg)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    // TODO: fast implementation using UCOMISD etc
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    registers_in_use[XMM0 + 16] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    LEA(64, ABI_PARAM1, HOTSTATE_VAR(cp1_fcr31));
    MOVD_xmm(R(ABI_PARAM2), reg);
    ABI_CallFunction(vr4300_jitter_check_output_float);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::compile_fpu_check_output_double(struct jit_instr *op, const RCX64Reg &reg)
{
#ifdef ACCURATE_FPU_BEHAVIOR
    // TODO: fast implementation using UCOMISD etc
    BitSet32 registers_in_use = caller_saved_registers_in_use();
    registers_in_use[XMM0 + 16] = true;
    ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
    LEA(64, ABI_PARAM1, HOTSTATE_VAR(cp1_fcr31));
    MOVQ_xmm(R(ABI_PARAM2), reg);
    ABI_CallFunction(vr4300_jitter_check_output_double);
    ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
#endif
}

void VR4300_Jitter::recompile_TRUNC_W_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCOpArg scratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    CVTTSS2SI(scratch.GetSimpleReg(), Rs);
    MOVD_xmm(Rd, scratch);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_FLOOR_W_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCOpArg scratch = m_gpr.Scratch();
    RCX64Reg scratch3 = m_fpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch, scratch3);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    ROUNDSS(scratch3, Rs, 0x1);
    CVTTSS2SI(scratch.GetSimpleReg(), scratch3);
    MOVD_xmm(Rd, scratch);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_FLOOR_W_D(struct jit_instr *op)
{
    if (op->s == op->d) {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        MOVSD(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, RCOpArg::R(XMM0));
        compile_fpu_reset_exceptions(op);

        // floor_w_d, untested
        ROUNDSD(Rd, R(XMM0), 0x1);

        compile_fpu_check_exceptions(op);
    } else {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        compile_fpu_reset_exceptions(op);

        // floor_w_d, untested
        ROUNDSD(Rd, Rs, 0x1);

        compile_fpu_check_exceptions(op);
    }
}

void VR4300_Jitter::recompile_TRUNC_W_D(struct jit_instr *op)
{
    if (op->d == op->s) {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        MOVSD(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, RCOpArg::R(XMM0));
        compile_fpu_reset_exceptions(op);

        CVTTPD2DQ(Rd, R(XMM0));

        compile_fpu_check_exceptions(op);
    } else {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd, Rs);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        compile_fpu_reset_exceptions(op);

        CVTTPD2DQ(Rd, Rs);

        compile_fpu_check_exceptions(op);
    }
}

void VR4300_Jitter::recompile_TRUNC_L_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rd, Rs, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    CVTTSS2SI64(gprscratch, Rs);
    MOVQ_xmm(Rd, R(gprscratch));

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CVT_S_W(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCX64Reg Rd = m_fpr.Bind(op->d, (op->s == op->d) ? RCMode::ReadWrite : RCMode::Write, true);
    RCX64Reg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rd, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_reset_exceptions(op);

    load_cop1_register_to_host_register(op, 32, op->s, gprscratch);

    CVTSI2SS(Rd, R(gprscratch));

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_CVT_S_D(struct jit_instr *op)
{
    if (op->s == op->d) {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        MOVSD(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, RCOpArg::R(XMM0));
        compile_fpu_reset_exceptions(op);

        CVTSD2SS(Rd, R(XMM0));

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    } else {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        compile_fpu_reset_exceptions(op);

        CVTSD2SS(Rd, Rs);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    }
}

void VR4300_Jitter::recompile_CVT_S_L(struct jit_instr *op)
{
    if (op->s == op->d) {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        MOVSD(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_reset_exceptions(op);

        CVTSS2SD(Rd, R(XMM0));

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    } else {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_reset_exceptions(op);

        CVTSS2SD(Rd, Rs);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    }
}

void VR4300_Jitter::recompile_CVT_W_D(struct jit_instr *op)
{
    if (op->s == op->d) {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        MOVSD(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, RCOpArg::R(XMM0));
        compile_fpu_reset_exceptions(op);

        PERFORM_FLOAT_OPERATION_WITH_ROUNDING_MODE({
            // round_w_d, untested
            ROUNDSD(Rd, R(XMM0), 0x0);
        },{
            // trunc_w_d
            CVTTPD2DQ(Rd, R(XMM0));
        },{
            // ceil_w_d, untested
            ROUNDSD(Rd, R(XMM0), 0x2);
        },{
            // floor_w_d, untested
            ROUNDSD(Rd, R(XMM0), 0x1);
        });

        compile_fpu_check_exceptions(op);
    } else {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        compile_fpu_reset_exceptions(op);

        PERFORM_FLOAT_OPERATION_WITH_ROUNDING_MODE({
            // round_w_d, untested
            ROUNDSD(Rd, Rs, 0x0);
        },{
            // trunc_w_d
            CVTTPD2DQ(Rd, Rs);
        },{
            // ceil_w_d, untested
            ROUNDSD(Rd, Rs, 0x2);
        },{
            // floor_w_d, untested
            ROUNDSD(Rd, Rs, 0x1);
        });

        compile_fpu_check_exceptions(op);
    }
}

void VR4300_Jitter::recompile_ROUND_W_D(struct jit_instr *op)
{
    if (op->s == op->d) {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        MOVSD(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, RCOpArg::R(XMM0));
        compile_fpu_reset_exceptions(op);

        // round_w_d, untested
        ROUNDSD(Rd, R(XMM0), 0x0);

        compile_fpu_check_exceptions(op);
    } else {
        VALIDATE_FIN(op, s);
        VALIDATE_FOUT32(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        compile_fpu_reset_exceptions(op);

        // round_w_d, untested
        ROUNDSD(Rd, Rs, 0x0);

        compile_fpu_check_exceptions(op);
    }
}

void VR4300_Jitter::recompile_CEIL_W_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCX64Reg fprscratch = m_fpr.Scratch();
    RegCache::Realize(Rs, Rd, fprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // ceil_w_d, untested
    ROUNDSD(fprscratch, Rs, 0x2);
    MOVSS(Rd, R(fprscratch));

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CEIL_W_S(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // ceil_w_s, untested
    ROUNDSS(Rd, Rs, 0x2);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CEIL_L_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // ceil_l_d, untested
    ROUNDSD(Rd, Rs, 0x2);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CEIL_L_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // ceil_l_s, untested
    ROUNDSS(Rd, Rs, 0x2);
    PAND(Rd, MConst(double_low_bits));

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_C_cond_fmt(struct jit_instr *op, int fmt)
{
    assert(op->has_c);
    assert(op->has_a);
    if (op->a == 0x10 || op->a == 0x14) {
        VALIDATE_FIN32(op, t);
        VALIDATE_FIN32(op, s);
    } else {
        VALIDATE_FIN(op, t);
        VALIDATE_FIN(op, s);
    }

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read, op->a == 0x10 || op->a == 0x14);
    RCX64Reg Rt = m_fpr.Bind(op->t, RCMode::Read, op->a == 0x10 || op->a == 0x14);

    RegCache::Realize(Rs, Rt);

    compile_fpu_reset_cause(op);

    switch (fmt) {
        case 0:
            UCOMISS(Rs, Rt);
            break;
        case 1:
            abort();
            break;
        case 2:
            UCOMISD(Rs, Rt);
            break;
        case 3:
            abort();
            break;
        default:
            abort();
    };

    switch(op->c) {
        case 0: {
            // C.F.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            break;
        }
        case 1: {
            // C.UN.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            leave_farcode();
            break;
        }
        case 2: {
            // C.EQ.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch eq = J_CC(CC_E);

            // neq
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(eq);

            // eq
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 4: {
            // C.OLT.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch less = J_CC(CC_B);

            // greater or equal
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            FixupBranch exit = J();
            SetJumpTarget(less);
            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 5: {
            // C.ULT.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch less = J_CC(CC_B);

            // greater or equal
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(less);

            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 6: {
            // C.OLE.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch less = J_CC(CC_BE);

            // greater or equal
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            FixupBranch exit = J();
            SetJumpTarget(less);

            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            leave_farcode();
            SetJumpTarget(exit);
            break;
        }
        case 7: {
            // C.ULE.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch leq = J_CC(CC_BE);

            // greater
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(leq);

            // leq
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            leave_farcode();
            SetJumpTarget(exit);
            break;
        }
        case 8: {
            // C.SF.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();
            break;
        }
        case 9: {
            // C.NGLE.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();
            break;
        }
            break;
        case 10: {
            // C.SEQ.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch eq = J_CC(CC_E);

            // neq
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(eq);

            // eq
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 11: {
            // C.NGL.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch eq = J_CC(CC_E);

            // neq
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(eq);

            // eq
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 12: {
            // C.LT.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch less = J_CC(CC_B);

            // greater or equal
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(less);

            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 13: {
            // C.NGE.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch less = J_CC(CC_B);

            // greater or equal
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(less);

            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 14: {
            // C.LE.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch leq = J_CC(CC_BE);

            // greater
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(leq);

            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
        case 15: {
            // C.NGT.fmt
            FixupBranch nan = J_CC(CC_P, XEmitter::Jump::Near);

            // here neither is nan
            FixupBranch leq = J_CC(CC_BE);

            // greater
            AND(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(~FCR31_CMP_BIT));
            FixupBranch exit = J();
            SetJumpTarget(leq);

            // less
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));

            switch_to_far_code();
            SetJumpTarget(nan);

            // at least one is nan
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CMP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_INVALIDOP_BIT));
            OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_FLAG_INVALIDOP_BIT));

            leave_farcode();

            SetJumpTarget(exit);
            break;
        }
            break;
        default:
            abort();
    };
}

void VR4300_Jitter::recompile_C_cond_S(struct jit_instr *op)
{
    VR4300_Jitter::recompile_C_cond_fmt(op, 0);
}

void VR4300_Jitter::recompile_C_cond_D(struct jit_instr *op)
{
    VR4300_Jitter::recompile_C_cond_fmt(op, 2);
}

void VR4300_Jitter::recompile_NEG_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);

    if (op->d != op->s) {
        MOVSS(Rd, Rs);
    }

    XORPS(Rd, MConst(double_low_only_sign_bit));

    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_NEG_D(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);

    if (op->d != op->s) {
        MOVSD(Rd, R(Rs));
    }

    XORPD(Rd, MConst(double_only_sign_bit));

    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_ROUND_W_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_reset_exceptions(op);
    compile_fpu_check_input_float(op, Rs);

    ROUNDSS(scratch2, Rs, 0x0);
    CVTSS2SI(gprscratch.GetSimpleReg(), scratch2);
    MOVD_xmm(Rd, gprscratch);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CVT_W_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    PERFORM_FLOAT_OPERATION_WITH_ROUNDING_MODE({
        // round_w_s
        ROUNDSS(scratch2, Rs, 0x0);
        CVTSS2SI(gprscratch.GetSimpleReg(), scratch2);
        MOVD_xmm(Rd, gprscratch);
    },{
        // trunc_w_s
        CVTTSS2SI(gprscratch.GetSimpleReg(), Rs);
        MOVD_xmm(Rd, gprscratch);
    },{
        // ceil_w_s, untested
        ROUNDSS(Rd, Rs, 0x2);
    },{
        // floor_w_s, untested
        ROUNDSS(Rd, Rs, 0x1);
    });

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CVT_L_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    PERFORM_FLOAT_OPERATION_WITH_ROUNDING_MODE({
        // round_l_s, untested
        ROUNDSD(scratch2, Rs, 0x0);
        CVTSS2SI(gprscratch.GetSimpleReg(), scratch2);
        MOVQ_xmm(Rd, gprscratch);
    },{
        // trunc_l_s, untested
        CVTTSS2SI64(gprscratch.GetSimpleReg(), Rs);
        MOVQ_xmm(Rd, gprscratch);
    },{
        // ceil_l_s, untested
        ROUNDSS(Rd, Rs, 0x2);
    },{
        // floor_l_s, untested
        ROUNDSS(Rd, Rs, 0x1);
    });

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_TRUNC_L_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // trunc_l_d, untested
    CVTTSD2SI(gprscratch.GetSimpleReg(), Rs);
    MOVQ_xmm(Rd, gprscratch);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CVT_L_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    PERFORM_FLOAT_OPERATION_WITH_ROUNDING_MODE({
        // round_l_d, untested
        ROUNDSD(scratch2, Rs, 0x0);
        CVTSD2SI(gprscratch.GetSimpleReg(), scratch2);
        MOVQ_xmm(Rd, gprscratch);
    },{
        // trunc_l_d, untested
        CVTTSD2SI(gprscratch.GetSimpleReg(), Rs);
        MOVQ_xmm(Rd, gprscratch);
    },{
        // ceil_l_d, untested
        ROUNDSD(Rd, Rs, 0x2);
    },{
        // floor_l_d, untested
        ROUNDSD(Rd, Rs, 0x1);
    });

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_ROUND_L_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // round_l_d, untested
    ROUNDSD(scratch2, Rs, 0x0);
    CVTSD2SI(gprscratch.GetSimpleReg(), scratch2);
    MOVQ_xmm(Rd, gprscratch);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_ROUND_L_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    // round_l_s, untested
    ROUNDSD(scratch2, Rs, 0x0);
    CVTSS2SI(gprscratch.GetSimpleReg(), scratch2);
    MOVD_xmm(Rd, gprscratch);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_CVT_D_S(struct jit_instr *op)
{
    if (op->s == op->d) {
        VALIDATE_FIN32(op, s);
        VALIDATE_FOUT(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
        RegCache::Realize(Rs);
        MOVSS(XMM0, Rs);
        Rs.Unlock();

        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
        RegCache::Realize(Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_float(op, RCOpArg::R(XMM0));
        compile_fpu_reset_exceptions(op);

        CVTSS2SD(Rd, R(XMM0));

        compile_fpu_check_output_double(op, Rd);
    } else {
        VALIDATE_FIN32(op, s);
        VALIDATE_FOUT(op, d);

        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_float(op, Rs);
        compile_fpu_reset_exceptions(op);

        CVTSS2SD(Rd, Rs);

        compile_fpu_check_output_double(op, Rd);
    }
}

void VR4300_Jitter::recompile_CVT_D_W(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg scratch = m_gpr.Scratch();
    RegCache::Realize(scratch);

    compile_fpu_reset_cause(op);
    compile_fpu_reset_exceptions(op);

    load_cop1_register_to_host_register(op, 32, op->s, scratch);

    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rd, scratch);

    CVTSI2SD(Rd, R(scratch));

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_CVT_D_L(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg scratch = m_gpr.Scratch();
    RegCache::Realize(scratch);

    compile_fpu_reset_cause(op);
    compile_fpu_reset_exceptions(op);

    load_cop1_register_to_host_register(op, 32, op->s, scratch);

    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rd, scratch);

    CVTSI2SD(Rd, R(scratch));

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_MUL_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FIN32(op, t);
    VALIDATE_FOUT32(op, d);

    compile_fpu_reset_cause(op);

    if (op->d == op->t) {
        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::ReadWrite, true);
        RegCache::Realize(Rd, Rs);

        compile_fpu_check_input_float(op, Rs);
        compile_fpu_reset_exceptions(op);

        MULSS(Rd, Rs);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    } else if (op->d == op->s) {
        RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read, true);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::ReadWrite, true);
        RegCache::Realize(Rt, Rd);

        compile_fpu_check_input_float(op, Rt);
        compile_fpu_reset_exceptions(op);

        MULSS(Rd, Rt);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    } else {
        RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read, true);
        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rt, Rs, Rd);

        compile_fpu_check_input_float(op, Rs);
        compile_fpu_check_input_float(op, Rt);
        compile_fpu_reset_exceptions(op);

        MOVSS(Rd, Rs);
        MULSS(Rd, Rt);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    }
}

void VR4300_Jitter::recompile_MUL_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FIN(op, t);
    VALIDATE_FOUT(op, d);

    RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read);
    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rt, Rs, Rd);

    compile_fpu_reset_cause(op);

    compile_fpu_check_input_double(op, Rs);
    if (op->t != op->s) compile_fpu_check_input_double(op, Rt);
    compile_fpu_reset_exceptions(op);

    if (op->d == op->t) {
        MULSD(Rd, Rs);
    } else if (op->d == op->s) {
        MULSD(Rd, Rt);
    } else {
        MOVSD(Rd, Rs);
        MULSD(Rd, Rt);
    }

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_DIV_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FIN32(op, t);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read, true);
    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rt, Rs, Rd);

    compile_fpu_reset_cause(op);

    compile_fpu_check_input_float(op, Rs);
    if (op->t != op->s) compile_fpu_check_input_float(op, Rt);
    compile_fpu_reset_exceptions(op);

    if (op->t == op->d) {
        RCX64Reg scratch2 = m_fpr.Scratch();
        RegCache::Realize(scratch2);

        MOVSS(scratch2, Rt);

        if (op->d != op->s) {
            MOVSS(Rd, Rs);
        }

        DIVSS(Rd, scratch2);
    } else {
        if (op->d != op->s) {
            MOVSS(Rd, Rs);
        }

        DIVSS(Rd, Rt);
    }

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_DIV_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FIN(op, t);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rt = m_fpr.Bind(op->t, RCMode::Read);
    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rt, Rs, Rd);

    compile_fpu_reset_cause(op);

    compile_fpu_check_input_double(op, Rs);
    if (op->t != op->s) compile_fpu_check_input_double(op, Rt);
    compile_fpu_reset_exceptions(op);

    if (op->d != op->s) {
        MOVSD(Rd, R(Rs));
    }

    DIVSD(Rd, Rt);

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_ADD_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FIN32(op, t);
    VALIDATE_FOUT32(op, d);

    compile_fpu_reset_cause(op);

    if (op->t == op->d) {
        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::ReadWrite, true);
        RegCache::Realize(Rs, Rd);

        compile_fpu_check_input_float(op, Rs);
        if (op->t != op->s) compile_fpu_check_input_float(op, Rd);
        compile_fpu_reset_exceptions(op);

        ADDSS(Rd, Rs);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    } else {
        RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read, true);
        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
        RCX64Reg Rd = m_fpr.Bind(op->d, (op->s == op->d) ? RCMode::ReadWrite : RCMode::Write, true);
        RegCache::Realize(Rt, Rs, Rd);

        compile_fpu_check_input_float(op, Rs);
        if (op->t != op->s) compile_fpu_check_input_float(op, Rt);
        compile_fpu_reset_exceptions(op);

        if (op->s != op->d) {
            MOVSS(Rd, Rs);
        }

        ADDSS(Rd, Rt);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_float(op, Rd);
    }
}

void VR4300_Jitter::recompile_ABS_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    if (op->d != op->s) {
        MOVSS(Rd, Rs);
    }

    PAND(Rd, MConst(double_low_bits_no_sign));

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_ABS_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    if (op->d != op->s) {
        MOVSD(Rd, Rs);
    }

    PAND(Rd, MConst(double_no_sign));

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_SUB_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FIN32(op, t);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read, true);
    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rt, Rs, Rd);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_float(op, Rs);
    if (op->t != op->s) compile_fpu_check_input_float(op, Rt);
    compile_fpu_reset_exceptions(op);

    if (op->t == op->d) {
        RCX64Reg scratch2 = m_fpr.Scratch();
        RegCache::Realize(scratch2);

        MOVSS(scratch2, Rt);

        if (op->d != op->s) {
            MOVSS(Rd, Rs);
        }

        SUBSS(Rd, scratch2);
    } else {
        if (op->d != op->s) {
            MOVSS(Rd, Rs);
        }

        SUBSS(Rd, Rt);
    }

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_SUB_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FIN(op, t);
    VALIDATE_FOUT(op, d);

    RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read);
    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rt, Rs, Rd);

    compile_fpu_reset_cause(op);

    compile_fpu_check_input_double(op, Rs);
    if (op->t != op->s) compile_fpu_check_input_double(op, Rt);
    compile_fpu_reset_exceptions(op);

    if (op->d != op->s) {
        MOVSD(Rd, Rs);
    }

    SUBSD(Rd, Rt);

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_SQRT_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);

    compile_fpu_check_input_float(op, Rs);
    compile_fpu_reset_exceptions(op);

    SQRTSS(Rd, Rs);

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_float(op, Rd);
}

void VR4300_Jitter::recompile_SQRT_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rs, Rd);

    compile_fpu_reset_cause(op);

    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    SQRTSD(Rd, Rs);

    compile_fpu_check_exceptions(op);
    compile_fpu_check_output_double(op, Rd);
}

void VR4300_Jitter::recompile_MOV_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT32(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read, true);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
    RegCache::Realize(Rs, Rd);

    if (op->d != op->s) {
        MOVSS(Rd, Rs);
    }
}

void VR4300_Jitter::recompile_MOV_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RegCache::Realize(Rs, Rd);

    if (op->d != op->s) {
        MOVSD(Rd, Rs);
    }
}

void VR4300_Jitter::recompile_ADD_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FIN(op, t);
    VALIDATE_FOUT(op, d);

    if (op->t == op->d) {
        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::ReadWrite);
        RegCache::Realize(Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        if (op->t != op->s) compile_fpu_check_input_double(op, Rd);
        compile_fpu_reset_exceptions(op);

        ADDSD(Rd, Rs);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_double(op, Rd);
    } else {
        RCOpArg Rt = m_fpr.Use(op->t, RCMode::Read);
        RCOpArg Rs = m_fpr.Use(op->s, RCMode::Read);
        RCX64Reg Rd = m_fpr.Bind(op->d, (op->d == op->s) ? RCMode::ReadWrite : RCMode::Write);
        RegCache::Realize(Rt, Rs, Rd);

        compile_fpu_reset_cause(op);
        compile_fpu_check_input_double(op, Rs);
        if (op->t != op->s) compile_fpu_check_input_double(op, Rt);
        compile_fpu_reset_exceptions(op);

        if (op->d != op->s) {
            MOVSD(Rd, Rs);
        }

        ADDSD(Rd, Rt);

        compile_fpu_check_exceptions(op);
        compile_fpu_check_output_double(op, Rd);
    }
}

void VR4300_Jitter::recompile_FLOOR_L_D(struct jit_instr *op)
{
    VALIDATE_FIN(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // floor_l_d, untested
    ROUNDSD(Rd, Rs, 0x1);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::recompile_FLOOR_L_S(struct jit_instr *op)
{
    VALIDATE_FIN32(op, s);
    VALIDATE_FOUT(op, d);

    RCX64Reg Rs = m_fpr.Bind(op->s, RCMode::Read);
    RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write);
    RCX64Reg scratch2 = m_fpr.Scratch();
    RCOpArg gprscratch = m_gpr.Scratch();
    RegCache::Realize(Rs, Rd, scratch2, gprscratch);

    compile_fpu_reset_cause(op);
    compile_fpu_check_input_double(op, Rs);
    compile_fpu_reset_exceptions(op);

    // floor_l_s, untested
    ROUNDSS(Rd, Rs, 0x1);

    compile_fpu_check_exceptions(op);
}

void VR4300_Jitter::store_host_register_to_cop1_register(struct jit_instr *op, int bits, const RCOpArg &cpu_val, const RCX64Reg &Rt)
{
    if (bits == 32) {
        if (cpu_val.IsImm()) {
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(scratch);

            MOV(32, R(scratch), Imm32(cpu_val.Imm64()));
            MOVD_xmm(Rt, R(scratch));
        } else {
            MOVD_xmm(Rt, cpu_val);
        }
    } else if (bits == 64) {
        if (cpu_val.IsImm()) {
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(scratch);

            if (vr4300_jitter_value_fits_in_32_bit_imm_positive(cpu_val.Imm64())) {
                MOV(32, R(scratch), Imm32(cpu_val.Imm64()));
            } else {
                MOV(64, R(scratch), Imm64(cpu_val.Imm64()));
            }
            MOVQ_xmm(Rt, R(scratch));
        } else {
            MOVQ_xmm(Rt, cpu_val);
        }
    } else abort();
}

void VR4300_Jitter::store_host_register_to_cop1_register(struct jit_instr *op, int bits, const RCOpArg &cpu_val, int reg)
{
    if (bits == 32) {
        RCX64Reg Rt = m_fpr.Bind(reg, RCMode::Write, true);
        RegCache::Realize(Rt);

        store_host_register_to_cop1_register(op, bits, cpu_val, Rt);
    } else if (bits == 64) {
        RCX64Reg Rt = m_fpr.Bind(reg, RCMode::Write);
        RegCache::Realize(Rt);

        store_host_register_to_cop1_register(op, bits, cpu_val, Rt);
    } else abort();
}

void VR4300_Jitter::load_cop1_register_to_host_register(struct jit_instr *op, int bits, const RCOpArg &Rt, const RCX64Reg &target)
{
    if (bits == 32) {
        if (Rt.IsSimpleReg()) {
            MOVD_xmm(R(target), Rt.GetSimpleReg());
            MOVSX(64, 32, target, R(target));
        } else {
            MOVSX(64, 32, target, Rt);
        }
    } else if (bits == 64) {
        if (Rt.IsSimpleReg()) {
            MOVQ_xmm(R(target), Rt.GetSimpleReg());
        } else {
            RCX64Reg fprscratch = m_fpr.Scratch();
            RegCache::Realize(fprscratch);

            MOVSD(fprscratch, Rt);
            MOVQ_xmm(R(target), fprscratch);
        }
    } else abort();
}

void VR4300_Jitter::load_cop1_register_to_host_register(struct jit_instr *op, int bits, int reg, const RCX64Reg &target)
{
    if (bits == 32) {
        RCOpArg Rt = m_fpr.Use(reg, RCMode::Read, true);
        RegCache::Realize(Rt);

        load_cop1_register_to_host_register(op, bits, Rt, target);
    } else if (bits == 64) {
        RCOpArg Rt = m_fpr.Use(reg, RCMode::Read);
        RegCache::Realize(Rt);

        load_cop1_register_to_host_register(op, bits, Rt, target);
    } else abort();
}

void VR4300_Jitter::recompile_SWC1(struct jit_instr *op)
{
    VALIDATE_FIN32(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCX64Reg out_reg = m_gpr.Scratch();
        RegCache::Realize(out_reg);

        load_cop1_register_to_host_register(op, 32, op->t, out_reg);

        if (vr4300_jitter_is_rdram_address(address)) {
            MOV(32, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(out_reg));

#if !NO_FLOAT_INVALIDATE
            compile_invalidate_code_rdram(address);
#endif

            exception_check = false;
        } else {
            abort();
        }
    } else {
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg out_reg = m_gpr.Scratch(), scratch_addr = m_gpr.Scratch(), scratch = m_gpr.Scratch();
        RegCache::Realize(Rb, out_reg, scratch_addr, scratch);

        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;

        if (m_gpr.IsImm(op->b)) {
            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            if (op->f) {
                MOV_sum(32, scratch_addr, Rb, Imm32((u32)(s16)op->f));
            } else {
                MOV(64, R(scratch_addr), Rb);
            }

            AND(32, R(scratch_addr), Imm32(~3));
            memory_location = MRegSum(RDRAM, scratch_addr);
        }

        load_cop1_register_to_host_register(op, 32, op->t, out_reg);

        u8 *mov_address = GetWritableCodePtr();
        MOV(32, memory_location, R(out_reg));

#if !NO_FLOAT_INVALIDATE
        compile_invalidate_code(Rb, (s32)(s16)op->f);
#endif

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();

        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(32, ABI_PARAM1, 32, ABI_PARAM2, Rb, RCOpArg::R(out_reg));
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_word_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        if (is_in_far_code()) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
    }
}

void VR4300_Jitter::recompile_LWC1(struct jit_instr *op)
{
    VALIDATE_FOUT32(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    update_hot_cycles(op);

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        if (vr4300_jitter_is_rdram_address(address)) {
            MOV(32, R(ABI_RETURN), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
            exception_check = false;
        } else {
            abort();
        }
    } else {
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratch_addr = m_gpr.Scratch(), scratch = m_gpr.Scratch();
        RegCache::Realize(Rb, scratch_addr, scratch);

        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;

        if (m_gpr.IsImm(op->b)) {
            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            if (op->f) {
                MOV_sum(32, scratch_addr, Rb, Imm32((u32)(s16)op->f));
            } else {
                MOV(64, R(scratch_addr), Rb);
            }
            AND(32, R(scratch_addr), Imm32(~3));
            memory_location = MRegSum(RDRAM, scratch_addr);
        }

        u8 *mov_address = GetWritableCodePtr();
        MOV(32, R(ABI_RETURN), memory_location);

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        if (!Rb.IsSimpleReg(ABI_PARAM1)) {
            MOV(64, R(ABI_PARAM1), Rb);
        }
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(read_word_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

        FixupBranch near_code = J(XEmitter::Jump::Near);
        switch_to_near_code();
        SetJumpTarget(near_code);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);
    }

    store_host_register_to_cop1_register(op, 32, RCOpArg::R(ABI_RETURN), op->t);
}

void VR4300_Jitter::recompile_LDC1(struct jit_instr *op)
{
    VALIDATE_FOUT(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    // TODO
    update_hot_cycles(op);

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        if (vr4300_jitter_is_rdram_address(address)) {
            MOV(64, R(ABI_RETURN), MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)));
            ROR(64, R(ABI_RETURN), Imm8(32));
            exception_check = false;
        } else {
            abort();
        }
    } else {
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratch_addr = m_gpr.Scratch(), scratch = m_gpr.Scratch();
        RegCache::Realize(Rb, scratch_addr, scratch);

        u8 *code_before = GetWritableCodePtr();

        OpArg memory_location;

        if (m_gpr.IsImm(op->b)) {
            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            if (op->f) {
                MOV_sum(32, scratch_addr, Rb, Imm32((u32)(s16)op->f));
            } else {
                MOV(64, R(scratch_addr), Rb);
            }
            AND(32, R(scratch_addr), Imm32(~3));
            memory_location = MRegSum(RDRAM, scratch_addr);
        }

        u8 *mov_address = GetWritableCodePtr();
        MOV(64, R(ABI_RETURN), memory_location);
        ROR(64, R(ABI_RETURN), Imm8(32));

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        if (!Rb.IsSimpleReg(ABI_PARAM1)) {
            MOV(64, R(ABI_PARAM1), Rb);
        }
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((s32)(s16)op->f));
        ABI_CallFunction(read_dword_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);

        FixupBranch near_code = J(XEmitter::Jump::Near);
        switch_to_near_code();
        SetJumpTarget(near_code);
    }

    if (exception_check) {
        // tlb exception check
        compile_tlb_exception_check(op, true, op->address + 4);
    }

    store_host_register_to_cop1_register(op, 64, RCOpArg::R(ABI_RETURN), op->t);
}

void VR4300_Jitter::recompile_SDC1(struct jit_instr *op)
{
    VALIDATE_FIN(op, t);
    VALIDATE_IN(op, b);
    assert(op->has_f);

    bool exception_check = true;

    u32 base = op->b ? (m_gpr.IsImm(op->b) ? m_gpr.Imm64(op->b) : 0) : 0;
    u32 address = base + (u32)(s16)op->f;
    if (!HOT_STATE->rdram_generate_slowcode && (!op->b || (m_gpr.IsImm(op->b) && vr4300_jitter_is_rdram_address(address)))) {
        RCX64Reg out_reg = m_gpr.Scratch();
        RegCache::Realize(out_reg);

        load_cop1_register_to_host_register(op, 64, op->t, out_reg);

        if (vr4300_jitter_is_rdram_address(address)) {
            ROR(64, R(out_reg), Imm8(32));
            MOV(64, MDisp(RDRAM, vr4300_jitter_rdram_dram_address(address & ~3)), R(out_reg));

#if !NO_FLOAT_INVALIDATE
            compile_invalidate_code_rdram(address);
#endif

            exception_check = false;
        } else {
            abort();
        }
    } else {
        RCOpArg Rb = op->b ? m_gpr.Use(op->b, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg out_reg = m_gpr.Scratch(), scratch_addr = m_gpr.Scratch();
        RegCache::Realize(Rb, out_reg, scratch_addr);

        u8 *code_before = GetWritableCodePtr();

        load_cop1_register_to_host_register(op, 64, op->t, out_reg);

        OpArg memory_location;

        if (m_gpr.IsImm(op->b)) {
            memory_location = MDisp(RDRAM, address & ~3);
        } else {
            if (op->f) {
                MOV_sum(32, scratch_addr, Rb, Imm32((u32)(s16)op->f));
            } else {
                MOV(64, R(scratch_addr), Rb);
            }
            AND(32, R(scratch_addr), Imm32(~3));
            memory_location = MRegSum(RDRAM, scratch_addr);
        }

        ROR(64, R(out_reg), Imm8(32));
        u8 *mov_address = GetWritableCodePtr();
        MOV(64, memory_location, R(out_reg));

#if !NO_FLOAT_INVALIDATE
        compile_invalidate_code(Rb, (s32)(s16)op->f);
#endif

        BackPatchInfo &info = m_back_patch_info[mov_address];
        info.code_after = GetCodePtr();
        info.code_before = code_before;
        info.read = false;

        switch_to_far_code();
        info.farcode = GetCodePtr();

        update_hot_cycles(op);

        BitSet32 registers_in_use = caller_saved_registers_in_use();
        ABI_PushRegistersAndAdjustStack(registers_in_use, 0);
        mov_2(32, ABI_PARAM1, 64, ABI_PARAM2, Rb, RCOpArg::R(out_reg));
        if (op->f) ADD(32, R(ABI_PARAM1), Imm32((u32)(s16)op->f));
        ABI_CallFunction(write_dword_from_dynarec);
        ABI_PopRegistersAndAdjustStack(registers_in_use, 0);
    }

    if (exception_check) {
        if (is_in_far_code()) {
            // tlb exception check
            compile_tlb_exception_check(op, true, op->address + 4);

            FixupBranch near_code = J(XEmitter::Jump::Near);
            switch_to_near_code();
            SetJumpTarget(near_code);
        }
    }
}

void VR4300_Jitter::recompile_MTC1(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_FOUT32(op, d);
    // in vr43xx.pdf the register is s, not d
    // but our decoder puts it into d because MTC0
    // has it in d too.

    if (op->t) {
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
        RegCache::Realize(Rt, Rd);

        store_host_register_to_cop1_register(op, 32, Rt, Rd);
    } else {
        RCX64Reg Rd = m_fpr.Bind(op->d, RCMode::Write, true);
        RegCache::Realize(Rd);
        store_host_register_to_cop1_register(op, 32, RCOpArg::Imm64(0), Rd);
    }
}

void VR4300_Jitter::recompile_DMTC1(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_FOUT(op, d);
    // in vr43xx.pdf the register is s, not d
    // but our decoder puts it into d because MTC0
    // has it in d too.

    if (op->t) {
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
        RegCache::Realize(Rt);

        store_host_register_to_cop1_register(op, 64, Rt, op->d);
    } else {
        store_host_register_to_cop1_register(op, 64, RCOpArg::Imm64(0), op->d);
    }
}

void VR4300_Jitter::recompile_MFC1(struct jit_instr *op)
{
    VALIDATE_FIN32(op, d);
    VALIDATE_OUT(op, t);
    // in vr43xx.pdf the register is s, not d
    // but our decoder puts it into d because MTC0
    // has it in d too.

    if (op->t) {
        RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
        RegCache::Realize(Rt);

        load_cop1_register_to_host_register(op, 32, op->d, Rt);
    }
}

void VR4300_Jitter::recompile_DMFC1(struct jit_instr *op)
{
    VALIDATE_FIN(op, d);
    VALIDATE_OUT(op, t);
    // in vr43xx.pdf the register is s, not d
    // but our decoder puts it into d because MTC0
    // has it in d too.

    if (op->t) {
        RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
        RegCache::Realize(Rt);

        load_cop1_register_to_host_register(op, 64, op->d, Rt);
    }
}

void VR4300_Jitter::recompile_CTC1(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    assert(op->has_d);
    assert((op->d == 31 || op->d == 0));
    // in vr43xx.pdf the register is s, not d
    // but our decoder puts it into d because MTC0
    // has it in d too.

    if (op->d == 31) {
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
        RegCache::Realize(Rt);

        if (!Rt.IsSimpleReg()) {
            if (Rt.IsImm()) {
                MOV(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(Rt.Imm64()));
            } else {
                RCX64Reg scratch = m_gpr.Scratch();
                RegCache::Realize(scratch);

                MOV(32, R(scratch), Rt);
                MOV(32, (HOTSTATE_VAR(cp1_fcr31)), R(scratch));
            }
        } else {
            MOV(32, (HOTSTATE_VAR(cp1_fcr31)), Rt);
        }
    }
}

void VR4300_Jitter::recompile_CFC1(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    assert(op->has_d);
    assert((op->d == 31 || op->d == 0));
    // in vr43xx.pdf the register is s, not d
    // but our decoder puts it into d because MTC0
    // has it in d too.

    RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
    RegCache::Realize(Rt);

    if (op->d == 31) {
        MOVSX(64, 32, Rt, (HOTSTATE_VAR(cp1_fcr31)));
    } else {
        MOVSX(64, 32, Rt, (HOTSTATE_VAR(cp1_fcr0)));
    }
}

void VR4300_Jitter::recompile_DCFC1(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);

    compile_fpu_reset_cause(op);

    OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_UNIMPLOP_BIT));

    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_FPE));
    compile_exception_general(op);
}

void VR4300_Jitter::recompile_DCTC1(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);

    compile_fpu_reset_cause(op);

    OR(32, (HOTSTATE_VAR(cp1_fcr31)), Imm32(FCR31_CAUSE_UNIMPLOP_BIT));

    MOV(32, HOTSTATE_CP0REG(CP0_CAUSE_REG), Imm32(CP0_CAUSE_EXCCODE_FPE));
    compile_exception_general(op);
}

