/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_integer.cpp                                 *
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

using namespace Gen;

static u64 Add(u64 a, u64 b)
{
  return a + b;
}

static u32 Add32(u32 a, u32 b)
{
  return a + b;
}

static u64 Sub(u64 a, u64 b)
{
  return a - b;
}

static u64 Or(u64 a, u64 b)
{
  return a | b;
}

static u64 Nor(u64 a, u64 b)
{
  return ~(a | b);
}

static u64 And(u64 a, u64 b)
{
  return a & b;
}

static u64 Xor(u64 a, u64 b)
{
  return a ^ b;
}

#define perform_for_64bit_imm(operation, target, imm, scratch) \
    do { \
        if (vr4300_jitter_value_fits_in_32_bit_imm_positive(imm)) { \
            operation(64, target, Imm32((u32)imm)); \
        } else { \
            MOV(64, R(scratch), Imm64(imm)); \
            operation(64, target, R(scratch)); \
        } \
    } while (0)

#define perform_for_64bit_imm_target32(operation, target, imm, scratch) \
    do { \
        operation(32, target, Imm32((u32)imm)); \
    } while (0)

void VR4300_Jitter::recompile_ADD(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        u32 (*doop)(u32, u32) = Add32;

        if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
            m_gpr.SetImmediate64(op->d, (s64)(s32)doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d || op->s == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rs, Rt, Rd);

            if (op->t != op->d && op->s != op->d) {
                MOV(64, Rd, Rs);
                if (Rt.IsImm()) {
                    ADD(32, Rd, Imm32(Rt.Imm64()));
                } else {
                    ADD(32, Rd, Rt);
                }
            } else {
                if (op->t == op->d) {
                    if (Rs.IsImm()) {
                        ADD(32, Rd, Imm32(Rs.Imm64()));
                    } else {
                        ADD(32, Rd, Rs);
                    }
                } else {
                    if (Rt.IsImm()) {
                        ADD(32, Rd, Imm32(Rt.Imm64()));
                    } else {
                        ADD(32, Rd, Rt);
                    }
                }
            }

            MOVSX(64, 32, Rd, Rd);
        }
    }
}

void VR4300_Jitter::recompile_DADD(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        u64 (*doop)(u64, u64) = Add;

        if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
            m_gpr.SetImmediate64(op->d, (s64)doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d || op->s == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rs, Rt, Rd);

            if (op->t != op->d && op->s != op->d) {
                MOV(64, Rd, Rs);
                if (Rt.IsImm()) {
                    perform_for_64bit_imm(ADD, Rd, Rt.Imm64(), RSCRATCH);
                } else {
                    ADD(64, Rd, Rt);
                }
            } else {
                if (op->t == op->d) {
                    if (Rs.IsImm()) {
                        perform_for_64bit_imm(ADD, Rd, Rs.Imm64(), RSCRATCH);
                    } else {
                        ADD(64, Rd, Rs);
                    }
                } else {
                    if (Rt.IsImm()) {
                        perform_for_64bit_imm(ADD, Rd, Rt.Imm64(), RSCRATCH);
                    } else {
                        ADD(64, Rd, Rt);
                    }
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_DADDU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_DADD(op);
}

void VR4300_Jitter::recompile_OR(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        u64 (*doop)(u64, u64) = Or;

        if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
            m_gpr.SetImmediate64(op->d, doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            if (!op->s && op->t) {
                if (op->t != op->d) {
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt);

                    MOV(64, Rd, Rt);
                }
            } else if (!op->t && op->s) {
                if (op->s != op->d) {
                    RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rs);

                    MOV(64, Rd, Rs);
                }
            } else {
                if (op->s == op->d) {
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd, Rt);

                    if (Rt.IsImm()) {
                        perform_for_64bit_imm(OR, Rd, Rt.Imm64(), RSCRATCH);
                    } else {
                        OR(64, Rd, Rt);
                    }
                } else if (op->t == op->d) {
                    RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd, Rs);

                    if (Rs.IsImm()) {
                        perform_for_64bit_imm(OR, Rd, Rs.Imm64(), RSCRATCH);
                    } else {
                        OR(64, Rd, Rs);
                    }
                } else {
                    RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt, Rs);

                    MOV(64, Rd, Rt);
                    if (Rs.IsImm()) {
                        perform_for_64bit_imm(OR, Rd, Rs.Imm64(), RSCRATCH);
                    } else {
                        OR(64, Rd, Rs);
                    }
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_NOR(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        u64 (*doop)(u64, u64) = Nor;

        if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
            m_gpr.SetImmediate64(op->d, doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->s == op->d || op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rd, Rt, Rs);

            if (op->s == op->d) {
                if (Rt.IsImm()) {
                    perform_for_64bit_imm(OR, Rd, Rt.Imm64(), RSCRATCH);
                } else {
                    OR(64, Rd, Rt);
                }
                NOT(64, Rd);
            } else if (op->t == op->d) {
                if (Rs.IsImm()) {
                    perform_for_64bit_imm(OR, Rd, Rs.Imm64(), RSCRATCH);
                } else {
                    OR(64, Rd, Rs);
                }
                NOT(64, Rd);
            } else {
                MOV(64, Rd, Rt);

                if (Rs.IsImm()) {
                    perform_for_64bit_imm(OR, Rd, Rs.Imm64(), RSCRATCH);
                } else {
                    OR(64, Rd, Rs);
                }
                NOT(64, Rd);
            }
        }
    }
}

void VR4300_Jitter::recompile_AND(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        u64 (*doop)(u64, u64) = And;

        if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
            m_gpr.SetImmediate64(op->d, doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            if (op->s == op->d) {
                RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                RegCache::Realize(Rd, Rt);

                if (Rt.IsImm()) {
                    perform_for_64bit_imm(AND, Rd, Rt.Imm64(), RSCRATCH);
                } else {
                    AND(64, Rd, Rt);
                }
            } else if (op->t == op->d) {
                RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
                RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                RegCache::Realize(Rd, Rs);

                if (Rs.IsImm()) {
                    perform_for_64bit_imm(AND, Rd, Rs.Imm64(), RSCRATCH);
                } else {
                    AND(64, Rd, Rs);
                }
            } else {
                RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
                RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
                RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                RegCache::Realize(Rd, Rt, Rs);

                MOV(64, Rd, Rt);
                if (Rs.IsImm()) {
                    perform_for_64bit_imm(AND, Rd, Rs.Imm64(), RSCRATCH);
                } else {
                    AND(64, Rd, Rs);
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_DADDI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);
    assert(op->has_k);

    u64 (*doop)(u64, u64) = Add;

    if (op->t != 0) {
        if (m_gpr.IsImm(op->s) || !op->s) {
            m_gpr.SetImmediate64(op->t, doop(op->s ? m_gpr.Imm64(op->s) : 0, (s64)(s32)(s16)op->k));
        } else {
            if (op->s != op->t) {
                RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
                RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
                RegCache::Realize(Rs, Rt);

                MOV(64, Rt, Rs);
                perform_for_64bit_imm(ADD, Rt, (s64)(s32)(s16)op->k, RSCRATCH);
            } else {
                RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
                RegCache::Realize(Rt);

                perform_for_64bit_imm(ADD, Rt, (s64)(s32)(s16)op->k, RSCRATCH);
            }
        }
    }
}

void VR4300_Jitter::recompile_DADDIU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_DADDI(op);
}

void VR4300_Jitter::recompile_ADDI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);
    assert(op->has_k);

    u32 (*doop)(u32, u32) = Add32;

    if (op->t != 0) {
        if (m_gpr.IsImm(op->s) || !op->s) {
            m_gpr.SetImmediate64(op->t, (s64)(s32)doop((u32)(op->s ? (u32)m_gpr.Imm64(op->s) : 0), (u32)(s16)op->k));
        } else {
            if (op->s != op->t) {
                RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
                RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
                RegCache::Realize(Rs, Rt);

                MOV(64, Rt, Rs);
                ADD(32, Rt, Imm32((u32)(s16)op->k));
                MOVSX(64, 32, Rt, Rt);
            } else {
                RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
                RegCache::Realize(Rt);

                ADD(32, Rt, Imm32((u32)(s16)op->k));
                MOVSX(64, 32, Rt, Rt);
            }
        }
    }
}

void VR4300_Jitter::recompile_ADDU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_ADD(op);
}

void VR4300_Jitter::recompile_SUB(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    u64 (*doop)(u64, u64) = Sub;

    if (op->d != 0) {
        bool rt_imm = false;
        u64 rt_imm_value = 0;
        if (!op->t || m_gpr.IsImm(op->t)) {
            rt_imm = true;
            rt_imm_value = op->t ? m_gpr.Imm64(op->t) : 0;
        }

        if ((m_gpr.IsImm(op->s) || !op->s) && (m_gpr.IsImm(op->t) || !op->t)) {
            m_gpr.SetImmediate64(op->d, (s64)(s32)doop(op->s ? (u32)m_gpr.Imm64(op->s) : 0, op->t ? (u32)m_gpr.Imm64(op->t) : 0));
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d || op->s == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rd, Rs, Rt);
            if (op->t != op->d && op->s != op->d) {
                MOV(64, Rd, Rs);
                if (rt_imm) {
                    SUB(32, Rd, Imm32(rt_imm_value));
                } else {
                    SUB(32, Rd, Rt);
                }
            } else {
                if (op->t == op->d) {
                    if (rt_imm) {
                        MOV(64, Rd, Rs);
                        SUB(32, Rd, Imm32(rt_imm_value));
                    } else if (op->s != op->d) {
                        MOV(64, R(RSCRATCH), Rd);
                        MOV(64, Rd, Rs);
                        SUB(32, Rd, R(RSCRATCH));
                    } else {
                        MOV(64, Rd, Imm32(0));
                    }
                } else {
                    if (rt_imm) {
                        SUB(32, Rd, Imm32(rt_imm_value));
                    } else {
                        SUB(32, Rd, Rt);
                    }
                }
            }

            MOVSX(64, 32, Rd, Rd);
        }
    }
}

void VR4300_Jitter::recompile_DSUB(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    u64 (*doop)(u64, u64) = Sub;

    if (op->d != 0) {
        bool rt_imm = false;
        u64 rt_imm_value = 0;
        if (!op->t || m_gpr.IsImm(op->t)) {
            rt_imm = true;
            rt_imm_value = op->t ? m_gpr.Imm64(op->t) : 0;
        }

        if ((m_gpr.IsImm(op->s) || !op->s) && (m_gpr.IsImm(op->t) || !op->t)) {
            m_gpr.SetImmediate64(op->d, doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d || op->s == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rd, Rs, Rt);
            if (op->t != op->d && op->s != op->d) {
                MOV(64, Rd, Rs);
                if (rt_imm) {
                    perform_for_64bit_imm(SUB, Rd, rt_imm_value, RSCRATCH);
                } else {
                    SUB(64, Rd, Rt);
                }
            } else {
                if (op->t == op->d) {
                    if (rt_imm) {
                        MOV(64, Rd, Rs);
                        perform_for_64bit_imm(SUB, Rd, rt_imm_value, RSCRATCH);
                    } else if (op->s != op->d) {
                        MOV(64, R(RSCRATCH), Rd);
                        MOV(64, Rd, Rs);
                        SUB(64, Rd, R(RSCRATCH));
                    } else {
                        MOV(64, Rd, Imm32(0));
                    }
                } else {
                    if (rt_imm) {
                        perform_for_64bit_imm(SUB, Rd, rt_imm_value, RSCRATCH);
                    } else {
                        SUB(64, Rd, Rt);
                    }
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_DSUBU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_DSUB(op);
}

void VR4300_Jitter::recompile_SUBU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_SUB(op);
}

void VR4300_Jitter::recompile_ORI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);

    u64 (*doop)(u64, u64) = Or;

    if (op->t != 0) {
        if (m_gpr.IsImm(op->s) || !op->s) {
            m_gpr.SetImmediate64(op->t, doop((u16)op->k, op->s ? m_gpr.Imm64(op->s) : 0));
        } else {
            RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
            RCX64Reg Rt = m_gpr.Bind(op->t, (op->s == op->t) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rs, Rt);
            if (op->s != op->t) {
                MOV(64, Rt, Rs);
            }
            OR(64, Rt, Imm32((u16)op->k));
        }
    }
}

void VR4300_Jitter::recompile_ADDIU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_ADDI(op);
}

void VR4300_Jitter::recompile_ANDI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);

    assert(op->has_k);

    u64 (*doop)(u64, u64) = And;

    if (op->t != 0) {
        if (m_gpr.IsImm(op->s) || !op->s) {
            m_gpr.SetImmediate64(op->t, doop(op->s ? m_gpr.Imm64(op->s) : 0, (u16)op->k));
        } else {
            if (op->s != op->t) {
                RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
                RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
                RegCache::Realize(Rs, Rt);

                MOV(64, Rt, Rs);
                AND(32, Rt, Imm32((u16)(op->k)));
            } else {
                RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::ReadWrite);
                RegCache::Realize(Rt);

                AND(32, Rt, Imm32((u16)(op->k)));
            }
        }
    }
}

void VR4300_Jitter::recompile_XOR(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        u64 (*doop)(u64, u64) = Xor;

        if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
            m_gpr.SetImmediate64(op->d, doop(op->s ? m_gpr.Imm64(op->s) : 0, op->t ? m_gpr.Imm64(op->t) : 0));
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->s == op->d || op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rs, Rt, Rd);
            if (op->s != op->d && op->t != op->d) {
                MOV(64, Rd, Rs);
                if (Rt.IsImm()) {
                    perform_for_64bit_imm(XOR, Rd, Rt.Imm64(), RSCRATCH);
                } else {
                    XOR(64, Rd, Rt);
                }
            } else {
                if (op->s == op->d) {
                    if (Rt.IsImm()) {
                        perform_for_64bit_imm(XOR, Rd, Rt.Imm64(), RSCRATCH);
                    } else {
                        XOR(64, Rd, Rt);
                    }
                } else if (op->t == op->d) {
                    if (Rs.IsImm()) {
                        perform_for_64bit_imm(XOR, Rd, Rt.Imm64(), RSCRATCH);
                    } else {
                        XOR(64, Rd, Rs);
                    }
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_XORI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);
    assert(op->has_k);

    if (op->t != 0) {
        u64 (*doop)(u64, u64) = Xor;

        if (!op->s || m_gpr.IsImm(op->s)) {
            m_gpr.SetImmediate64(op->t, doop(op->s ? m_gpr.Imm64(op->s) : 0, (u16)op->k));
        } else {
            RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
            RCX64Reg Rt = m_gpr.Bind(op->t, (op->s == op->t) ? RCMode::ReadWrite : RCMode::Write);
            RegCache::Realize(Rs, Rt);
            if (op->s != op->t) {
                MOV(64, Rt, Imm32((u16)op->k));
                XOR(64, Rt, Rs);
            } else {
                XOR(64, Rt, Imm32((u16)op->k));
            }
        }
    }
}

void VR4300_Jitter::recompile_LUI(struct jit_instr *op)
{
    VALIDATE_OUT(op, t);
    assert(op->has_k);

    if (op->t != 0) {
        m_gpr.SetImmediate64(op->t, (s64)(s32)(op->k << 16));
    }
}

void VR4300_Jitter::recompile_SLTI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);
    assert(op->has_k);

    if (op->t != 0) {
        if (!op->s || m_gpr.IsImm(op->s)) {
            s64 sval = (op->s ? m_gpr.Imm64(op->s) : 0);
            if (sval < (s64)(s32)(s16)op->k) {
                m_gpr.SetImmediate64(op->t, 1);
            } else {
                m_gpr.SetImmediate64(op->t, 0);
            }
        } else {
            RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
            RegCache::Realize(Rt, Rs);

            if (vr4300_jitter_value_fits_in_32_bit_imm_positive((s64)(s32)(s16)op->k)) {
                CMP_or_TEST(64, Rs, Imm32((s64)(s32)(s16)op->k));
            } else {
                MOV(64, R(RSCRATCH), Imm64((s64)(s32)(s16)op->k));
                CMP(64, Rs, R(RSCRATCH));
            }

            // these need to be here in case op->s == op->d or op->t == op->d
            // (if we did them earlier we would overwrite Rd before the check).
            MOV(32, R(RSCRATCH), Imm32(1));
            MOV(32, Rt, Imm32(0));
            CMOVcc(32, Rt, R(RSCRATCH), CC_L);
        }
    }
}

void VR4300_Jitter::recompile_CACHE(struct jit_instr *op)
{
    VALIDATE_IN(op, b);
    assert(op->has_k);
}

void VR4300_Jitter::recompile_SLTIU(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, t);
    assert(op->has_k);

    if (op->t != 0) {
        if (!op->s || m_gpr.IsImm(op->s)) {
            u64 sval = (op->s ? m_gpr.Imm64(op->s) : 0);
            if (sval < (u64)(s64)(s32)(s16)op->k) {
                m_gpr.SetImmediate64(op->t, 1);
            } else {
                m_gpr.SetImmediate64(op->t, 0);
            }
        } else {
            RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
            RCX64Reg Rt = m_gpr.Bind(op->t, RCMode::Write);
            RegCache::Realize(Rt, Rs);

            if (vr4300_jitter_value_fits_in_32_bit_imm_positive((s64)(s32)(s16)op->k)) {
                CMP_or_TEST(64, Rs, Imm32((s64)(s32)(s16)op->k));
            } else {
                MOV(64, R(RSCRATCH), Imm64((s64)(s32)(s16)op->k));
                CMP(64, Rs, R(RSCRATCH));
            }

            // these need to be here in case op->s == op->t
            // (if we did them earlier we would overwrite Rd before the check).
            MOV(32, R(RSCRATCH), Imm32(1));
            MOV(32, Rt, Imm32(0));
            CMOVcc(32, Rt, R(RSCRATCH), CC_NAE);
        }
    }
}

void VR4300_Jitter::recompile_SLT(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            m_gpr.SetImmediate64(op->d, ((s64)(op->s ? m_gpr.Imm64(op->s) : 0) < (s64)(op->t ? m_gpr.Imm64(op->t) : 0)) ? 1 : 0);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
            RegCache::Realize(Rd, Rt, Rs);

            bool rs_rt_swapped = false;
            if (Rs.IsImm() && vr4300_jitter_value_fits_in_32_bit_imm_positive(Rs.Imm64())) {
                CMP_or_TEST(64, Rt, Imm32(Rs.Imm64()));
                rs_rt_swapped = true;
             } else if (Rt.IsImm() && vr4300_jitter_value_fits_in_32_bit_imm_positive(Rt.Imm64())) {
                CMP_or_TEST(64, Rs, Imm32(Rt.Imm64()));
            } else {
                if (Rt.IsImm()) {
                    MOV(64, R(RSCRATCH), Rt);
                    CMP(64, Rs, R(RSCRATCH));
                } else if (Rs.IsImm()) {
                    MOV(64, R(RSCRATCH), Rs);
                    CMP(64, Rt, R(RSCRATCH));
                    rs_rt_swapped = true;
                } else {
                    MOV(64, R(RSCRATCH), Rt);
                    CMP(64, Rs, R(RSCRATCH));
                }
            }

            // these need to be here in case op->s == op->d
            // (if we did them earlier we would overwrite Rd before the check).
            MOV(64, R(RSCRATCH), Imm32(1));
            MOV(64, Rd, Imm32(0));
            CMOVcc(64, Rd, R(RSCRATCH), rs_rt_swapped ? CC_G : CC_L);
        }
    }
}

void VR4300_Jitter::recompile_SLTU(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            m_gpr.SetImmediate64(op->d, ((u64)(op->s ? m_gpr.Imm64(op->s) : 0) < (u64)(op->t ? m_gpr.Imm64(op->t) : 0)) ? 1 : 0);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
            RegCache::Realize(Rd, Rt, Rs);

            bool rs_rt_swapped = false;
            if (Rs.IsImm() && vr4300_jitter_value_fits_in_32_bit_imm_positive(Rs.Imm64())) {
                CMP_or_TEST(64, Rt, Imm32(Rs.Imm64()));
                rs_rt_swapped = true;
             } else if (Rt.IsImm() && vr4300_jitter_value_fits_in_32_bit_imm_positive(Rt.Imm64())) {
                CMP_or_TEST(64, Rs, Imm32(Rt.Imm64()));
            } else {
                if (Rt.IsImm()) {
                    RCX64Reg scratch = m_gpr.Scratch();
                    RegCache::Realize(scratch);

                    MOV(64, R(scratch), Rt);
                    CMP(64, Rs, R(scratch));
                } else if (Rs.IsImm()) {
                    RCX64Reg scratch = m_gpr.Scratch();
                    RegCache::Realize(scratch);

                    MOV(64, R(scratch), Rs);
                    CMP(64, Rt, R(scratch));
                    rs_rt_swapped = true;
                } else if (Rt.IsSimpleReg()) {
                    CMP(64, Rt, Rs);
                    rs_rt_swapped = true;
                } else if (Rs.IsSimpleReg()) {
                    CMP(64, Rs, Rt);
                } else {
                    RCX64Reg scratch = m_gpr.Scratch();
                    RegCache::Realize(scratch);

                    MOV(64, R(scratch), Rt);
                    CMP(64, Rs, R(scratch));
                }
            }

            // these need to be here in case op->s == op->d
            // (if we did them earlier we would overwrite Rd before the check).
            MOV(32, R(RSCRATCH), Imm32(1));
            MOV(32, Rd, Imm32(0));
            CMOVcc(32, Rd, R(RSCRATCH), rs_rt_swapped ? CC_A : CC_NAE);
        }
    }
}

void VR4300_Jitter::recompile_SLL(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u64 res = (s64)(s32)((op->t ? m_gpr.Imm64(op->t) : 0) << op->k);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            if (op->t != op->d) {
                if (op->k >= 0 && op->k < 256) {
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt);

                    MOV(64, Rd, Rt);
                    SHL(32, Rd, Imm8((u8)op->k));
                    MOVSX(64, 32, Rd, Rd);
                } else {
                    RCX64Reg scratch = m_gpr.Scratch(RCX);
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd, Rt, scratch);

                    MOV(32, scratch, Imm32((u32)op->k));
                    SHLX(32, Rd, R(Rd), scratch);
                    MOVSX(64, 32, Rd, Rd);
                }
            } else {
                if (op->k >= 0 && op->k < 256) {
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd);

                    SHL(32, Rd, Imm8((u8)op->k));
                    MOVSX(64, 32, Rd, Rd);
                } else {
                    RCX64Reg scratch = m_gpr.Scratch(RCX);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd, scratch);

                    MOV(32, scratch, Imm32((u32)op->k));
                    SHLX(32, Rd, R(Rd), scratch);
                    MOVSX(64, 32, Rd, Rd);
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_SRL(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            s64 res = (s64)(s32)((u32)(op->t ? m_gpr.Imm64(op->t) : 0) >> (u32)op->k);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            if (op->t != op->d) {
                if (op->k >= 0 && op->k < 256) {
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt);

                    MOV(64, Rd, Rt);
                    SHR(32, Rd, Imm8((u8)op->k));
                    MOVSX(64, 32, Rd, Rd);
                } else {
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg scratch = m_gpr.Scratch(RCX);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt, scratch);

                    MOV(32, scratch, Imm32((u32)op->k));
                    MOV(64, Rd, Rt);
                    SHRX(32, Rd, Rd, scratch);
                    MOVSX(64, 32, Rd, Rd);
                }
            } else {
                if (op->k >= 0 && op->k < 256) {
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd);

                    SHR(32, Rd, Imm8((u8)op->k));
                    MOVSX(64, 32, Rd, Rd);
                } else {
                    RCX64Reg scratch = m_gpr.Scratch(RCX);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd, scratch);

                    MOV(32, scratch, Imm32((u32)op->k));
                    SHRX(32, Rd, Rd, scratch);
                    MOVSX(64, 32, Rd, Rd);
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_SRA(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u64 res = (s64)((s64)(op->t ? m_gpr.Imm64(op->t) : 0) >> (u32)op->k);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            if (op->t != op->d) {
                if (op->k >= 0 && op->k < 256) {
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt);

                    MOV(64, Rd, Rt);
                    SAR(64, Rd, Imm8((u8)op->k));
                    MOVSX(64, 32, Rd, Rd);
                } else {
                    RCX64Reg scratch = m_gpr.Scratch(RCX);
                    RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
                    RegCache::Realize(Rd, Rt, scratch);

                    MOV(32, scratch, Imm32((u32)op->k));
                    MOV(64, Rd, Rt);
                    SAR(64, Rd, scratch);
                    MOVSX(64, 32, Rd, Rd);
                }
            } else {
                if (op->k >= 0 && op->k < 256) {
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd);

                    SAR(64, Rd, Imm8((u8)op->k));
                    MOVSX(64, 32, Rd, Rd);
                } else {
                    RCX64Reg scratch = m_gpr.Scratch(RCX);
                    RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::ReadWrite);
                    RegCache::Realize(Rd, scratch);

                    MOV(32, scratch, Imm32((u32)op->k));
                    SAR(64, Rd, scratch);
                    MOVSX(64, 32, Rd, Rd);
                }
            }
        }
    }
}

void VR4300_Jitter::recompile_SRLV(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            u32 low_order_bits = op->s ? (m_gpr.Imm64(op->s) & 0x1f) : 0;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) >> (u32)low_order_bits);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rs, Rt, Rd, scratch);

            // in case op->s == op->d do this early
            MOV(64, scratch, Rs);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            AND(32, scratch, Imm32(0x1f));
            SHRX(32, Rd, Rd, scratch);
            MOVSX(64, 32, Rd, Rd);
        }
    }
}

void VR4300_Jitter::recompile_SRAV(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            u32 low_order_bits = op->s ? (m_gpr.Imm64(op->s) & 0x1f) : 0;
            u64 res = ((s64)((s32)(op->t ? m_gpr.Imm64(op->t) : 0) >> (u32)low_order_bits));
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rs, Rt, Rd, scratch);

            // in case op->s == op->d do this early
            MOV(64, scratch, Rs);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            AND(32, scratch, Imm32(0x1f));
            SAR(64, Rd, scratch);
            MOVSX(64, 32, Rd, Rd);
        }
    }
}

void VR4300_Jitter::recompile_DSRAV(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            u32 low_order_bits = op->s ? (m_gpr.Imm64(op->s) & 0x3f) : 0;
            u64 res = ((s64)((s64)(op->t ? m_gpr.Imm64(op->t) : 0) >> (u32)low_order_bits));
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rs, Rt, Rd, scratch);

            // in case op->s == op->d do this early
            MOV(64, scratch, Rs);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            AND(32, scratch, Imm32(0x3f));
            SAR(64, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_SLLV(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            u32 low_order_bits = op->s ? (m_gpr.Imm64(op->s) & 0x1f) : 0;
            u64 res = (u64)((s64)(s32)((op->t ? m_gpr.Imm64(op->t) : 0) << (u32)low_order_bits));
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rs, Rt, Rd, scratch);

            // in case op->s == op->d do this early
            MOV(64, scratch, Rs);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            AND(32, scratch, Imm32(0x1f));
            SHLX(32, Rd, R(Rd), scratch);
            MOVSX(64, 32, Rd, Rd);
        }
    }
}

void VR4300_Jitter::recompile_DSLL32(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u32 shift = 32 + op->k;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) << shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rd, scratch);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            MOV(64, scratch, Imm32(32 + op->k));
            SHLX(64, Rd, R(Rd), scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSLL(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u32 shift = op->k;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) << shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rd, scratch);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            MOV(64, scratch, Imm32(op->k));
            SHLX(64, Rd, R(Rd), scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSRL(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u32 shift = op->k;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) >> shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rd, scratch);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            MOV(64, scratch, Imm32(op->k));
            SHRX(64, Rd, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSRLV(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            u32 shift = op->s ? (u32)m_gpr.Imm64(op->s) : 0;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) >> shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rs, Rd, scratch);

            if (Rs.IsImm()) {
                MOV(64, scratch, Imm32(Rs.Imm64() & 0x3f));
            } else {
                MOV(64, scratch, Rs);
                AND(32, scratch, Imm32(0x3f));
            }

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            SHRX(64, Rd, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSLLV(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_IN(op, s);
    VALIDATE_OUT(op, d);

    if (op->d != 0) {
        if ((!op->t || m_gpr.IsImm(op->t)) && (!op->s || m_gpr.IsImm(op->s))) {
            u32 shift = op->s ? (u32)m_gpr.Imm64(op->s) : 0;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) << shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rs, Rd, scratch);

            if (Rs.IsImm()) {
                MOV(64, scratch, Imm32(Rs.Imm64() & 0x3f));
            } else {
                MOV(64, scratch, Rs);
                AND(32, scratch, Imm32(0x3f));
            }

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            SHLX(64, Rd, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSRL32(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u32 shift = 32 + op->k;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) >> shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rd, scratch);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            MOV(64, scratch, Imm32(32 + op->k));
            SHRX(64, Rd, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSRA32(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u32 shift = 32 + op->k;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) >> shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rd, scratch);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            MOV(64, scratch, Imm32(32 + op->k));
            SAR(64, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_DSRA(struct jit_instr *op)
{
    VALIDATE_IN(op, t);
    VALIDATE_OUT(op, d);
    assert(op->has_k);

    if (op->d != 0) {
        if (!op->t || m_gpr.IsImm(op->t)) {
            u32 shift = op->k;
            u64 res = ((u64)(op->t ? m_gpr.Imm64(op->t) : 0) >> shift);
            m_gpr.SetImmediate64(op->d, res);
        } else {
            RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
            RCX64Reg Rd = m_gpr.Bind(op->d, (op->t == op->d) ? RCMode::ReadWrite : RCMode::Write);
            RCX64Reg scratch = m_gpr.Scratch(RCX);
            RegCache::Realize(Rt, Rd, scratch);

            if (op->d != op->t) {
                MOV(64, Rd, Rt);
            }

            MOV(64, scratch, Imm32(op->k));
            SAR(64, Rd, scratch);
        }
    }
}

void VR4300_Jitter::recompile_MULT(struct jit_instr *op, bool unsigned_multiply)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);

    if (m_gpr.IsImm(op->s) && m_gpr.IsImm(op->t)) {
        u64 m;
        if (unsigned_multiply) {
            m = (u32)m_gpr.Imm64(op->t) * (u32)m_gpr.Imm64(op->s);
        } else {
            m = (s32)m_gpr.Imm64(op->t) * (s32)m_gpr.Imm64(op->s);
        }
        MOV(64, (HOTSTATE_VAR(lo)), Imm32(m & 0xffffffff));
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(m >> 32));
    } else if (op->s && op->t) {
        RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg scratch = m_gpr.Scratch(), rdx = m_gpr.Scratch(RDX), rax = m_gpr.Scratch(RAX);
        RegCache::Realize(Rs, Rt, scratch, rdx, rax);

        MOV(64, R(rdx), Rt);

        if (!Rs.IsSimpleReg()) {
            MOV(64, R(scratch), Rs);
            if (unsigned_multiply) {
                MULX(32, rax, rdx, R(scratch));
            } else {
                MOV(32, R(rax), R(rdx));
                IMUL(32, R(scratch));
            }
        } else {
            if (unsigned_multiply) {
                MULX(32, rax, rdx, Rs);
            } else {
                MOV(32, R(rax), R(rdx));
                IMUL(32, Rs);
            }
        }

        MOVSX(64, 32, rax, R(rax));
        MOVSX(64, 32, rdx, R(rdx));
        MOV(64, (HOTSTATE_VAR(lo)), R(rax));
        MOV(64, (HOTSTATE_VAR(hi)), R(rdx));
    } else {
        MOV(64, (HOTSTATE_VAR(lo)), Imm32(0));
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));
    }
}

void VR4300_Jitter::recompile_MULTU(struct jit_instr *op)
{
    return recompile_MULT(op, true);
}

void VR4300_Jitter::recompile_DMULTU(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);

    if (op->s && op->t) {
        RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
        RCX64Reg rdx = m_gpr.Scratch(RDX);
        RegCache::Realize(Rs, Rt, rdx);

        MOV(64, rdx, Rt);

        assert(rdx == RDX);
        MULX(64, RSCRATCH, rdx, Rs);

        MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
        MOV(64, (HOTSTATE_VAR(hi)), R(rdx));
    } else {
        MOV(64, (HOTSTATE_VAR(lo)), Imm32(0));
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));
    }
}

void VR4300_Jitter::recompile_DMULT(struct jit_instr *op)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);

    if (op->s && op->t) {
        RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
        RCOpArg Rt = m_gpr.Use(op->t, RCMode::Read);
        RCX64Reg rdx = m_gpr.Scratch(RDX);
        RegCache::Realize(Rs, Rt, rdx);

        MOV(64, rdx, Rt);
        MOV(64, R(RSCRATCH), Rs);

        assert(rdx == RDX);
        IMUL(64, RSCRATCH, rdx);

        MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
        MOV(64, (HOTSTATE_VAR(hi)), R(rdx));
    } else {
        MOV(64, (HOTSTATE_VAR(lo)), Imm32(0));
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));
    }
}

void VR4300_Jitter::div_core(struct jit_instr *op, const RCOpArg &edx, const RCOpArg &eax, const RCOpArg &Rs, const RCOpArg &Rt, bool un_signed)
{
    MOV(64, R(eax.GetSimpleReg()), Rs);
    if (un_signed) {
        XOR(32, R(edx.GetSimpleReg()), R(edx.GetSimpleReg()));
    } else {
        CDQ();
    }
    if (un_signed) {
        DIV(32, Rt);
    } else {
        IDIV(32, Rt);
    }
    MOVSX(64, 32, eax.GetSimpleReg(), R(eax.GetSimpleReg()));
    MOVSX(64, 32, edx.GetSimpleReg(), R(edx.GetSimpleReg()));
}

void VR4300_Jitter::recompile_DDIV(struct jit_instr *op, bool unsigned_div)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);

    if ((!op->s || m_gpr.IsImm(op->s)) && (!op->t || m_gpr.IsImm(op->t))) {
        if (unsigned_div) {
            if (op->t && (m_gpr.Imm64(op->t) != 0)) {
                    u64 res = (u64)(op->s ? m_gpr.Imm64(op->s) : 0) / (u64)m_gpr.Imm64(op->t);
                    u64 rem = (u64)(op->s ? m_gpr.Imm64(op->s) : 0) % (u64)m_gpr.Imm64(op->t);
                    if (vr4300_jitter_value_fits_in_32_bit_imm_positive(res)) {
                        MOV(64, (HOTSTATE_VAR(lo)), Imm32(res));
                    } else {
                        MOV(64, R(RSCRATCH), Imm64(res));
                        MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
                    }
                    if (vr4300_jitter_value_fits_in_32_bit_imm_positive(rem)) {
                        MOV(64, (HOTSTATE_VAR(hi)), Imm32(rem));
                    } else {
                        MOV(64, R(RSCRATCH), Imm64(rem));
                        MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));
                    }
            } else {
                MOV(64, (HOTSTATE_VAR(lo)), Imm64(-1));
                if (vr4300_jitter_value_fits_in_32_bit_imm_positive(op->s ? m_gpr.Imm64(op->s) : 0)) {
                    MOV(64, (HOTSTATE_VAR(hi)), Imm32(op->s ? m_gpr.Imm64(op->s) : 0));
                } else {
                    MOV(64, R(RSCRATCH), Imm64(op->s ? m_gpr.Imm64(op->s) : 0));
                    MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));
                }
            }
        } else {
            if (m_gpr.Imm64(op->t) != 0)
            {
                if ((s64)m_gpr.Imm64(op->s) == INT64_MIN && (s64)m_gpr.Imm64(op->t) == -1)
                {
                    if (!op->s || vr4300_jitter_value_fits_in_32_bit_imm_positive(m_gpr.Imm64(op->s))) {
                        MOV(64, (HOTSTATE_VAR(lo)), Imm32(op->s ? m_gpr.Imm64(op->s) : 0));
                    } else {
                        MOV(64, R(RSCRATCH), Imm64(m_gpr.Imm64(op->s)));
                        MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
                    }
                    MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));
                }
                else
                {
                    MOV(64, R(RSCRATCH), Imm64(m_gpr.Imm64(op->s) / m_gpr.Imm64(op->t)));
                    MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
                    MOV(64, R(RSCRATCH), Imm64(m_gpr.Imm64(op->s) % m_gpr.Imm64(op->t)));
                    MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));
                }
            }
            else
            {
                MOV(64, R(RSCRATCH), Imm64(m_gpr.Imm64(op->s)));
                MOV(64, (HOTSTATE_VAR(lo)), (m_gpr.Imm64(op->s) < 0 ? Imm32(1) : Imm64(-1)));
                MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));
            }
        }
    } else if (op->s || op->t) {
        RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
        RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);
        RCX64Reg rax = m_gpr.Scratch(RAX);
        RCX64Reg rdx = m_gpr.Scratch(RDX);
        RCX64Reg scratch = m_gpr.Scratch();

        RegCache::Realize(Rs, Rt, rax, rdx, scratch);

        CMP_or_TEST(64, Rt, Imm32(0));
        FixupBranch rt_zero = J_CC(CC_Z);

        // if (rrs == INT64_MIN && rrt == -1)
        MOV(64, R(RSCRATCH2), Imm64(-1));
        CMP(64, Rt, R(RSCRATCH2));
        FixupBranch not_special_case2 = J_CC(CC_NE);

        MOV(64, R(RSCRATCH), Rs);
        MOV(64, R(RSCRATCH2), Imm64(INT64_MIN));
        CMP(32, R(RSCRATCH), R(RSCRATCH2));
        FixupBranch not_special_case = J_CC(CC_NE);

        MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));

        SetJumpTarget(not_special_case);
        SetJumpTarget(not_special_case2);

        MOV(64, rdx, Imm32(0));
        MOV(64, rax, Rs);
        if (unsigned_div) {
            DIV(64, Rt);
        } else {
            IDIV(64, Rt);
        }

        MOV(64, (HOTSTATE_VAR(lo)), R(rax));
        MOV(64, (HOTSTATE_VAR(hi)), R(rdx));
        FixupBranch exit = J();

        SetJumpTarget(rt_zero);

        if (unsigned_div) {
            MOV(64, R(RSCRATCH), Imm32(-1));
        } else {
            MOV(64, R(RSCRATCH), Imm64(-1));
        }

        CMP(64, Rs, Imm32(0));
        FixupBranch rs_less_than_0 = J_CC(CC_L);

        MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH)); // -1
        MOV(64, R(RSCRATCH), Rs);
        MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));

        FixupBranch exit2 = J();

        SetJumpTarget(rs_less_than_0);
        if (unsigned_div) {
            MOV(64, R(RSCRATCH), Imm64(-1));
            MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
        } else {
            MOV(64, (HOTSTATE_VAR(lo)), Imm32(1));
        }
        MOV(64, R(RSCRATCH), Rs);
        MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));

        SetJumpTarget(exit);
        SetJumpTarget(exit2);

    } else {
        if (unsigned_div) {
            MOV(64, (HOTSTATE_VAR(lo)), Imm32(-1));
        } else {
            MOV(64, R(RSCRATCH), Imm64(-1));
            MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
        }
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));
    }
}

void VR4300_Jitter::recompile_DDIVU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_DDIV(op, true);
}

void VR4300_Jitter::recompile_DIV(struct jit_instr *op, bool un_signed)
{
    VALIDATE_IN(op, s);
    VALIDATE_IN(op, t);

    RCOpArg Rs = op->s ? m_gpr.Use(op->s, RCMode::Read) : RCOpArg::Imm64(0);
    RCOpArg Rt = op->t ? m_gpr.Use(op->t, RCMode::Read) : RCOpArg::Imm64(0);

    RegCache::Realize(Rs, Rt);

    if (Rs.IsImm() && Rt.IsImm()) {
        if (!un_signed) {
            if ((u32)Rt.Imm64()) {
                if ((s64)Rs.Imm64() == INT32_MIN && (s32)Rt.Imm64() == -1) {
                    perform_for_64bit_imm(MOV, (HOTSTATE_VAR(lo)), (s64)(s32)Rs.Imm64(), RSCRATCH);
                    MOV(64, HOTSTATE_VAR(hi), Imm32(0));
                } else {
                    perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), (s64)((s32)Rs.Imm64() / (s32)Rt.Imm64()), RSCRATCH);
                    perform_for_64bit_imm(MOV, HOTSTATE_VAR(hi), (s64)((s32)Rs.Imm64() % (s32)Rt.Imm64()), RSCRATCH);
                }
            } else {
                perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), (s64)((s32)Rs.Imm64() < 0 ? 1 : -1), RSCRATCH);
                perform_for_64bit_imm(MOV, HOTSTATE_VAR(hi), (s64)(s32)Rs.Imm64(), RSCRATCH);
            }
        } else {
            if ((u32)Rt.Imm64()) {
                perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), (s64)((u32)Rs.Imm64() / (u32)Rt.Imm64()), RSCRATCH);
                perform_for_64bit_imm(MOV, HOTSTATE_VAR(hi), (s64)((u32)Rs.Imm64() % (u32)Rt.Imm64()), RSCRATCH);
            } else {
                perform_for_64bit_imm(MOV, (HOTSTATE_VAR(lo)), -1, RSCRATCH);
                perform_for_64bit_imm(MOV, (HOTSTATE_VAR(hi)), (s64)(s32)Rs.Imm64(), RSCRATCH);
            }
        }
    } else {
        // here at most one can be an imm.
        if (!un_signed) {
            RCX64Reg eax = m_gpr.Scratch(EAX); // no register choice
            RCX64Reg edx = m_gpr.Scratch(EDX); // no register choice
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(eax, edx, scratch);

            if (Rt.IsImm()) {
                if ((u32)Rt.Imm64()) {
                    if ((s32)Rt.Imm64() == -1) {
                        FixupBranch rs_not_int32_min = J_CC(CC_NE);

                        MOVSX(64, 32, scratch, Rs);
                        MOV(64, HOTSTATE_VAR(lo), R(scratch));
                        MOV(64, HOTSTATE_VAR(hi), Imm32(0));

                        FixupBranch exit = J();

                        SetJumpTarget(rs_not_int32_min);

                        MOV(32, R(scratch), Imm32(Rt.Imm64()));
                        div_core(op, RCOpArg::R(edx), RCOpArg::R(eax), Rs, RCOpArg::R(scratch), un_signed);

                        MOV(64, HOTSTATE_VAR(lo), R(eax));
                        MOV(64, HOTSTATE_VAR(hi), R(edx));
                        SetJumpTarget(exit);
                    } else {
                        MOV(32, R(scratch), Imm32(Rt.Imm64()));
                        div_core(op, RCOpArg::R(edx), RCOpArg::R(eax), Rs, RCOpArg::R(scratch), un_signed);

                        MOV(64, HOTSTATE_VAR(lo), R(eax));
                        MOV(64, HOTSTATE_VAR(hi), R(edx));
                    }
                } else {
                    CMP(32, Rs, Imm32(0));
                    FixupBranch rs_less_than_0 = J_CC(CC_L);
                    perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), -1, scratch);
                    FixupBranch done = J();
                    SetJumpTarget(rs_less_than_0);
                    MOV(64, HOTSTATE_VAR(lo), Imm32(1));
                    SetJumpTarget(done);
                    MOVSX(64, 32, scratch, Rs);
                    MOV(64, HOTSTATE_VAR(hi), R(scratch));
                }
            } else {
                CMP(32, Rt, Imm32(0));
                FixupBranch rt_zero = J_CC(CC_E);

                CMP(32, Rt, Imm32(-1));
                FixupBranch rt_not_minus_1 = J_CC(CC_NE);
                FixupBranch exit2;

                if (Rs.IsImm()) {
                    if ((s32)Rs.Imm64() == INT32_MIN) {
                        MOV(32, R(scratch), Imm32(Rs.Imm64()));
                        MOVSX(64, 32, scratch, R(scratch));
                        MOV(64, HOTSTATE_VAR(lo), R(scratch));
                        MOV(64, HOTSTATE_VAR(hi), Imm32(0));
                        exit2 = J();
                    }
                } else {
                    CMP(32, Rs, Imm32(INT32_MIN));
                    FixupBranch rs_not_int32_min = J_CC(CC_NE);

                    MOVSX(64, 32, scratch, Rs);
                    MOV(64, HOTSTATE_VAR(lo), R(scratch));
                    MOV(64, HOTSTATE_VAR(hi), Imm32(0));

                    exit2 = J();

                    SetJumpTarget(rs_not_int32_min);
                }

                SetJumpTarget(rt_not_minus_1);

                div_core(op, RCOpArg::R(edx), RCOpArg::R(eax), Rs, Rt, un_signed);
                MOV(64, HOTSTATE_VAR(lo), R(eax));
                MOV(64, HOTSTATE_VAR(hi), R(edx));

                FixupBranch exit = J();

                SetJumpTarget(rt_zero);
                if (Rs.IsImm()) {
                    perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), (s32)Rs.Imm64() < 0 ? 1 : -1, scratch);
                    MOV(64, R(scratch), Imm64((s64)(s32)Rs.Imm64()));
                    MOV(64, HOTSTATE_VAR(hi), R(scratch));
                } else {
                    CMP(32, Rs, Imm32(0));
                    FixupBranch rs_less_than_0 = J_CC(CC_L);
                    perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), -1, scratch);
                    FixupBranch done = J();
                    SetJumpTarget(rs_less_than_0);
                    MOV(64, HOTSTATE_VAR(lo), Imm32(1));
                    SetJumpTarget(done);
                    MOVSX(64, 32, scratch, Rs);
                    MOV(64, HOTSTATE_VAR(hi), R(scratch));
                }

                SetJumpTarget(exit);
                if (!Rs.IsImm() || ((s32)Rs.Imm64() == INT32_MIN)) {
                    SetJumpTarget(exit2);
                }
            }
        } else {
            RCX64Reg eax = m_gpr.Scratch(EAX); // no register choice
            RCX64Reg edx = m_gpr.Scratch(EDX); // no register choice
            RCX64Reg scratch = m_gpr.Scratch();
            RegCache::Realize(eax, edx, scratch);

            if (Rt.IsImm()) {
                if ((u32)Rt.Imm64()) {
                    MOV(32, R(scratch), Imm32(Rt.Imm64()));
                    div_core(op, RCOpArg::R(edx), RCOpArg::R(eax), Rs, RCOpArg::R(scratch), un_signed);
                    MOV(64, HOTSTATE_VAR(lo), R(eax));
                    MOV(64, HOTSTATE_VAR(hi), R(edx));
                } else {
                    perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), -1, scratch);
                    MOVSX(64, 32, scratch, Rs);
                    MOV(64, HOTSTATE_VAR(hi), R(scratch));
                }
            } else {
                CMP(32, Rt, Imm32(0));
                FixupBranch rt_zero = J_CC(CC_E);

                div_core(op, RCOpArg::R(edx), RCOpArg::R(eax), Rs, Rt, un_signed);
                MOV(64, HOTSTATE_VAR(lo), R(eax));
                MOV(64, HOTSTATE_VAR(hi), R(edx));

                FixupBranch exit = J();

                SetJumpTarget(rt_zero);
                perform_for_64bit_imm(MOV, HOTSTATE_VAR(lo), -1, scratch);
                if (Rs.IsImm()) {
                    MOV(32, R(scratch), Imm32(Rs.Imm64()));
                    MOVSX(64, 32, scratch, R(scratch));
                } else {
                    MOVSX(64, 32, scratch, Rs);
                }
                MOV(64, HOTSTATE_VAR(hi), R(scratch));

                SetJumpTarget(exit);
            }
        }
    }
}

void VR4300_Jitter::recompile_DIVU(struct jit_instr *op)
{
    VR4300_Jitter::recompile_DIV(op, true);
}

void VR4300_Jitter::recompile_MFLO(struct jit_instr *op)
{
    VALIDATE_OUT(op, d);

    if (op->d) {
        RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
        RegCache::Realize(Rd);
        MOV(64, Rd, (HOTSTATE_VAR(lo)));
    }
}

void VR4300_Jitter::recompile_MFHI(struct jit_instr *op)
{
    VALIDATE_OUT(op, d);

    if (op->d) {
        RCX64Reg Rd = m_gpr.Bind(op->d, RCMode::Write);
        RegCache::Realize(Rd);
        MOV(64, Rd, (HOTSTATE_VAR(hi)));
    }
}

void VR4300_Jitter::recompile_MTLO(struct jit_instr *op)
{
    VALIDATE_IN(op, s);

    if (op->s) {
        RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        if (Rs.IsSimpleReg()) {
            MOV(64, (HOTSTATE_VAR(lo)), Rs);
        } else if (Rs.IsImm()) {
            perform_for_64bit_imm(MOV, (HOTSTATE_VAR(lo)), Rs.Imm64(), RSCRATCH);
        } else {
            MOV(64, R(RSCRATCH), Rs);
            MOV(64, (HOTSTATE_VAR(lo)), R(RSCRATCH));
        }
    } else {
        MOV(64, (HOTSTATE_VAR(lo)), Imm32(0));
    }
}

void VR4300_Jitter::recompile_MTHI(struct jit_instr *op)
{
    VALIDATE_IN(op, s);

    if (op->s) {
        RCOpArg Rs = m_gpr.Use(op->s, RCMode::Read);
        RegCache::Realize(Rs);
        if (!Rs.IsSimpleReg()) {
            MOV(64, R(RSCRATCH), Rs);
            MOV(64, (HOTSTATE_VAR(hi)), R(RSCRATCH));
        } else {
            MOV(64, (HOTSTATE_VAR(hi)), Rs);
        }
    } else {
        MOV(64, (HOTSTATE_VAR(hi)), Imm32(0));
    }
}


