/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_branch.cpp                                  *
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

#include "RegCache/GPRRegCache.h"
#include "RegCache/FPURegCache.h"

#include "vr4300_jitter_instruction_decoder.h"
#include "Common/x64Emitter.h"
#include "device/r4300/fpu.h"
#include "ConstantPool.h"

using namespace Gen;

void VR4300_Jitter::recompile_BNE(struct jit_instr *op, bool likely)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    assert(op->has_f);

    if (!op->s && !op->t) {
        recompile_delay_slot(&op[1], likely);
        return;
    }

    bool all_imm = false;
    bool branch_taken = false;
    if (op->s != op->t) {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rt = op->t ? m_gpr.UseNoImm(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs, Rt);

        if (Rs.IsImm() && Rt.IsImm()) {
            all_imm = true;
            branch_taken = Rs.Imm64() != Rt.Imm64();
        } else if (Rs.IsImm()) {
            if (vr4300_jitter_value_fits_in_32_bit_imm_positive(Rs.Imm64())) {
                CMP_or_TEST(64, Rt, Imm32(Rs.Imm64()));
            } else {
                MOV(64, R(RSCRATCH2), Rs);
                CMP(64, Rt, R(RSCRATCH2));
            }
        } else if (Rt.IsImm()) {
            if (vr4300_jitter_value_fits_in_32_bit_imm_positive(Rt.Imm64())) {
                CMP_or_TEST(64, Rs, Imm32(Rt.Imm64()));
            } else {
                MOV(64, R(RSCRATCH2), Rt);
                CMP(64, Rs, R(RSCRATCH2));
            }
        } else {
            if (Rt.IsSimpleReg()) {
                CMP(64, Rs, Rt);
            } else if (Rs.IsSimpleReg()) {
                CMP(64, Rs, Rt);
            } else {
                MOV(64, R(RSCRATCH), Rt);
                CMP(64, Rs, R(RSCRATCH));
            }
        }
    } else {
        recompile_delay_slot(&op[1], likely);
        return;
    }

    if (all_imm || branch_taken) {
        if (branch_taken) {
            // not equal
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        } else {
            recompile_delay_slot(&op[1], likely);
            return;
        }
    } else {
        FixupBranch eq = J_CC(CC_E, XEmitter::Jump::Near);

        {
            // not equal
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);
            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        }

        SetJumpTarget(eq);
        // equal

        recompile_delay_slot(&op[1], likely);
    }
}

void VR4300_Jitter::recompile_BNEL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BNE(op, true);
}

void VR4300_Jitter::recompile_BEQ(struct jit_instr *op, bool likely)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    assert(op->has_f);

    bool all_imm = false;
    bool branch_taken = false;
    if (op->s != op->t) {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rt = op->t ? m_gpr.UseNoImm(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs, Rt);

        if (Rs.IsImm() && Rt.IsImm()) {
            all_imm = true;
            branch_taken = Rs.Imm64() == Rt.Imm64();
        } else {
            MOV(64, R(RSCRATCH), Rt);
            if (Rs.IsImm()) {
                if (!op->s || vr4300_jitter_value_fits_in_32_bit_imm_positive(m_gpr.Imm64(op->s))) {
                    CMP_or_TEST(64, R(RSCRATCH), Imm32(op->s ? m_gpr.Imm64(op->s) : 0));
                } else {
                    MOV(64, R(RSCRATCH2), Rs);
                    CMP(64, R(RSCRATCH), R(RSCRATCH2));
                }
            } else {
                CMP(64, Rs, R(RSCRATCH));
            }
        }
    } else {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        recompile_delay_slot(&op[1], false);
        compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

        compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        return;
    }

    if (all_imm || branch_taken) {
        if (branch_taken) {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        } else {
            recompile_delay_slot(&op[1], likely);
            return;
        }
    } else {
        FixupBranch neq = J_CC(CC_NE, XEmitter::Jump::Near);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);
            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        }

        SetJumpTarget(neq);

        recompile_delay_slot(&op[1], likely);
    }
}

void VR4300_Jitter::recompile_BEQL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BEQ(op, true);
}

void VR4300_Jitter::recompile_BC1T(struct jit_instr *op, bool likely)
{
    assert(op->has_f);

    TEST(32, HOTSTATE_VAR(cp1_fcr31), Imm32(FCR31_CMP_BIT));

    FixupBranch skip_branch = J_CC(CC_Z, XEmitter::Jump::Near);

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        recompile_delay_slot(&op[1], false);
        compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

        compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
    }

    SetJumpTarget(skip_branch);

    recompile_delay_slot(&op[1], likely);
}

void VR4300_Jitter::recompile_BC1TL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BC1T(op, true);
}

void VR4300_Jitter::recompile_BC1F(struct jit_instr *op, bool likely)
{
    assert(op->has_f);

    TEST(32, HOTSTATE_VAR(cp1_fcr31), Imm32(FCR31_CMP_BIT));

    FixupBranch skip_branch = J_CC(CC_NZ, XEmitter::Jump::Near);

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        recompile_delay_slot(&op[1], false);
        compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

        compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
    }

    SetJumpTarget(skip_branch);

    recompile_delay_slot(&op[1], likely);
}

void VR4300_Jitter::recompile_BC1FL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BC1F(op, true);
}

void VR4300_Jitter::recompile_BLEZ(struct jit_instr *op, bool likely)
{
    VALIDATE_IN(op, s);
    assert(op->has_f);

    bool all_imm = false;
    bool branch_taken = false;

    {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs);

        if (Rs.IsImm()) {
            all_imm = true;
            branch_taken = true;
        } else {
            CMP_or_TEST(64, Rs, Imm32(0));
        }
    }

    if (all_imm) {
        if (branch_taken) {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        } else {
            recompile_delay_slot(&op[1], likely);
        }
    } else {
        FixupBranch greater_than_0 = J_CC(CC_G, XEmitter::Jump::Near);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);

            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        }

        SetJumpTarget(greater_than_0);

        recompile_delay_slot(&op[1], likely);
    }
}

void VR4300_Jitter::recompile_BLEZL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BLEZ(op, true);
}

void VR4300_Jitter::recompile_BGTZ(struct jit_instr *op, bool likely)
{
    VALIDATE_IN(op, s);
    assert(op->has_f);

    bool all_imm = false;
    bool branch_taken = false;

    {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs);

        if (Rs.IsImm()) {
            all_imm = true;
            branch_taken = false;
        } else {
            CMP_or_TEST(64, Rs, Imm32(0));
        }
    }

    if (all_imm) {
        if (branch_taken) {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        } else {
            recompile_delay_slot(&op[1], likely);
        }
    } else {
        FixupBranch less_than_or_equal_0 = J_CC(CC_LE, XEmitter::Jump::Near);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        }

        SetJumpTarget(less_than_or_equal_0);

        recompile_delay_slot(&op[1], likely);
    }
}

void VR4300_Jitter::recompile_BGTZL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BGTZ(op, true);
}

void VR4300_Jitter::recompile_JALR(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, d);

    u32 imm_addr = 0;
    bool was_imm = false;

#if USE_REG_FOR_STORED_PC
    m_gpr.Steal(RSTOREDPC);
#endif

    {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs);
        if (Rs.IsImm()) {
            imm_addr = Rs.Imm64();
            was_imm = true;
        } else {
            // the checks in the delay slot might change HOT_STATE->pc :(
            if (Rs.IsSimpleReg()) {
#if USE_REG_FOR_STORED_PC
                MOV(32, R(RSTOREDPC), Rs);
#else
                MOV(32, HOTSTATE_VAR(stored_pc), Rs);
#endif
            } else {
                MOV(32, R(RSCRATCH), Rs);
#if USE_REG_FOR_STORED_PC
                MOV(32, R(RSTOREDPC), R(RSCRATCH));
#else
                MOV(32, HOTSTATE_VAR(stored_pc), R(RSCRATCH));
#endif
            }
        }
    }

    m_gpr.SetImmediate64(op->d, (s64)(s32)(op[0].address + 8));

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        recompile_delay_slot(&op[1], false);

#if USE_REG_FOR_STORED_PC
        MOV(32, R(RSCRATCH), R(RSTOREDPC));
        MOV(32, HOTSTATE_VAR(pc), R(RSCRATCH));
#else
        MOV(32, R(RSCRATCH), HOTSTATE_VAR(stored_pc));
        MOV(32, HOTSTATE_VAR(pc), R(RSCRATCH));
#endif
        compile_cycle_count_checks(op, 0, false, false, op[0].address + 8);

        if (!was_imm) {
#if USE_REG_FOR_STORED_PC
            MOV(32, R(RSCRATCH), R(RSTOREDPC));
#else
            MOV(32, R(RSCRATCH), HOTSTATE_VAR(stored_pc));
#endif
            MOV(32, HOTSTATE_VAR(pc), R(RSCRATCH));
        }

        if (op->s == 31) {
            if (!was_imm) {
                compile_goto_dispatcher_destinhotstate_ret();
            } else {
                compile_goto_dispatcher_ret(op, imm_addr);
            }
        } else {
            if (!was_imm) {
                compile_goto_dispatcher_destinhotstate(op, true, true);
            } else {
                compile_goto_dispatcher(op, imm_addr, true, true);
            }
        }
    }

#if USE_REG_FOR_STORED_PC
    m_gpr.Unsteal(RSTOREDPC);
#endif
}

void VR4300_Jitter::recompile_JAL(struct jit_instr *op)
{
    assert(op->has_k);

    uint32_t high_order_bits = ((op[0].address + 4) & 0xf0000000);

    uint32_t target = high_order_bits | ((u32)(op->k & 0x3FFFFFF) << 2);

    m_gpr.SetImmediate64(31, (s64)(s32)(op[0].address + 8));

    {
        RCForkGuard gpr_guard = m_gpr.Fork();
        RCForkGuard fpr_guard = m_fpr.Fork();

        recompile_delay_slot(&op[1], false);

        compile_cycle_count_checks(op, target, false, false, op[0].address + 8);

        compile_goto_dispatcher(op, target, true, true);
    }
}

void VR4300_Jitter::recompile_JR(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    u32 imm_addr = 0;
    bool was_imm = false;

#if USE_REG_FOR_STORED_PC
    m_gpr.Steal(RSTOREDPC);
#endif

    {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs);
        if (Rs.IsImm()) {
            imm_addr = Rs.Imm64();
            was_imm = true;
        } else {
            if (Rs.IsImm()) {
#if USE_REG_FOR_STORED_PC
                MOV(32, R(RSTOREDPC), Imm32(Rs.Imm64()));
#else
                MOV(32, HOTSTATE_VAR(stored_pc), Imm32(Rs.Imm64()));
#endif
            } else if (Rs.IsSimpleReg()) {
#if USE_REG_FOR_STORED_PC
                MOV(32, R(RSTOREDPC), Rs);
#else
                MOV(32, HOTSTATE_VAR(stored_pc), Rs);
#endif
            } else {
                MOV(32, R(RSCRATCH), Rs);
#if USE_REG_FOR_STORED_PC
                MOV(32, R(RSTOREDPC), R(RSCRATCH));
#else
                MOV(32, HOTSTATE_VAR(stored_pc), R(RSCRATCH));
#endif
            }
        }
    }

    RCForkGuard gpr_guard = m_gpr.Fork();
    RCForkGuard fpr_guard = m_fpr.Fork();

    recompile_delay_slot(&op[1], false);
#if USE_REG_FOR_STORED_PC
    MOV(32, R(RSCRATCH), R(RSTOREDPC));
#else
    MOV(32, R(RSCRATCH), HOTSTATE_VAR(stored_pc));
#endif
    MOV(32, HOTSTATE_VAR(pc), R(RSCRATCH));
    compile_cycle_count_checks(op, 0, false, false, op[0].address + 8);

    if (!was_imm) {
#if USE_REG_FOR_STORED_PC
        MOV(32, R(RSCRATCH), R(RSTOREDPC));
#else
        MOV(32, R(RSCRATCH), HOTSTATE_VAR(stored_pc));
#endif
        MOV(32, HOTSTATE_VAR(pc), R(RSCRATCH));
    }

    if (op->s == 31) {
        if (!was_imm) {
            compile_goto_dispatcher_destinhotstate_ret();
        } else {
            compile_goto_dispatcher_ret(op, imm_addr);
        }
    } else {
        if (!was_imm) {
            compile_goto_dispatcher_destinhotstate(op, false, false);
        } else {
            compile_goto_dispatcher(op, imm_addr, false, false);
        }
    }

#if USE_REG_FOR_STORED_PC
    m_gpr.Unsteal(RSTOREDPC);
#endif
}

void VR4300_Jitter::recompile_J(struct jit_instr *op)
{
    assert(op->has_k);

    uint32_t high_order_bits = (op[0].address + 4) & 0xf0000000;

    uint32_t target = high_order_bits | ((u32)(op->k & 0x3FFFFFF) << 2);

    RCForkGuard gpr_guard = m_gpr.Fork();
    RCForkGuard fpr_guard = m_fpr.Fork();

    recompile_delay_slot(&op[1], false);
    compile_cycle_count_checks(op, target, false, false, op[0].address + 8);

    compile_goto_dispatcher(op, target, false, false);
}

void VR4300_Jitter::recompile_BGEZ(struct jit_instr *op, bool likely, bool link)
{
    VALIDATE_IN(op, s);
    assert(op->has_f);

    if (link) {
        m_gpr.SetImmediate64(31, (s64)(s32)(op[0].address + 8));
    }

    bool all_imm = false;
    bool branch_taken = false;

    {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs);

        if (Rs.IsImm()) {
            all_imm = true;
            branch_taken = true;
        } else {
            CMP_or_TEST(64, Rs, Imm32(0));
        }
    }

    if (all_imm) {
        if (branch_taken) {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        } else {
            recompile_delay_slot(&op[1], likely);
        }
    } else {
        FixupBranch less_than_0 = J_CC(CC_L, XEmitter::Jump::Near);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), link, link);
        }

        SetJumpTarget(less_than_0);

        recompile_delay_slot(&op[1], likely);
    }
}

void VR4300_Jitter::recompile_BGEZALL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BGEZ(op, true, true);
}

void VR4300_Jitter::recompile_BGEZAL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BGEZ(op, false, true);
}

void VR4300_Jitter::recompile_BGEZL(struct jit_instr *op)
{
    VR4300_Jitter::recompile_BGEZ(op, true);
}

void VR4300_Jitter::recompile_BLTZ(struct jit_instr *op, bool likely, bool link)
{
    VALIDATE_IN(op, s);
    assert(op->has_f);

    bool all_imm = false;
    bool branch_taken = false;

    if (link) {
        m_gpr.SetImmediate64(31, (s64)(s32)(op[0].address + 8));
    }

    {
        RCOpArg Rs = op->s ? m_gpr.UseNoImm(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RegCache::Realize(Rs);

        if (Rs.IsImm()) {
            all_imm = true;
            branch_taken = false;
        } else {
            CMP_or_TEST(64, Rs, Imm32(0));
        }
    }

    if (all_imm) {
        if (branch_taken) {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false);
        } else {
            recompile_delay_slot(&op[1], likely);
        }
    } else {
        FixupBranch not_less_than_0 = J_CC(CC_GE, XEmitter::Jump::Near);

        {
            RCForkGuard gpr_guard = m_gpr.Fork();
            RCForkGuard fpr_guard = m_fpr.Fork();

            recompile_delay_slot(&op[1], false);
            compile_cycle_count_checks(op, op[0].address + (s32)((s16)(op->f + 1) * 4), false, false, op[0].address + 8);

            compile_goto_dispatcher(op, op[0].address + (s32)((s16)(op->f + 1) * 4), link, link);
        }

        SetJumpTarget(not_less_than_0);

        recompile_delay_slot(&op[1], likely);
    }
}

void VR4300_Jitter::recompile_BLTZL(struct jit_instr *op)
{
    return recompile_BLTZ(op, true);
}

void VR4300_Jitter::recompile_BLTZALL(struct jit_instr *op)
{
    return recompile_BLTZ(op, true, true);
}

void VR4300_Jitter::recompile_BLTZAL(struct jit_instr *op)
{
    return recompile_BLTZ(op, false, true);
}

