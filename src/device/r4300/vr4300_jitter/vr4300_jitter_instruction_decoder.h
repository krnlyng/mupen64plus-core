/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_instruction_decoder.h                     *
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

#ifndef M64P_DEVICE_R4300_VR4300_JITTER_INSTRUCTION_DECODER_H
#define M64P_DEVICE_R4300_VR4300_JITTER_INSTRUCTION_DECODER_H

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include "Common/BitSet.h"

struct jit_instr {
    uint32_t instruction = 0;
    uint32_t address = 0;
    bool is_branch_or_jump = false;
    bool may_cause_exception = false;
    bool is_delay_slot = false;
    bool is_first_float_instruction = false;
    bool next_is_follow = false;
    bool next_is_link_continue = false;
    bool next_is_extend = false;
    uint32_t operation = 0;
    BitSet32 regsInUse = BitSet32::AllTrue(0);
    BitSet32 fregsInUse = BitSet32::AllTrue(0);
    BitSet32 fregsInUse32 = BitSet32::AllTrue(0);
    BitSet32 regsDiscardable = BitSet32::AllTrue(0);
    BitSet32 fregsDiscardable = BitSet32::AllTrue(0);
    BitSet32 regsIn = BitSet32::AllTrue(0);
    BitSet32 fregsIn = BitSet32::AllTrue(0);
    BitSet32 regsOut = BitSet32::AllTrue(0);
    BitSet32 fregsOut = BitSet32::AllTrue(0);
    BitSet32 fregsIn32 = BitSet32::AllTrue(0);
    BitSet32 fregsOut32 = BitSet32::AllTrue(0);
    BitSet32 fregsIncompatible = BitSet32::AllTrue(0);
    BitSet32 fregsIncompatible32 = BitSet32::AllTrue(0);
    bool modifies_count_reg = false;
    bool modifies_status_reg = false;
    // fmt - operand format (float)
    int a = 0;
    bool has_a = false;
    // base address
    int b = 0;
    bool has_b = false;
    // cond - conditional
    int c = 0;
    bool has_c = false;
    // destination register number
    int d = 0;
    bool has_d = false;
    // offset address
    int f = 0;
    bool has_f = false;
    // literal/immediate value
    int k = 0;
    bool has_k = false;
    // source register number
    int s = 0;
    bool has_s = false;
    // temporary register number
    int t = 0;
    bool has_t = false;
    // coprocessor number
    int x = 0;
    bool has_x = false;
    const char *name = nullptr;
};

union vr4300_instruction {
    uint32_t raw;
    vr4300_instruction() = default;
    vr4300_instruction(uint32_t instruction) {
        raw = instruction;
    }

    struct {
        uint32_t ADD_opcode_0_10 : 11; // 32
        uint32_t ADD_d : 5; // 0
        uint32_t ADD_t : 5; // 0
        uint32_t ADD_s : 5; // 0
        uint32_t ADD_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t ADDI_k : 16; // 0
        uint32_t ADDI_t : 5; // 0
        uint32_t ADDI_s : 5; // 0
        uint32_t ADDI_opcode_26_31 : 6; // 8
    };
    struct {
        uint32_t ADDIU_k : 16; // 0
        uint32_t ADDIU_t : 5; // 0
        uint32_t ADDIU_s : 5; // 0
        uint32_t ADDIU_opcode_26_31 : 6; // 9
    };
    struct {
        uint32_t ADDU_opcode_0_10 : 11; // 33
        uint32_t ADDU_d : 5; // 0
        uint32_t ADDU_t : 5; // 0
        uint32_t ADDU_s : 5; // 0
        uint32_t ADDU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t AND_opcode_0_10 : 11; // 36
        uint32_t AND_d : 5; // 0
        uint32_t AND_t : 5; // 0
        uint32_t AND_s : 5; // 0
        uint32_t AND_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t ANDI_k : 16; // 0
        uint32_t ANDI_t : 5; // 0
        uint32_t ANDI_s : 5; // 0
        uint32_t ANDI_opcode_26_31 : 6; // 12
    };
    struct {
        uint32_t BEQ_f : 16; // 0
        uint32_t BEQ_t : 5; // 0
        uint32_t BEQ_s : 5; // 0
        uint32_t BEQ_opcode_26_31 : 6; // 4
    };
    struct {
        uint32_t BEQL_f : 16; // 0
        uint32_t BEQL_t : 5; // 0
        uint32_t BEQL_s : 5; // 0
        uint32_t BEQL_opcode_26_31 : 6; // 20
    };
    struct {
        uint32_t BGEZ_f : 16; // 0
        uint32_t BGEZ_opcode_16_20 : 5; // 1
        uint32_t BGEZ_s : 5; // 0
        uint32_t BGEZ_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BGEZAL_f : 16; // 0
        uint32_t BGEZAL_opcode_16_20 : 5; // 17
        uint32_t BGEZAL_s : 5; // 0
        uint32_t BGEZAL_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BGEZALL_f : 16; // 0
        uint32_t BGEZALL_opcode_16_20 : 5; // 19
        uint32_t BGEZALL_s : 5; // 0
        uint32_t BGEZALL_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BGEZL_f : 16; // 0
        uint32_t BGEZL_opcode_16_20 : 5; // 3
        uint32_t BGEZL_s : 5; // 0
        uint32_t BGEZL_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BGTZ_f : 16; // 0
        uint32_t BGTZ_opcode_16_20 : 5; // 0
        uint32_t BGTZ_s : 5; // 0
        uint32_t BGTZ_opcode_26_31 : 6; // 7
    };
    struct {
        uint32_t BGTZL_f : 16; // 0
        uint32_t BGTZL_opcode_16_20 : 5; // 0
        uint32_t BGTZL_s : 5; // 0
        uint32_t BGTZL_opcode_26_31 : 6; // 23
    };
    struct {
        uint32_t BLEZ_f : 16; // 0
        uint32_t BLEZ_opcode_16_20 : 5; // 0
        uint32_t BLEZ_s : 5; // 0
        uint32_t BLEZ_opcode_26_31 : 6; // 6
    };
    struct {
        uint32_t BLEZL_f : 16; // 0
        uint32_t BLEZL_opcode_16_20 : 5; // 0
        uint32_t BLEZL_s : 5; // 0
        uint32_t BLEZL_opcode_26_31 : 6; // 22
    };
    struct {
        uint32_t BLTZ_f : 16; // 0
        uint32_t BLTZ_opcode_16_20 : 5; // 0
        uint32_t BLTZ_s : 5; // 0
        uint32_t BLTZ_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BLTZAL_f : 16; // 0
        uint32_t BLTZAL_opcode_16_20 : 5; // 16
        uint32_t BLTZAL_s : 5; // 0
        uint32_t BLTZAL_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BLTZALL_f : 16; // 0
        uint32_t BLTZALL_opcode_16_20 : 5; // 18
        uint32_t BLTZALL_s : 5; // 0
        uint32_t BLTZALL_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BLTZL_f : 16; // 0
        uint32_t BLTZL_opcode_16_20 : 5; // 2
        uint32_t BLTZL_s : 5; // 0
        uint32_t BLTZL_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t BNE_f : 16; // 0
        uint32_t BNE_t : 5; // 0
        uint32_t BNE_s : 5; // 0
        uint32_t BNE_opcode_26_31 : 6; // 5
    };
    struct {
        uint32_t BNEL_f : 16; // 0
        uint32_t BNEL_t : 5; // 0
        uint32_t BNEL_s : 5; // 0
        uint32_t BNEL_opcode_26_31 : 6; // 21
    };
    struct {
        uint32_t BREAK_opcode_0_5 : 6; // 13
        uint32_t BREAK_k : 20; // 0
        uint32_t BREAK_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t CACHE_f : 16; // 0
        uint32_t CACHE_k : 5; // 0
        uint32_t CACHE_b : 5; // 0
        uint32_t CACHE_opcode_26_31 : 6; // 47
    };
    struct {
        uint32_t CFC0_opcode_0_10 : 11; // 0
        uint32_t CFC0_d : 5; // 0
        uint32_t CFC0_t : 5; // 0
        uint32_t CFC0_opcode_21_31 : 11; // 514
    };
    struct {
        uint32_t CFC1_opcode_0_10 : 11; // 0
        uint32_t CFC1_d : 5; // 0
        uint32_t CFC1_t : 5; // 0
        uint32_t CFC1_opcode_21_31 : 11; // 546
    };
    struct {
        uint32_t CFC2_opcode_0_10 : 11; // 0
        uint32_t CFC2_d : 5; // 0
        uint32_t CFC2_t : 5; // 0
        uint32_t CFC2_opcode_21_31 : 11; // 578
    };
    struct {
        uint32_t CTC0_opcode_0_10 : 11; // 0
        uint32_t CTC0_d : 5; // 0
        uint32_t CTC0_t : 5; // 0
        uint32_t CTC0_opcode_21_31 : 11; // 518
    };
    struct {
        uint32_t CTC1_opcode_0_10 : 11; // 0
        uint32_t CTC1_d : 5; // 0
        uint32_t CTC1_t : 5; // 0
        uint32_t CTC1_opcode_21_31 : 11; // 550
    };
    struct {
        uint32_t CTC2_opcode_0_10 : 11; // 0
        uint32_t CTC2_d : 5; // 0
        uint32_t CTC2_t : 5; // 0
        uint32_t CTC2_opcode_21_31 : 11; // 582
    };
    struct {
        uint32_t DADD_opcode_0_10 : 11; // 44
        uint32_t DADD_d : 5; // 0
        uint32_t DADD_t : 5; // 0
        uint32_t DADD_s : 5; // 0
        uint32_t DADD_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DADDI_k : 16; // 0
        uint32_t DADDI_t : 5; // 0
        uint32_t DADDI_s : 5; // 0
        uint32_t DADDI_opcode_26_31 : 6; // 24
    };
    struct {
        uint32_t DADDIU_k : 16; // 0
        uint32_t DADDIU_t : 5; // 0
        uint32_t DADDIU_s : 5; // 0
        uint32_t DADDIU_opcode_26_31 : 6; // 25
    };
    struct {
        uint32_t DADDU_opcode_0_10 : 11; // 45
        uint32_t DADDU_d : 5; // 0
        uint32_t DADDU_t : 5; // 0
        uint32_t DADDU_s : 5; // 0
        uint32_t DADDU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DDIV_opcode_0_15 : 16; // 30
        uint32_t DDIV_t : 5; // 0
        uint32_t DDIV_s : 5; // 0
        uint32_t DDIV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DDIVU_opcode_0_15 : 16; // 31
        uint32_t DDIVU_t : 5; // 0
        uint32_t DDIVU_s : 5; // 0
        uint32_t DDIVU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DIV_opcode_0_15 : 16; // 26
        uint32_t DIV_t : 5; // 0
        uint32_t DIV_s : 5; // 0
        uint32_t DIV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DIVU_opcode_0_15 : 16; // 27
        uint32_t DIVU_t : 5; // 0
        uint32_t DIVU_s : 5; // 0
        uint32_t DIVU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DMFC0_opcode_0_10 : 11; // 0
        uint32_t DMFC0_d : 5; // 0
        uint32_t DMFC0_t : 5; // 0
        uint32_t DMFC0_opcode_21_31 : 11; // 513
    };
    struct {
        uint32_t DMFC1_opcode_0_10 : 11; // 0
        uint32_t DMFC1_d : 5; // 0
        uint32_t DMFC1_t : 5; // 0
        uint32_t DMFC1_opcode_21_31 : 11; // 545
    };
    struct {
        uint32_t DMFC2_opcode_0_10 : 11; // 0
        uint32_t DMFC2_d : 5; // 0
        uint32_t DMFC2_t : 5; // 0
        uint32_t DMFC2_opcode_21_31 : 11; // 577
    };
    struct {
        uint32_t DCFC1_opcode_0_10 : 11; // 0
        uint32_t DCFC1_d : 5; // 0
        uint32_t DCFC1_t : 5; // 0
        uint32_t DCFC1_opcode_21_31 : 11; // 547
    };
    struct {
        uint32_t DCFC2_opcode_0_10 : 11; // 0
        uint32_t DCFC2_d : 5; // 0
        uint32_t DCFC2_t : 5; // 0
        uint32_t DCFC2_opcode_21_31 : 11; // 579
    };
    struct {
        uint32_t DMTC0_opcode_0_10 : 11; // 0
        uint32_t DMTC0_d : 5; // 0
        uint32_t DMTC0_t : 5; // 0
        uint32_t DMTC0_opcode_21_31 : 11; // 517
    };
    struct {
        uint32_t DMTC1_opcode_0_10 : 11; // 0
        uint32_t DMTC1_d : 5; // 0
        uint32_t DMTC1_t : 5; // 0
        uint32_t DMTC1_opcode_21_31 : 11; // 549
    };
    struct {
        uint32_t DMTC2_opcode_0_10 : 11; // 0
        uint32_t DMTC2_d : 5; // 0
        uint32_t DMTC2_t : 5; // 0
        uint32_t DMTC2_opcode_21_31 : 11; // 581
    };
    struct {
        uint32_t DCTC1_opcode_0_10 : 11; // 0
        uint32_t DCTC1_d : 5; // 0
        uint32_t DCTC1_t : 5; // 0
        uint32_t DCTC1_opcode_21_31 : 11; // 551
    };
    struct {
        uint32_t DCTC2_opcode_0_10 : 11; // 0
        uint32_t DCTC2_d : 5; // 0
        uint32_t DCTC2_t : 5; // 0
        uint32_t DCTC2_opcode_21_31 : 11; // 583
    };
    struct {
        uint32_t DMULT_opcode_0_15 : 16; // 28
        uint32_t DMULT_t : 5; // 0
        uint32_t DMULT_s : 5; // 0
        uint32_t DMULT_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DMULTU_opcode_0_15 : 16; // 29
        uint32_t DMULTU_t : 5; // 0
        uint32_t DMULTU_s : 5; // 0
        uint32_t DMULTU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DSLL_opcode_0_5 : 6; // 56
        uint32_t DSLL_k : 5; // 0
        uint32_t DSLL_d : 5; // 0
        uint32_t DSLL_t : 5; // 0
        uint32_t DSLL_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t DSLLV_opcode_0_10 : 11; // 20
        uint32_t DSLLV_d : 5; // 0
        uint32_t DSLLV_t : 5; // 0
        uint32_t DSLLV_s : 5; // 0
        uint32_t DSLLV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DSLL32_opcode_0_5 : 6; // 60
        uint32_t DSLL32_k : 5; // 0
        uint32_t DSLL32_d : 5; // 0
        uint32_t DSLL32_t : 5; // 0
        uint32_t DSLL32_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t DSRA_opcode_0_5 : 6; // 59
        uint32_t DSRA_k : 5; // 0
        uint32_t DSRA_d : 5; // 0
        uint32_t DSRA_t : 5; // 0
        uint32_t DSRA_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t DSRAV_opcode_0_10 : 11; // 23
        uint32_t DSRAV_d : 5; // 0
        uint32_t DSRAV_t : 5; // 0
        uint32_t DSRAV_s : 5; // 0
        uint32_t DSRAV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DSRA32_opcode_0_5 : 6; // 63
        uint32_t DSRA32_k : 5; // 0
        uint32_t DSRA32_d : 5; // 0
        uint32_t DSRA32_t : 5; // 0
        uint32_t DSRA32_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t DSRL_opcode_0_5 : 6; // 58
        uint32_t DSRL_k : 5; // 0
        uint32_t DSRL_d : 5; // 0
        uint32_t DSRL_t : 5; // 0
        uint32_t DSRL_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t DSRLV_opcode_0_10 : 11; // 22
        uint32_t DSRLV_d : 5; // 0
        uint32_t DSRLV_t : 5; // 0
        uint32_t DSRLV_s : 5; // 0
        uint32_t DSRLV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DSRL32_opcode_0_5 : 6; // 62
        uint32_t DSRL32_k : 5; // 0
        uint32_t DSRL32_d : 5; // 0
        uint32_t DSRL32_t : 5; // 0
        uint32_t DSRL32_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t DSUB_opcode_0_10 : 11; // 46
        uint32_t DSUB_d : 5; // 0
        uint32_t DSUB_t : 5; // 0
        uint32_t DSUB_s : 5; // 0
        uint32_t DSUB_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t DSUBU_opcode_0_10 : 11; // 47
        uint32_t DSUBU_d : 5; // 0
        uint32_t DSUBU_t : 5; // 0
        uint32_t DSUBU_s : 5; // 0
        uint32_t DSUBU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t ERET_opcode_0_31 : 32; // 1107296280
    };
    struct {
        uint32_t J_k : 26; // 0
        uint32_t J_opcode_26_31 : 6; // 2
    };
    struct {
        uint32_t JAL_k : 26; // 0
        uint32_t JAL_opcode_26_31 : 6; // 3
    };
    struct {
        uint32_t JALR_opcode_0_10 : 11; // 9
        uint32_t JALR_d : 5; // 0
        uint32_t JALR_opcode_16_20 : 5; // 0
        uint32_t JALR_s : 5; // 0
        uint32_t JALR_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t JR_opcode_0_20 : 21; // 8
        uint32_t JR_s : 5; // 0
        uint32_t JR_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t LB_f : 16; // 0
        uint32_t LB_t : 5; // 0
        uint32_t LB_b : 5; // 0
        uint32_t LB_opcode_26_31 : 6; // 32
    };
    struct {
        uint32_t LBU_f : 16; // 0
        uint32_t LBU_t : 5; // 0
        uint32_t LBU_b : 5; // 0
        uint32_t LBU_opcode_26_31 : 6; // 36
    };
    struct {
        uint32_t LD_f : 16; // 0
        uint32_t LD_t : 5; // 0
        uint32_t LD_b : 5; // 0
        uint32_t LD_opcode_26_31 : 6; // 55
    };
    struct {
        uint32_t LDL_f : 16; // 0
        uint32_t LDL_t : 5; // 0
        uint32_t LDL_b : 5; // 0
        uint32_t LDL_opcode_26_31 : 6; // 26
    };
    struct {
        uint32_t LDR_f : 16; // 0
        uint32_t LDR_t : 5; // 0
        uint32_t LDR_b : 5; // 0
        uint32_t LDR_opcode_26_31 : 6; // 27
    };
    struct {
        uint32_t LH_f : 16; // 0
        uint32_t LH_t : 5; // 0
        uint32_t LH_b : 5; // 0
        uint32_t LH_opcode_26_31 : 6; // 33
    };
    struct {
        uint32_t LHU_f : 16; // 0
        uint32_t LHU_t : 5; // 0
        uint32_t LHU_b : 5; // 0
        uint32_t LHU_opcode_26_31 : 6; // 37
    };
    struct {
        uint32_t LL_f : 16; // 0
        uint32_t LL_t : 5; // 0
        uint32_t LL_b : 5; // 0
        uint32_t LL_opcode_26_31 : 6; // 48
    };
    struct {
        uint32_t LLD_f : 16; // 0
        uint32_t LLD_t : 5; // 0
        uint32_t LLD_b : 5; // 0
        uint32_t LLD_opcode_26_31 : 6; // 52
    };
    struct {
        uint32_t LUI_k : 16; // 0
        uint32_t LUI_t : 5; // 0
        uint32_t LUI_opcode_21_31 : 11; // 480
    };
    struct {
        uint32_t LW_f : 16; // 0
        uint32_t LW_t : 5; // 0
        uint32_t LW_b : 5; // 0
        uint32_t LW_opcode_26_31 : 6; // 35
    };
    struct {
        uint32_t LWL_f : 16; // 0
        uint32_t LWL_t : 5; // 0
        uint32_t LWL_b : 5; // 0
        uint32_t LWL_opcode_26_31 : 6; // 34
    };
    struct {
        uint32_t LWR_f : 16; // 0
        uint32_t LWR_t : 5; // 0
        uint32_t LWR_b : 5; // 0
        uint32_t LWR_opcode_26_31 : 6; // 38
    };
    struct {
        uint32_t LWU_f : 16; // 0
        uint32_t LWU_t : 5; // 0
        uint32_t LWU_b : 5; // 0
        uint32_t LWU_opcode_26_31 : 6; // 39
    };
    struct {
        uint32_t MFC0_opcode_0_10 : 11; // 0
        uint32_t MFC0_d : 5; // 0
        uint32_t MFC0_t : 5; // 0
        uint32_t MFC0_opcode_21_31 : 11; // 512
    };
    struct {
        uint32_t MFC1_opcode_0_10 : 11; // 0
        uint32_t MFC1_d : 5; // 0
        uint32_t MFC1_t : 5; // 0
        uint32_t MFC1_opcode_21_31 : 11; // 544
    };
    struct {
        uint32_t MFC2_opcode_0_10 : 11; // 0
        uint32_t MFC2_d : 5; // 0
        uint32_t MFC2_t : 5; // 0
        uint32_t MFC2_opcode_21_31 : 11; // 576
    };
    struct {
        uint32_t MFHI_opcode_0_10 : 11; // 16
        uint32_t MFHI_d : 5; // 0
        uint32_t MFHI_opcode_16_31 : 16; // 0
    };
    struct {
        uint32_t MFLO_opcode_0_10 : 11; // 18
        uint32_t MFLO_d : 5; // 0
        uint32_t MFLO_opcode_16_31 : 16; // 0
    };
    struct {
        uint32_t MTC0_opcode_0_10 : 11; // 0
        uint32_t MTC0_d : 5; // 0
        uint32_t MTC0_t : 5; // 0
        uint32_t MTC0_opcode_21_31 : 11; // 516
    };
    struct {
        uint32_t MTC1_opcode_0_10 : 11; // 0
        uint32_t MTC1_d : 5; // 0
        uint32_t MTC1_t : 5; // 0
        uint32_t MTC1_opcode_21_31 : 11; // 548
    };
    struct {
        uint32_t MTC2_opcode_0_10 : 11; // 0
        uint32_t MTC2_d : 5; // 0
        uint32_t MTC2_t : 5; // 0
        uint32_t MTC2_opcode_21_31 : 11; // 580
    };
    struct {
        uint32_t MTHI_opcode_0_20 : 21; // 17
        uint32_t MTHI_s : 5; // 0
        uint32_t MTHI_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t MTLO_opcode_0_20 : 21; // 19
        uint32_t MTLO_s : 5; // 0
        uint32_t MTLO_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t MULT_opcode_0_15 : 16; // 24
        uint32_t MULT_t : 5; // 0
        uint32_t MULT_s : 5; // 0
        uint32_t MULT_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t MULTU_opcode_0_15 : 16; // 25
        uint32_t MULTU_t : 5; // 0
        uint32_t MULTU_s : 5; // 0
        uint32_t MULTU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t NOR_opcode_0_10 : 11; // 39
        uint32_t NOR_d : 5; // 0
        uint32_t NOR_t : 5; // 0
        uint32_t NOR_s : 5; // 0
        uint32_t NOR_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t OR_opcode_0_10 : 11; // 37
        uint32_t OR_d : 5; // 0
        uint32_t OR_t : 5; // 0
        uint32_t OR_s : 5; // 0
        uint32_t OR_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t ORI_k : 16; // 0
        uint32_t ORI_t : 5; // 0
        uint32_t ORI_s : 5; // 0
        uint32_t ORI_opcode_26_31 : 6; // 13
    };
    struct {
        uint32_t SB_f : 16; // 0
        uint32_t SB_t : 5; // 0
        uint32_t SB_b : 5; // 0
        uint32_t SB_opcode_26_31 : 6; // 40
    };
    struct {
        uint32_t SC_f : 16; // 0
        uint32_t SC_t : 5; // 0
        uint32_t SC_b : 5; // 0
        uint32_t SC_opcode_26_31 : 6; // 56
    };
    struct {
        uint32_t SCD_f : 16; // 0
        uint32_t SCD_t : 5; // 0
        uint32_t SCD_b : 5; // 0
        uint32_t SCD_opcode_26_31 : 6; // 60
    };
    struct {
        uint32_t SD_f : 16; // 0
        uint32_t SD_t : 5; // 0
        uint32_t SD_b : 5; // 0
        uint32_t SD_opcode_26_31 : 6; // 63
    };
    struct {
        uint32_t SDL_f : 16; // 0
        uint32_t SDL_t : 5; // 0
        uint32_t SDL_b : 5; // 0
        uint32_t SDL_opcode_26_31 : 6; // 44
    };
    struct {
        uint32_t SDR_f : 16; // 0
        uint32_t SDR_t : 5; // 0
        uint32_t SDR_b : 5; // 0
        uint32_t SDR_opcode_26_31 : 6; // 45
    };
    struct {
        uint32_t SH_f : 16; // 0
        uint32_t SH_t : 5; // 0
        uint32_t SH_b : 5; // 0
        uint32_t SH_opcode_26_31 : 6; // 41
    };
    struct {
        uint32_t SLL_opcode_0_5 : 6; // 0
        uint32_t SLL_k : 5; // 0
        uint32_t SLL_d : 5; // 0
        uint32_t SLL_t : 5; // 0
        uint32_t SLL_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t SLLV_opcode_0_10 : 11; // 4
        uint32_t SLLV_d : 5; // 0
        uint32_t SLLV_t : 5; // 0
        uint32_t SLLV_s : 5; // 0
        uint32_t SLLV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SLT_opcode_0_10 : 11; // 42
        uint32_t SLT_d : 5; // 0
        uint32_t SLT_t : 5; // 0
        uint32_t SLT_s : 5; // 0
        uint32_t SLT_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SLTI_k : 16; // 0
        uint32_t SLTI_t : 5; // 0
        uint32_t SLTI_s : 5; // 0
        uint32_t SLTI_opcode_26_31 : 6; // 10
    };
    struct {
        uint32_t SLTIU_k : 16; // 0
        uint32_t SLTIU_t : 5; // 0
        uint32_t SLTIU_s : 5; // 0
        uint32_t SLTIU_opcode_26_31 : 6; // 11
    };
    struct {
        uint32_t SLTU_opcode_0_10 : 11; // 43
        uint32_t SLTU_d : 5; // 0
        uint32_t SLTU_t : 5; // 0
        uint32_t SLTU_s : 5; // 0
        uint32_t SLTU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SRA_opcode_0_5 : 6; // 3
        uint32_t SRA_k : 5; // 0
        uint32_t SRA_d : 5; // 0
        uint32_t SRA_t : 5; // 0
        uint32_t SRA_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t SRAV_opcode_0_10 : 11; // 7
        uint32_t SRAV_d : 5; // 0
        uint32_t SRAV_t : 5; // 0
        uint32_t SRAV_s : 5; // 0
        uint32_t SRAV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SRL_opcode_0_5 : 6; // 2
        uint32_t SRL_k : 5; // 0
        uint32_t SRL_d : 5; // 0
        uint32_t SRL_t : 5; // 0
        uint32_t SRL_opcode_21_31 : 11; // 0
    };
    struct {
        uint32_t SRLV_opcode_0_10 : 11; // 6
        uint32_t SRLV_d : 5; // 0
        uint32_t SRLV_t : 5; // 0
        uint32_t SRLV_s : 5; // 0
        uint32_t SRLV_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SUB_opcode_0_10 : 11; // 34
        uint32_t SUB_d : 5; // 0
        uint32_t SUB_t : 5; // 0
        uint32_t SUB_s : 5; // 0
        uint32_t SUB_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SUBU_opcode_0_10 : 11; // 35
        uint32_t SUBU_d : 5; // 0
        uint32_t SUBU_t : 5; // 0
        uint32_t SUBU_s : 5; // 0
        uint32_t SUBU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t SW_f : 16; // 0
        uint32_t SW_t : 5; // 0
        uint32_t SW_b : 5; // 0
        uint32_t SW_opcode_26_31 : 6; // 43
    };
    struct {
        uint32_t SWL_f : 16; // 0
        uint32_t SWL_t : 5; // 0
        uint32_t SWL_b : 5; // 0
        uint32_t SWL_opcode_26_31 : 6; // 42
    };
    struct {
        uint32_t SWR_f : 16; // 0
        uint32_t SWR_t : 5; // 0
        uint32_t SWR_b : 5; // 0
        uint32_t SWR_opcode_26_31 : 6; // 46
    };
    struct {
        uint32_t SYNC_opcode_0_31 : 32; // 15
    };
    struct {
        uint32_t SYSCALL_opcode_0_5 : 6; // 12
        uint32_t SYSCALL_k : 20; // 0
        uint32_t SYSCALL_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TEQ_opcode_0_5 : 6; // 52
        uint32_t TEQ_k : 10; // 0
        uint32_t TEQ_t : 5; // 0
        uint32_t TEQ_s : 5; // 0
        uint32_t TEQ_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TEQI_k : 16; // 0
        uint32_t TEQI_opcode_16_20 : 5; // 12
        uint32_t TEQI_s : 5; // 0
        uint32_t TEQI_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t TGE_opcode_0_5 : 6; // 48
        uint32_t TGE_k : 10; // 0
        uint32_t TGE_t : 5; // 0
        uint32_t TGE_s : 5; // 0
        uint32_t TGE_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TGEI_k : 16; // 0
        uint32_t TGEI_opcode_16_20 : 5; // 8
        uint32_t TGEI_s : 5; // 0
        uint32_t TGEI_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t TGEIU_k : 16; // 0
        uint32_t TGEIU_opcode_16_20 : 5; // 9
        uint32_t TGEIU_s : 5; // 0
        uint32_t TGEIU_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t TGEU_opcode_0_5 : 6; // 49
        uint32_t TGEU_k : 10; // 0
        uint32_t TGEU_t : 5; // 0
        uint32_t TGEU_s : 5; // 0
        uint32_t TGEU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TLBP_opcode_0_31 : 32; // 1107296264
    };
    struct {
        uint32_t TLBR_opcode_0_31 : 32; // 1107296257
    };
    struct {
        uint32_t TLBWI_opcode_0_31 : 32; // 1107296258
    };
    struct {
        uint32_t TLBWR_opcode_0_31 : 32; // 1107296262
    };
    struct {
        uint32_t TLT_opcode_0_5 : 6; // 50
        uint32_t TLT_k : 10; // 0
        uint32_t TLT_t : 5; // 0
        uint32_t TLT_s : 5; // 0
        uint32_t TLT_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TLTI_k : 16; // 0
        uint32_t TLTI_opcode_16_20 : 5; // 10
        uint32_t TLTI_s : 5; // 0
        uint32_t TLTI_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t TLTIU_k : 16; // 0
        uint32_t TLTIU_opcode_16_20 : 5; // 11
        uint32_t TLTIU_s : 5; // 0
        uint32_t TLTIU_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t TLTU_opcode_0_5 : 6; // 51
        uint32_t TLTU_k : 10; // 0
        uint32_t TLTU_t : 5; // 0
        uint32_t TLTU_s : 5; // 0
        uint32_t TLTU_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TNE_opcode_0_5 : 6; // 54
        uint32_t TNE_k : 10; // 0
        uint32_t TNE_t : 5; // 0
        uint32_t TNE_s : 5; // 0
        uint32_t TNE_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t TNEI_k : 16; // 0
        uint32_t TNEI_opcode_16_20 : 5; // 14
        uint32_t TNEI_s : 5; // 0
        uint32_t TNEI_opcode_26_31 : 6; // 1
    };
    struct {
        uint32_t XOR_opcode_0_10 : 11; // 38
        uint32_t XOR_d : 5; // 0
        uint32_t XOR_t : 5; // 0
        uint32_t XOR_s : 5; // 0
        uint32_t XOR_opcode_26_31 : 6; // 0
    };
    struct {
        uint32_t XORI_k : 16; // 0
        uint32_t XORI_t : 5; // 0
        uint32_t XORI_s : 5; // 0
        uint32_t XORI_opcode_26_31 : 6; // 14
    };
    struct {
        uint32_t BC0F_f : 16; // 0
        uint32_t BC0F_opcode_16_31 : 16; // 16640
    };
    struct {
        uint32_t BC1F_f : 16; // 0
        uint32_t BC1F_opcode_16_31 : 16; // 17664
    };
    struct {
        uint32_t BC0FL_f : 16; // 0
        uint32_t BC0FL_opcode_16_31 : 16; // 16642
    };
    struct {
        uint32_t BC1FL_f : 16; // 0
        uint32_t BC1FL_opcode_16_31 : 16; // 17666
    };
    struct {
        uint32_t BC0T_f : 16; // 0
        uint32_t BC0T_opcode_16_31 : 16; // 16641
    };
    struct {
        uint32_t BC1T_f : 16; // 0
        uint32_t BC1T_opcode_16_31 : 16; // 17665
    };
    struct {
        uint32_t BC0TL_f : 16; // 0
        uint32_t BC0TL_opcode_16_31 : 16; // 16643
    };
    struct {
        uint32_t BC1TL_f : 16; // 0
        uint32_t BC1TL_opcode_16_31 : 16; // 17667
    };
    struct {
        uint32_t ABS_S_opcode_0_5 : 6; // 5
        uint32_t ABS_S_d : 5; // 0
        uint32_t ABS_S_s : 5; // 0
        uint32_t ABS_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t ABS_W_opcode_0_5 : 6; // 5
        uint32_t ABS_W_d : 5; // 0
        uint32_t ABS_W_s : 5; // 0
        uint32_t ABS_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t ABS_D_opcode_0_5 : 6; // 5
        uint32_t ABS_D_d : 5; // 0
        uint32_t ABS_D_s : 5; // 0
        uint32_t ABS_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t ABS_L_opcode_0_5 : 6; // 5
        uint32_t ABS_L_d : 5; // 0
        uint32_t ABS_L_s : 5; // 0
        uint32_t ABS_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t ADD_S_opcode_0_5 : 6; // 0
        uint32_t ADD_S_d : 5; // 0
        uint32_t ADD_S_s : 5; // 0
        uint32_t ADD_S_t : 5; // 0
        uint32_t ADD_S_opcode_21_31 : 11; // 560
    };
    struct {
        uint32_t ADD_W_opcode_0_5 : 6; // 0
        uint32_t ADD_W_d : 5; // 0
        uint32_t ADD_W_s : 5; // 0
        uint32_t ADD_W_t : 5; // 0
        uint32_t ADD_W_opcode_21_31 : 11; // 564
    };
    struct {
        uint32_t ADD_D_opcode_0_5 : 6; // 0
        uint32_t ADD_D_d : 5; // 0
        uint32_t ADD_D_s : 5; // 0
        uint32_t ADD_D_t : 5; // 0
        uint32_t ADD_D_opcode_21_31 : 11; // 561
    };
    struct {
        uint32_t ADD_L_opcode_0_5 : 6; // 0
        uint32_t ADD_L_d : 5; // 0
        uint32_t ADD_L_s : 5; // 0
        uint32_t ADD_L_t : 5; // 0
        uint32_t ADD_L_opcode_21_31 : 11; // 565
    };
    struct {
        uint32_t C_cond_S_c : 4; // 0
        uint32_t C_cond_S_opcode_4_10 : 7; // 3
        uint32_t C_cond_S_s : 5; // 0
        uint32_t C_cond_S_t : 5; // 0
        uint32_t C_cond_S_opcode_21_31 : 11; // 560
    };
    struct {
        uint32_t C_cond_W_c : 4; // 0
        uint32_t C_cond_W_opcode_4_10 : 7; // 3
        uint32_t C_cond_W_s : 5; // 0
        uint32_t C_cond_W_t : 5; // 0
        uint32_t C_cond_W_opcode_21_31 : 11; // 564
    };
    struct {
        uint32_t C_cond_D_c : 4; // 0
        uint32_t C_cond_D_opcode_4_10 : 7; // 3
        uint32_t C_cond_D_s : 5; // 0
        uint32_t C_cond_D_t : 5; // 0
        uint32_t C_cond_D_opcode_21_31 : 11; // 561
    };
    struct {
        uint32_t C_cond_L_c : 4; // 0
        uint32_t C_cond_L_opcode_4_10 : 7; // 3
        uint32_t C_cond_L_s : 5; // 0
        uint32_t C_cond_L_t : 5; // 0
        uint32_t C_cond_L_opcode_21_31 : 11; // 565
    };
    struct {
        uint32_t CEIL_L_S_opcode_0_5 : 6; // 10
        uint32_t CEIL_L_S_d : 5; // 0
        uint32_t CEIL_L_S_s : 5; // 0
        uint32_t CEIL_L_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t CEIL_L_W_opcode_0_5 : 6; // 10
        uint32_t CEIL_L_W_d : 5; // 0
        uint32_t CEIL_L_W_s : 5; // 0
        uint32_t CEIL_L_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t CEIL_L_D_opcode_0_5 : 6; // 10
        uint32_t CEIL_L_D_d : 5; // 0
        uint32_t CEIL_L_D_s : 5; // 0
        uint32_t CEIL_L_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t CEIL_L_L_opcode_0_5 : 6; // 10
        uint32_t CEIL_L_L_d : 5; // 0
        uint32_t CEIL_L_L_s : 5; // 0
        uint32_t CEIL_L_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t CEIL_W_S_opcode_0_5 : 6; // 14
        uint32_t CEIL_W_S_d : 5; // 0
        uint32_t CEIL_W_S_s : 5; // 0
        uint32_t CEIL_W_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t CEIL_W_W_opcode_0_5 : 6; // 14
        uint32_t CEIL_W_W_d : 5; // 0
        uint32_t CEIL_W_W_s : 5; // 0
        uint32_t CEIL_W_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t CEIL_W_D_opcode_0_5 : 6; // 14
        uint32_t CEIL_W_D_d : 5; // 0
        uint32_t CEIL_W_D_s : 5; // 0
        uint32_t CEIL_W_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t CEIL_W_L_opcode_0_5 : 6; // 14
        uint32_t CEIL_W_L_d : 5; // 0
        uint32_t CEIL_W_L_s : 5; // 0
        uint32_t CEIL_W_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t CVT_D_S_opcode_0_5 : 6; // 33
        uint32_t CVT_D_S_d : 5; // 0
        uint32_t CVT_D_S_s : 5; // 0
        uint32_t CVT_D_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t CVT_D_W_opcode_0_5 : 6; // 33
        uint32_t CVT_D_W_d : 5; // 0
        uint32_t CVT_D_W_s : 5; // 0
        uint32_t CVT_D_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t CVT_D_D_opcode_0_5 : 6; // 33
        uint32_t CVT_D_D_d : 5; // 0
        uint32_t CVT_D_D_s : 5; // 0
        uint32_t CVT_D_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t CVT_D_L_opcode_0_5 : 6; // 33
        uint32_t CVT_D_L_d : 5; // 0
        uint32_t CVT_D_L_s : 5; // 0
        uint32_t CVT_D_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t CVT_L_S_opcode_0_5 : 6; // 37
        uint32_t CVT_L_S_d : 5; // 0
        uint32_t CVT_L_S_s : 5; // 0
        uint32_t CVT_L_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t CVT_L_W_opcode_0_5 : 6; // 37
        uint32_t CVT_L_W_d : 5; // 0
        uint32_t CVT_L_W_s : 5; // 0
        uint32_t CVT_L_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t CVT_L_D_opcode_0_5 : 6; // 37
        uint32_t CVT_L_D_d : 5; // 0
        uint32_t CVT_L_D_s : 5; // 0
        uint32_t CVT_L_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t CVT_L_L_opcode_0_5 : 6; // 37
        uint32_t CVT_L_L_d : 5; // 0
        uint32_t CVT_L_L_s : 5; // 0
        uint32_t CVT_L_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t CVT_S_S_opcode_0_5 : 6; // 32
        uint32_t CVT_S_S_d : 5; // 0
        uint32_t CVT_S_S_s : 5; // 0
        uint32_t CVT_S_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t CVT_S_W_opcode_0_5 : 6; // 32
        uint32_t CVT_S_W_d : 5; // 0
        uint32_t CVT_S_W_s : 5; // 0
        uint32_t CVT_S_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t CVT_S_D_opcode_0_5 : 6; // 32
        uint32_t CVT_S_D_d : 5; // 0
        uint32_t CVT_S_D_s : 5; // 0
        uint32_t CVT_S_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t CVT_S_L_opcode_0_5 : 6; // 32
        uint32_t CVT_S_L_d : 5; // 0
        uint32_t CVT_S_L_s : 5; // 0
        uint32_t CVT_S_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t CVT_W_S_opcode_0_5 : 6; // 36
        uint32_t CVT_W_S_d : 5; // 0
        uint32_t CVT_W_S_s : 5; // 0
        uint32_t CVT_W_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t CVT_W_W_opcode_0_5 : 6; // 36
        uint32_t CVT_W_W_d : 5; // 0
        uint32_t CVT_W_W_s : 5; // 0
        uint32_t CVT_W_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t CVT_W_D_opcode_0_5 : 6; // 36
        uint32_t CVT_W_D_d : 5; // 0
        uint32_t CVT_W_D_s : 5; // 0
        uint32_t CVT_W_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t CVT_W_L_opcode_0_5 : 6; // 36
        uint32_t CVT_W_L_d : 5; // 0
        uint32_t CVT_W_L_s : 5; // 0
        uint32_t CVT_W_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t DIV_S_opcode_0_5 : 6; // 3
        uint32_t DIV_S_d : 5; // 0
        uint32_t DIV_S_s : 5; // 0
        uint32_t DIV_S_t : 5; // 0
        uint32_t DIV_S_opcode_21_31 : 11; // 560
    };
    struct {
        uint32_t DIV_W_opcode_0_5 : 6; // 3
        uint32_t DIV_W_d : 5; // 0
        uint32_t DIV_W_s : 5; // 0
        uint32_t DIV_W_t : 5; // 0
        uint32_t DIV_W_opcode_21_31 : 11; // 564
    };
    struct {
        uint32_t DIV_D_opcode_0_5 : 6; // 3
        uint32_t DIV_D_d : 5; // 0
        uint32_t DIV_D_s : 5; // 0
        uint32_t DIV_D_t : 5; // 0
        uint32_t DIV_D_opcode_21_31 : 11; // 561
    };
    struct {
        uint32_t DIV_L_opcode_0_5 : 6; // 3
        uint32_t DIV_L_d : 5; // 0
        uint32_t DIV_L_s : 5; // 0
        uint32_t DIV_L_t : 5; // 0
        uint32_t DIV_L_opcode_21_31 : 11; // 565
    };
    struct {
        uint32_t FLOOR_L_S_opcode_0_5 : 6; // 11
        uint32_t FLOOR_L_S_d : 5; // 0
        uint32_t FLOOR_L_S_s : 5; // 0
        uint32_t FLOOR_L_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t FLOOR_L_W_opcode_0_5 : 6; // 11
        uint32_t FLOOR_L_W_d : 5; // 0
        uint32_t FLOOR_L_W_s : 5; // 0
        uint32_t FLOOR_L_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t FLOOR_L_D_opcode_0_5 : 6; // 11
        uint32_t FLOOR_L_D_d : 5; // 0
        uint32_t FLOOR_L_D_s : 5; // 0
        uint32_t FLOOR_L_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t FLOOR_L_L_opcode_0_5 : 6; // 11
        uint32_t FLOOR_L_L_d : 5; // 0
        uint32_t FLOOR_L_L_s : 5; // 0
        uint32_t FLOOR_L_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t FLOOR_W_S_opcode_0_5 : 6; // 15
        uint32_t FLOOR_W_S_d : 5; // 0
        uint32_t FLOOR_W_S_s : 5; // 0
        uint32_t FLOOR_W_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t FLOOR_W_W_opcode_0_5 : 6; // 15
        uint32_t FLOOR_W_W_d : 5; // 0
        uint32_t FLOOR_W_W_s : 5; // 0
        uint32_t FLOOR_W_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t FLOOR_W_D_opcode_0_5 : 6; // 15
        uint32_t FLOOR_W_D_d : 5; // 0
        uint32_t FLOOR_W_D_s : 5; // 0
        uint32_t FLOOR_W_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t FLOOR_W_L_opcode_0_5 : 6; // 15
        uint32_t FLOOR_W_L_d : 5; // 0
        uint32_t FLOOR_W_L_s : 5; // 0
        uint32_t FLOOR_W_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t LDC1_f : 16; // 0
        uint32_t LDC1_t : 5; // 0
        uint32_t LDC1_b : 5; // 0
        uint32_t LDC1_opcode_26_31 : 6; // 53
    };
    struct {
        uint32_t LWC1_f : 16; // 0
        uint32_t LWC1_t : 5; // 0
        uint32_t LWC1_b : 5; // 0
        uint32_t LWC1_opcode_26_31 : 6; // 49
    };
    struct {
        uint32_t MOV_S_opcode_0_5 : 6; // 6
        uint32_t MOV_S_d : 5; // 0
        uint32_t MOV_S_s : 5; // 0
        uint32_t MOV_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t MOV_W_opcode_0_5 : 6; // 6
        uint32_t MOV_W_d : 5; // 0
        uint32_t MOV_W_s : 5; // 0
        uint32_t MOV_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t MOV_D_opcode_0_5 : 6; // 6
        uint32_t MOV_D_d : 5; // 0
        uint32_t MOV_D_s : 5; // 0
        uint32_t MOV_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t MOV_L_opcode_0_5 : 6; // 6
        uint32_t MOV_L_d : 5; // 0
        uint32_t MOV_L_s : 5; // 0
        uint32_t MOV_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t MUL_S_opcode_0_5 : 6; // 2
        uint32_t MUL_S_d : 5; // 0
        uint32_t MUL_S_s : 5; // 0
        uint32_t MUL_S_t : 5; // 0
        uint32_t MUL_S_opcode_21_31 : 11; // 560
    };
    struct {
        uint32_t MUL_W_opcode_0_5 : 6; // 2
        uint32_t MUL_W_d : 5; // 0
        uint32_t MUL_W_s : 5; // 0
        uint32_t MUL_W_t : 5; // 0
        uint32_t MUL_W_opcode_21_31 : 11; // 564
    };
    struct {
        uint32_t MUL_D_opcode_0_5 : 6; // 2
        uint32_t MUL_D_d : 5; // 0
        uint32_t MUL_D_s : 5; // 0
        uint32_t MUL_D_t : 5; // 0
        uint32_t MUL_D_opcode_21_31 : 11; // 561
    };
    struct {
        uint32_t MUL_L_opcode_0_5 : 6; // 2
        uint32_t MUL_L_d : 5; // 0
        uint32_t MUL_L_s : 5; // 0
        uint32_t MUL_L_t : 5; // 0
        uint32_t MUL_L_opcode_21_31 : 11; // 565
    };
    struct {
        uint32_t NEG_S_opcode_0_5 : 6; // 7
        uint32_t NEG_S_d : 5; // 0
        uint32_t NEG_S_s : 5; // 0
        uint32_t NEG_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t NEG_W_opcode_0_5 : 6; // 7
        uint32_t NEG_W_d : 5; // 0
        uint32_t NEG_W_s : 5; // 0
        uint32_t NEG_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t NEG_D_opcode_0_5 : 6; // 7
        uint32_t NEG_D_d : 5; // 0
        uint32_t NEG_D_s : 5; // 0
        uint32_t NEG_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t NEG_L_opcode_0_5 : 6; // 7
        uint32_t NEG_L_d : 5; // 0
        uint32_t NEG_L_s : 5; // 0
        uint32_t NEG_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t ROUND_L_S_opcode_0_5 : 6; // 8
        uint32_t ROUND_L_S_d : 5; // 0
        uint32_t ROUND_L_S_s : 5; // 0
        uint32_t ROUND_L_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t ROUND_L_W_opcode_0_5 : 6; // 8
        uint32_t ROUND_L_W_d : 5; // 0
        uint32_t ROUND_L_W_s : 5; // 0
        uint32_t ROUND_L_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t ROUND_L_D_opcode_0_5 : 6; // 8
        uint32_t ROUND_L_D_d : 5; // 0
        uint32_t ROUND_L_D_s : 5; // 0
        uint32_t ROUND_L_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t ROUND_L_L_opcode_0_5 : 6; // 8
        uint32_t ROUND_L_L_d : 5; // 0
        uint32_t ROUND_L_L_s : 5; // 0
        uint32_t ROUND_L_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t ROUND_W_S_opcode_0_5 : 6; // 12
        uint32_t ROUND_W_S_d : 5; // 0
        uint32_t ROUND_W_S_s : 5; // 0
        uint32_t ROUND_W_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t ROUND_W_W_opcode_0_5 : 6; // 12
        uint32_t ROUND_W_W_d : 5; // 0
        uint32_t ROUND_W_W_s : 5; // 0
        uint32_t ROUND_W_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t ROUND_W_D_opcode_0_5 : 6; // 12
        uint32_t ROUND_W_D_d : 5; // 0
        uint32_t ROUND_W_D_s : 5; // 0
        uint32_t ROUND_W_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t ROUND_W_L_opcode_0_5 : 6; // 12
        uint32_t ROUND_W_L_d : 5; // 0
        uint32_t ROUND_W_L_s : 5; // 0
        uint32_t ROUND_W_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t SDC1_f : 16; // 0
        uint32_t SDC1_t : 5; // 0
        uint32_t SDC1_b : 5; // 0
        uint32_t SDC1_opcode_26_31 : 6; // 61
    };
    struct {
        uint32_t SQRT_S_opcode_0_5 : 6; // 4
        uint32_t SQRT_S_d : 5; // 0
        uint32_t SQRT_S_s : 5; // 0
        uint32_t SQRT_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t SQRT_W_opcode_0_5 : 6; // 4
        uint32_t SQRT_W_d : 5; // 0
        uint32_t SQRT_W_s : 5; // 0
        uint32_t SQRT_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t SQRT_D_opcode_0_5 : 6; // 4
        uint32_t SQRT_D_d : 5; // 0
        uint32_t SQRT_D_s : 5; // 0
        uint32_t SQRT_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t SQRT_L_opcode_0_5 : 6; // 4
        uint32_t SQRT_L_d : 5; // 0
        uint32_t SQRT_L_s : 5; // 0
        uint32_t SQRT_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t SUB_S_opcode_0_5 : 6; // 1
        uint32_t SUB_S_d : 5; // 0
        uint32_t SUB_S_s : 5; // 0
        uint32_t SUB_S_t : 5; // 0
        uint32_t SUB_S_opcode_21_31 : 11; // 560
    };
    struct {
        uint32_t SUB_W_opcode_0_5 : 6; // 1
        uint32_t SUB_W_d : 5; // 0
        uint32_t SUB_W_s : 5; // 0
        uint32_t SUB_W_t : 5; // 0
        uint32_t SUB_W_opcode_21_31 : 11; // 564
    };
    struct {
        uint32_t SUB_D_opcode_0_5 : 6; // 1
        uint32_t SUB_D_d : 5; // 0
        uint32_t SUB_D_s : 5; // 0
        uint32_t SUB_D_t : 5; // 0
        uint32_t SUB_D_opcode_21_31 : 11; // 561
    };
    struct {
        uint32_t SUB_L_opcode_0_5 : 6; // 1
        uint32_t SUB_L_d : 5; // 0
        uint32_t SUB_L_s : 5; // 0
        uint32_t SUB_L_t : 5; // 0
        uint32_t SUB_L_opcode_21_31 : 11; // 565
    };
    struct {
        uint32_t SWC1_f : 16; // 0
        uint32_t SWC1_t : 5; // 0
        uint32_t SWC1_b : 5; // 0
        uint32_t SWC1_opcode_26_31 : 6; // 57
    };
    struct {
        uint32_t TRUNC_L_S_opcode_0_5 : 6; // 9
        uint32_t TRUNC_L_S_d : 5; // 0
        uint32_t TRUNC_L_S_s : 5; // 0
        uint32_t TRUNC_L_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t TRUNC_L_W_opcode_0_5 : 6; // 9
        uint32_t TRUNC_L_W_d : 5; // 0
        uint32_t TRUNC_L_W_s : 5; // 0
        uint32_t TRUNC_L_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t TRUNC_L_D_opcode_0_5 : 6; // 9
        uint32_t TRUNC_L_D_d : 5; // 0
        uint32_t TRUNC_L_D_s : 5; // 0
        uint32_t TRUNC_L_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t TRUNC_L_L_opcode_0_5 : 6; // 9
        uint32_t TRUNC_L_L_d : 5; // 0
        uint32_t TRUNC_L_L_s : 5; // 0
        uint32_t TRUNC_L_L_opcode_16_31 : 16; // 18080
    };
    struct {
        uint32_t TRUNC_W_S_opcode_0_5 : 6; // 13
        uint32_t TRUNC_W_S_d : 5; // 0
        uint32_t TRUNC_W_S_s : 5; // 0
        uint32_t TRUNC_W_S_opcode_16_31 : 16; // 17920
    };
    struct {
        uint32_t TRUNC_W_W_opcode_0_5 : 6; // 13
        uint32_t TRUNC_W_W_d : 5; // 0
        uint32_t TRUNC_W_W_s : 5; // 0
        uint32_t TRUNC_W_W_opcode_16_31 : 16; // 18048
    };
    struct {
        uint32_t TRUNC_W_D_opcode_0_5 : 6; // 13
        uint32_t TRUNC_W_D_d : 5; // 0
        uint32_t TRUNC_W_D_s : 5; // 0
        uint32_t TRUNC_W_D_opcode_16_31 : 16; // 17952
    };
    struct {
        uint32_t TRUNC_W_L_opcode_0_5 : 6; // 13
        uint32_t TRUNC_W_L_d : 5; // 0
        uint32_t TRUNC_W_L_s : 5; // 0
        uint32_t TRUNC_W_L_opcode_16_31 : 16; // 18080
    };
};

#define ABS_D_opcode_0_5_val 5
#define ABS_D_opcode_16_31_val 1176502272
extern "C" void ABS_D(void);
#define ABS_L_opcode_0_5_val 5
#define ABS_L_opcode_16_31_val 1184890880
extern "C" void ABS_L(void);
#define ABS_S_opcode_0_5_val 5
#define ABS_S_opcode_16_31_val 1174405120
extern "C" void ABS_S(void);
#define ABS_W_opcode_0_5_val 5
#define ABS_W_opcode_16_31_val 1182793728
extern "C" void ABS_W(void);
#define ADD_opcode_0_10_val 32
#define ADD_opcode_26_31_val 0
extern "C" void ADD(void);
#define ADDI_opcode_26_31_val 536870912
extern "C" void ADDI(void);
#define ADDIU_opcode_26_31_val 603979776
extern "C" void ADDIU(void);
#define ADDU_opcode_0_10_val 33
#define ADDU_opcode_26_31_val 0
extern "C" void ADDU(void);
#define ADD_D_opcode_0_5_val 0
#define ADD_D_opcode_21_31_val 1176502272
extern "C" void ADD_D(void);
#define ADD_L_opcode_0_5_val 0
#define ADD_L_opcode_21_31_val 1184890880
extern "C" void ADD_L(void);
#define ADD_S_opcode_0_5_val 0
#define ADD_S_opcode_21_31_val 1174405120
extern "C" void ADD_S(void);
#define ADD_W_opcode_0_5_val 0
#define ADD_W_opcode_21_31_val 1182793728
extern "C" void ADD_W(void);
#define AND_opcode_0_10_val 36
#define AND_opcode_26_31_val 0
extern "C" void AND(void);
#define ANDI_opcode_26_31_val 805306368
extern "C" void ANDI(void);
#define BC0F_opcode_16_31_val 1090519040
extern "C" void BC0F(void);
#define BC0FL_opcode_16_31_val 1090650112
extern "C" void BC0FL(void);
#define BC0T_opcode_16_31_val 1090584576
extern "C" void BC0T(void);
#define BC0TL_opcode_16_31_val 1090715648
extern "C" void BC0TL(void);
#define BC1F_opcode_16_31_val 1157627904
extern "C" void BC1F(void);
#define BC1FL_opcode_16_31_val 1157758976
extern "C" void BC1FL(void);
#define BC1T_opcode_16_31_val 1157693440
extern "C" void BC1T(void);
#define BC1TL_opcode_16_31_val 1157824512
extern "C" void BC1TL(void);
#define BEQ_opcode_26_31_val 268435456
extern "C" void BEQ(void);
#define BEQL_opcode_26_31_val 1342177280
extern "C" void BEQL(void);
#define BGEZ_opcode_16_20_val 65536
#define BGEZ_opcode_26_31_val 67108864
extern "C" void BGEZ(void);
#define BGEZAL_opcode_16_20_val 1114112
#define BGEZAL_opcode_26_31_val 67108864
extern "C" void BGEZAL(void);
#define BGEZALL_opcode_16_20_val 1245184
#define BGEZALL_opcode_26_31_val 67108864
extern "C" void BGEZALL(void);
#define BGEZL_opcode_16_20_val 196608
#define BGEZL_opcode_26_31_val 67108864
extern "C" void BGEZL(void);
#define BGTZ_opcode_16_20_val 0
#define BGTZ_opcode_26_31_val 469762048
extern "C" void BGTZ(void);
#define BGTZL_opcode_16_20_val 0
#define BGTZL_opcode_26_31_val 1543503872
extern "C" void BGTZL(void);
#define BLEZ_opcode_16_20_val 0
#define BLEZ_opcode_26_31_val 402653184
extern "C" void BLEZ(void);
#define BLEZL_opcode_16_20_val 0
#define BLEZL_opcode_26_31_val 1476395008
extern "C" void BLEZL(void);
#define BLTZ_opcode_16_20_val 0
#define BLTZ_opcode_26_31_val 67108864
extern "C" void BLTZ(void);
#define BLTZAL_opcode_16_20_val 1048576
#define BLTZAL_opcode_26_31_val 67108864
extern "C" void BLTZAL(void);
#define BLTZALL_opcode_16_20_val 1179648
#define BLTZALL_opcode_26_31_val 67108864
extern "C" void BLTZALL(void);
#define BLTZL_opcode_16_20_val 131072
#define BLTZL_opcode_26_31_val 67108864
extern "C" void BLTZL(void);
#define BNE_opcode_26_31_val 335544320
extern "C" void BNE(void);
#define BNEL_opcode_26_31_val 1409286144
extern "C" void BNEL(void);
#define BREAK_opcode_0_5_val 13
#define BREAK_opcode_26_31_val 0
extern "C" void BREAK(void);
#define CACHE_opcode_26_31_val -1140850688
extern "C" void CACHE(void);
#define CEIL_L_D_opcode_0_5_val 10
#define CEIL_L_D_opcode_16_31_val 1176502272
extern "C" void CEIL_L_D(void);
#define CEIL_L_L_opcode_0_5_val 10
#define CEIL_L_L_opcode_16_31_val 1184890880
extern "C" void CEIL_L_L(void);
#define CEIL_L_S_opcode_0_5_val 10
#define CEIL_L_S_opcode_16_31_val 1174405120
extern "C" void CEIL_L_S(void);
#define CEIL_L_W_opcode_0_5_val 10
#define CEIL_L_W_opcode_16_31_val 1182793728
extern "C" void CEIL_L_W(void);
#define CEIL_W_D_opcode_0_5_val 14
#define CEIL_W_D_opcode_16_31_val 1176502272
extern "C" void CEIL_W_D(void);
#define CEIL_W_L_opcode_0_5_val 14
#define CEIL_W_L_opcode_16_31_val 1184890880
extern "C" void CEIL_W_L(void);
#define CEIL_W_S_opcode_0_5_val 14
#define CEIL_W_S_opcode_16_31_val 1174405120
extern "C" void CEIL_W_S(void);
#define CEIL_W_W_opcode_0_5_val 14
#define CEIL_W_W_opcode_16_31_val 1182793728
extern "C" void CEIL_W_W(void);
#define CFC0_opcode_0_10_val 0
#define CFC0_opcode_21_31_val 1077936128
extern "C" void CFC0(void);
#define CFC1_opcode_0_10_val 0
#define CFC1_opcode_21_31_val 1145044992
extern "C" void CFC1(void);
#define CFC2_opcode_0_10_val 0
#define CFC2_opcode_21_31_val 1212153856
extern "C" void CFC2(void);
#define CTC0_opcode_0_10_val 0
#define CTC0_opcode_21_31_val 1086324736
extern "C" void CTC0(void);
#define CTC1_opcode_0_10_val 0
#define CTC1_opcode_21_31_val 1153433600
extern "C" void CTC1(void);
#define CTC2_opcode_0_10_val 0
#define CTC2_opcode_21_31_val 1220542464
extern "C" void CTC2(void);
#define CVT_D_D_opcode_0_5_val 33
#define CVT_D_D_opcode_16_31_val 1176502272
extern "C" void CVT_D_D(void);
#define CVT_D_L_opcode_0_5_val 33
#define CVT_D_L_opcode_16_31_val 1184890880
extern "C" void CVT_D_L(void);
#define CVT_D_S_opcode_0_5_val 33
#define CVT_D_S_opcode_16_31_val 1174405120
extern "C" void CVT_D_S(void);
#define CVT_D_W_opcode_0_5_val 33
#define CVT_D_W_opcode_16_31_val 1182793728
extern "C" void CVT_D_W(void);
#define CVT_L_D_opcode_0_5_val 37
#define CVT_L_D_opcode_16_31_val 1176502272
extern "C" void CVT_L_D(void);
#define CVT_L_L_opcode_0_5_val 37
#define CVT_L_L_opcode_16_31_val 1184890880
extern "C" void CVT_L_L(void);
#define CVT_L_S_opcode_0_5_val 37
#define CVT_L_S_opcode_16_31_val 1174405120
extern "C" void CVT_L_S(void);
#define CVT_L_W_opcode_0_5_val 37
#define CVT_L_W_opcode_16_31_val 1182793728
extern "C" void CVT_L_W(void);
#define CVT_S_D_opcode_0_5_val 32
#define CVT_S_D_opcode_16_31_val 1176502272
extern "C" void CVT_S_D(void);
#define CVT_S_L_opcode_0_5_val 32
#define CVT_S_L_opcode_16_31_val 1184890880
extern "C" void CVT_S_L(void);
#define CVT_S_S_opcode_0_5_val 32
#define CVT_S_S_opcode_16_31_val 1174405120
extern "C" void CVT_S_S(void);
#define CVT_S_W_opcode_0_5_val 32
#define CVT_S_W_opcode_16_31_val 1182793728
extern "C" void CVT_S_W(void);
#define CVT_W_D_opcode_0_5_val 36
#define CVT_W_D_opcode_16_31_val 1176502272
extern "C" void CVT_W_D(void);
#define CVT_W_L_opcode_0_5_val 36
#define CVT_W_L_opcode_16_31_val 1184890880
extern "C" void CVT_W_L(void);
#define CVT_W_S_opcode_0_5_val 36
#define CVT_W_S_opcode_16_31_val 1174405120
extern "C" void CVT_W_S(void);
#define CVT_W_W_opcode_0_5_val 36
#define CVT_W_W_opcode_16_31_val 1182793728
extern "C" void CVT_W_W(void);
#define C_cond_D_opcode_4_10_val 48
#define C_cond_D_opcode_21_31_val 1176502272
extern "C" void C_cond_D(void);
#define C_cond_L_opcode_4_10_val 48
#define C_cond_L_opcode_21_31_val 1184890880
extern "C" void C_cond_L(void);
#define C_cond_S_opcode_4_10_val 48
#define C_cond_S_opcode_21_31_val 1174405120
extern "C" void C_cond_S(void);
#define C_cond_W_opcode_4_10_val 48
#define C_cond_W_opcode_21_31_val 1182793728
extern "C" void C_cond_W(void);
#define DADD_opcode_0_10_val 44
#define DADD_opcode_26_31_val 0
extern "C" void DADD(void);
#define DADDI_opcode_26_31_val 1610612736
extern "C" void DADDI(void);
#define DADDIU_opcode_26_31_val 1677721600
extern "C" void DADDIU(void);
#define DADDU_opcode_0_10_val 45
#define DADDU_opcode_26_31_val 0
extern "C" void DADDU(void);
#define DCFC1_opcode_0_10_val 0
#define DCFC1_opcode_21_31_val 1147142144
extern "C" void DCFC1(void);
#define DCFC2_opcode_0_10_val 0
#define DCFC2_opcode_21_31_val 1214251008
extern "C" void DCFC2(void);
#define DCTC1_opcode_0_10_val 0
#define DCTC1_opcode_21_31_val 1155530752
extern "C" void DCTC1(void);
#define DCTC2_opcode_0_10_val 0
#define DCTC2_opcode_21_31_val 1222639616
extern "C" void DCTC2(void);
#define DDIV_opcode_0_15_val 30
#define DDIV_opcode_26_31_val 0
extern "C" void DDIV(void);
#define DDIVU_opcode_0_15_val 31
#define DDIVU_opcode_26_31_val 0
extern "C" void DDIVU(void);
#define DIV_opcode_0_15_val 26
#define DIV_opcode_26_31_val 0
extern "C" void DIV(void);
#define DIVU_opcode_0_15_val 27
#define DIVU_opcode_26_31_val 0
extern "C" void DIVU(void);
#define DIV_D_opcode_0_5_val 3
#define DIV_D_opcode_21_31_val 1176502272
extern "C" void DIV_D(void);
#define DIV_L_opcode_0_5_val 3
#define DIV_L_opcode_21_31_val 1184890880
extern "C" void DIV_L(void);
#define DIV_S_opcode_0_5_val 3
#define DIV_S_opcode_21_31_val 1174405120
extern "C" void DIV_S(void);
#define DIV_W_opcode_0_5_val 3
#define DIV_W_opcode_21_31_val 1182793728
extern "C" void DIV_W(void);
#define DMFC0_opcode_0_10_val 0
#define DMFC0_opcode_21_31_val 1075838976
extern "C" void DMFC0(void);
#define DMFC1_opcode_0_10_val 0
#define DMFC1_opcode_21_31_val 1142947840
extern "C" void DMFC1(void);
#define DMFC2_opcode_0_10_val 0
#define DMFC2_opcode_21_31_val 1210056704
extern "C" void DMFC2(void);
#define DMTC0_opcode_0_10_val 0
#define DMTC0_opcode_21_31_val 1084227584
extern "C" void DMTC0(void);
#define DMTC1_opcode_0_10_val 0
#define DMTC1_opcode_21_31_val 1151336448
extern "C" void DMTC1(void);
#define DMTC2_opcode_0_10_val 0
#define DMTC2_opcode_21_31_val 1218445312
extern "C" void DMTC2(void);
#define DMULT_opcode_0_15_val 28
#define DMULT_opcode_26_31_val 0
extern "C" void DMULT(void);
#define DMULTU_opcode_0_15_val 29
#define DMULTU_opcode_26_31_val 0
extern "C" void DMULTU(void);
#define DSLL_opcode_0_5_val 56
#define DSLL_opcode_21_31_val 0
extern "C" void DSLL(void);
#define DSLL32_opcode_0_5_val 60
#define DSLL32_opcode_21_31_val 0
extern "C" void DSLL32(void);
#define DSLLV_opcode_0_10_val 20
#define DSLLV_opcode_26_31_val 0
extern "C" void DSLLV(void);
#define DSRA_opcode_0_5_val 59
#define DSRA_opcode_21_31_val 0
extern "C" void DSRA(void);
#define DSRA32_opcode_0_5_val 63
#define DSRA32_opcode_21_31_val 0
extern "C" void DSRA32(void);
#define DSRAV_opcode_0_10_val 23
#define DSRAV_opcode_26_31_val 0
extern "C" void DSRAV(void);
#define DSRL_opcode_0_5_val 58
#define DSRL_opcode_21_31_val 0
extern "C" void DSRL(void);
#define DSRL32_opcode_0_5_val 62
#define DSRL32_opcode_21_31_val 0
extern "C" void DSRL32(void);
#define DSRLV_opcode_0_10_val 22
#define DSRLV_opcode_26_31_val 0
extern "C" void DSRLV(void);
#define DSUB_opcode_0_10_val 46
#define DSUB_opcode_26_31_val 0
extern "C" void DSUB(void);
#define DSUBU_opcode_0_10_val 47
#define DSUBU_opcode_26_31_val 0
extern "C" void DSUBU(void);
#define ERET_opcode_0_31_val 1107296280
extern "C" void ERET(void);
#define FLOOR_L_D_opcode_0_5_val 11
#define FLOOR_L_D_opcode_16_31_val 1176502272
extern "C" void FLOOR_L_D(void);
#define FLOOR_L_L_opcode_0_5_val 11
#define FLOOR_L_L_opcode_16_31_val 1184890880
extern "C" void FLOOR_L_L(void);
#define FLOOR_L_S_opcode_0_5_val 11
#define FLOOR_L_S_opcode_16_31_val 1174405120
extern "C" void FLOOR_L_S(void);
#define FLOOR_L_W_opcode_0_5_val 11
#define FLOOR_L_W_opcode_16_31_val 1182793728
extern "C" void FLOOR_L_W(void);
#define FLOOR_W_D_opcode_0_5_val 15
#define FLOOR_W_D_opcode_16_31_val 1176502272
extern "C" void FLOOR_W_D(void);
#define FLOOR_W_L_opcode_0_5_val 15
#define FLOOR_W_L_opcode_16_31_val 1184890880
extern "C" void FLOOR_W_L(void);
#define FLOOR_W_S_opcode_0_5_val 15
#define FLOOR_W_S_opcode_16_31_val 1174405120
extern "C" void FLOOR_W_S(void);
#define FLOOR_W_W_opcode_0_5_val 15
#define FLOOR_W_W_opcode_16_31_val 1182793728
extern "C" void FLOOR_W_W(void);
#define J_opcode_26_31_val 134217728
extern "C" void J(void);
#define JAL_opcode_26_31_val 201326592
extern "C" void JAL(void);
#define JALR_opcode_0_10_val 9
#define JALR_opcode_16_20_val 0
#define JALR_opcode_26_31_val 0
extern "C" void JALR(void);
#define JR_opcode_0_20_val 8
#define JR_opcode_26_31_val 0
extern "C" void JR(void);
#define LB_opcode_26_31_val -2147483648
extern "C" void LB(void);
#define LBU_opcode_26_31_val -1879048192
extern "C" void LBU(void);
#define LD_opcode_26_31_val -603979776
extern "C" void LD(void);
#define LDC1_opcode_26_31_val -738197504
extern "C" void LDC1(void);
#define LDL_opcode_26_31_val 1744830464
extern "C" void LDL(void);
#define LDR_opcode_26_31_val 1811939328
extern "C" void LDR(void);
#define LH_opcode_26_31_val -2080374784
extern "C" void LH(void);
#define LHU_opcode_26_31_val -1811939328
extern "C" void LHU(void);
#define LL_opcode_26_31_val -1073741824
extern "C" void LL(void);
#define LLD_opcode_26_31_val -805306368
extern "C" void LLD(void);
#define LUI_opcode_21_31_val 1006632960
extern "C" void LUI(void);
#define LW_opcode_26_31_val -1946157056
extern "C" void LW(void);
#define LWC1_opcode_26_31_val -1006632960
extern "C" void LWC1(void);
#define LWL_opcode_26_31_val -2013265920
extern "C" void LWL(void);
#define LWR_opcode_26_31_val -1744830464
extern "C" void LWR(void);
#define LWU_opcode_26_31_val -1677721600
extern "C" void LWU(void);
#define MFC0_opcode_0_10_val 0
#define MFC0_opcode_21_31_val 1073741824
extern "C" void MFC0(void);
#define MFC1_opcode_0_10_val 0
#define MFC1_opcode_21_31_val 1140850688
extern "C" void MFC1(void);
#define MFC2_opcode_0_10_val 0
#define MFC2_opcode_21_31_val 1207959552
extern "C" void MFC2(void);
#define MFHI_opcode_0_10_val 16
#define MFHI_opcode_16_31_val 0
extern "C" void MFHI(void);
#define MFLO_opcode_0_10_val 18
#define MFLO_opcode_16_31_val 0
extern "C" void MFLO(void);
#define MOV_D_opcode_0_5_val 6
#define MOV_D_opcode_16_31_val 1176502272
extern "C" void MOV_D(void);
#define MOV_L_opcode_0_5_val 6
#define MOV_L_opcode_16_31_val 1184890880
extern "C" void MOV_L(void);
#define MOV_S_opcode_0_5_val 6
#define MOV_S_opcode_16_31_val 1174405120
extern "C" void MOV_S(void);
#define MOV_W_opcode_0_5_val 6
#define MOV_W_opcode_16_31_val 1182793728
extern "C" void MOV_W(void);
#define MTC0_opcode_0_10_val 0
#define MTC0_opcode_21_31_val 1082130432
extern "C" void MTC0(void);
#define MTC1_opcode_0_10_val 0
#define MTC1_opcode_21_31_val 1149239296
extern "C" void MTC1(void);
#define MTC2_opcode_0_10_val 0
#define MTC2_opcode_21_31_val 1216348160
extern "C" void MTC2(void);
#define MTHI_opcode_0_20_val 17
#define MTHI_opcode_26_31_val 0
extern "C" void MTHI(void);
#define MTLO_opcode_0_20_val 19
#define MTLO_opcode_26_31_val 0
extern "C" void MTLO(void);
#define MULT_opcode_0_15_val 24
#define MULT_opcode_26_31_val 0
extern "C" void MULT(void);
#define MULTU_opcode_0_15_val 25
#define MULTU_opcode_26_31_val 0
extern "C" void MULTU(void);
#define MUL_D_opcode_0_5_val 2
#define MUL_D_opcode_21_31_val 1176502272
extern "C" void MUL_D(void);
#define MUL_L_opcode_0_5_val 2
#define MUL_L_opcode_21_31_val 1184890880
extern "C" void MUL_L(void);
#define MUL_S_opcode_0_5_val 2
#define MUL_S_opcode_21_31_val 1174405120
extern "C" void MUL_S(void);
#define MUL_W_opcode_0_5_val 2
#define MUL_W_opcode_21_31_val 1182793728
extern "C" void MUL_W(void);
#define NEG_D_opcode_0_5_val 7
#define NEG_D_opcode_16_31_val 1176502272
extern "C" void NEG_D(void);
#define NEG_L_opcode_0_5_val 7
#define NEG_L_opcode_16_31_val 1184890880
extern "C" void NEG_L(void);
#define NEG_S_opcode_0_5_val 7
#define NEG_S_opcode_16_31_val 1174405120
extern "C" void NEG_S(void);
#define NEG_W_opcode_0_5_val 7
#define NEG_W_opcode_16_31_val 1182793728
extern "C" void NEG_W(void);
#define NOR_opcode_0_10_val 39
#define NOR_opcode_26_31_val 0
extern "C" void NOR(void);
#define OR_opcode_0_10_val 37
#define OR_opcode_26_31_val 0
extern "C" void OR(void);
#define ORI_opcode_26_31_val 872415232
extern "C" void ORI(void);
#define ROUND_L_D_opcode_0_5_val 8
#define ROUND_L_D_opcode_16_31_val 1176502272
extern "C" void ROUND_L_D(void);
#define ROUND_L_L_opcode_0_5_val 8
#define ROUND_L_L_opcode_16_31_val 1184890880
extern "C" void ROUND_L_L(void);
#define ROUND_L_S_opcode_0_5_val 8
#define ROUND_L_S_opcode_16_31_val 1174405120
extern "C" void ROUND_L_S(void);
#define ROUND_L_W_opcode_0_5_val 8
#define ROUND_L_W_opcode_16_31_val 1182793728
extern "C" void ROUND_L_W(void);
#define ROUND_W_D_opcode_0_5_val 12
#define ROUND_W_D_opcode_16_31_val 1176502272
extern "C" void ROUND_W_D(void);
#define ROUND_W_L_opcode_0_5_val 12
#define ROUND_W_L_opcode_16_31_val 1184890880
extern "C" void ROUND_W_L(void);
#define ROUND_W_S_opcode_0_5_val 12
#define ROUND_W_S_opcode_16_31_val 1174405120
extern "C" void ROUND_W_S(void);
#define ROUND_W_W_opcode_0_5_val 12
#define ROUND_W_W_opcode_16_31_val 1182793728
extern "C" void ROUND_W_W(void);
#define SB_opcode_26_31_val -1610612736
extern "C" void SB(void);
#define SC_opcode_26_31_val -536870912
extern "C" void SC(void);
#define SCD_opcode_26_31_val -268435456
extern "C" void SCD(void);
#define SD_opcode_26_31_val -67108864
extern "C" void SD(void);
#define SDC1_opcode_26_31_val -201326592
extern "C" void SDC1(void);
#define SDL_opcode_26_31_val -1342177280
extern "C" void SDL(void);
#define SDR_opcode_26_31_val -1275068416
extern "C" void SDR(void);
#define SH_opcode_26_31_val -1543503872
extern "C" void SH(void);
#define SLL_opcode_0_5_val 0
#define SLL_opcode_21_31_val 0
extern "C" void SLL(void);
#define SLLV_opcode_0_10_val 4
#define SLLV_opcode_26_31_val 0
extern "C" void SLLV(void);
#define SLT_opcode_0_10_val 42
#define SLT_opcode_26_31_val 0
extern "C" void SLT(void);
#define SLTI_opcode_26_31_val 671088640
extern "C" void SLTI(void);
#define SLTIU_opcode_26_31_val 738197504
extern "C" void SLTIU(void);
#define SLTU_opcode_0_10_val 43
#define SLTU_opcode_26_31_val 0
extern "C" void SLTU(void);
#define SQRT_D_opcode_0_5_val 4
#define SQRT_D_opcode_16_31_val 1176502272
extern "C" void SQRT_D(void);
#define SQRT_L_opcode_0_5_val 4
#define SQRT_L_opcode_16_31_val 1184890880
extern "C" void SQRT_L(void);
#define SQRT_S_opcode_0_5_val 4
#define SQRT_S_opcode_16_31_val 1174405120
extern "C" void SQRT_S(void);
#define SQRT_W_opcode_0_5_val 4
#define SQRT_W_opcode_16_31_val 1182793728
extern "C" void SQRT_W(void);
#define SRA_opcode_0_5_val 3
#define SRA_opcode_21_31_val 0
extern "C" void SRA(void);
#define SRAV_opcode_0_10_val 7
#define SRAV_opcode_26_31_val 0
extern "C" void SRAV(void);
#define SRL_opcode_0_5_val 2
#define SRL_opcode_21_31_val 0
extern "C" void SRL(void);
#define SRLV_opcode_0_10_val 6
#define SRLV_opcode_26_31_val 0
extern "C" void SRLV(void);
#define SUB_opcode_0_10_val 34
#define SUB_opcode_26_31_val 0
extern "C" void SUB(void);
#define SUBU_opcode_0_10_val 35
#define SUBU_opcode_26_31_val 0
extern "C" void SUBU(void);
#define SUB_D_opcode_0_5_val 1
#define SUB_D_opcode_21_31_val 1176502272
extern "C" void SUB_D(void);
#define SUB_L_opcode_0_5_val 1
#define SUB_L_opcode_21_31_val 1184890880
extern "C" void SUB_L(void);
#define SUB_S_opcode_0_5_val 1
#define SUB_S_opcode_21_31_val 1174405120
extern "C" void SUB_S(void);
#define SUB_W_opcode_0_5_val 1
#define SUB_W_opcode_21_31_val 1182793728
extern "C" void SUB_W(void);
#define SW_opcode_26_31_val -1409286144
extern "C" void SW(void);
#define SWC1_opcode_26_31_val -469762048
extern "C" void SWC1(void);
#define SWL_opcode_26_31_val -1476395008
extern "C" void SWL(void);
#define SWR_opcode_26_31_val -1207959552
extern "C" void SWR(void);
#define SYNC_opcode_0_31_val 15
extern "C" void SYNC(void);
#define SYSCALL_opcode_0_5_val 12
#define SYSCALL_opcode_26_31_val 0
extern "C" void SYSCALL(void);
#define TEQ_opcode_0_5_val 52
#define TEQ_opcode_26_31_val 0
extern "C" void TEQ(void);
#define TEQI_opcode_16_20_val 786432
#define TEQI_opcode_26_31_val 67108864
extern "C" void TEQI(void);
#define TGE_opcode_0_5_val 48
#define TGE_opcode_26_31_val 0
extern "C" void TGE(void);
#define TGEI_opcode_16_20_val 524288
#define TGEI_opcode_26_31_val 67108864
extern "C" void TGEI(void);
#define TGEIU_opcode_16_20_val 589824
#define TGEIU_opcode_26_31_val 67108864
extern "C" void TGEIU(void);
#define TGEU_opcode_0_5_val 49
#define TGEU_opcode_26_31_val 0
extern "C" void TGEU(void);
#define TLBP_opcode_0_31_val 1107296264
extern "C" void TLBP(void);
#define TLBR_opcode_0_31_val 1107296257
extern "C" void TLBR(void);
#define TLBWI_opcode_0_31_val 1107296258
extern "C" void TLBWI(void);
#define TLBWR_opcode_0_31_val 1107296262
extern "C" void TLBWR(void);
#define TLT_opcode_0_5_val 50
#define TLT_opcode_26_31_val 0
extern "C" void TLT(void);
#define TLTI_opcode_16_20_val 655360
#define TLTI_opcode_26_31_val 67108864
extern "C" void TLTI(void);
#define TLTIU_opcode_16_20_val 720896
#define TLTIU_opcode_26_31_val 67108864
extern "C" void TLTIU(void);
#define TLTU_opcode_0_5_val 51
#define TLTU_opcode_26_31_val 0
extern "C" void TLTU(void);
#define TNE_opcode_0_5_val 54
#define TNE_opcode_26_31_val 0
extern "C" void TNE(void);
#define TNEI_opcode_16_20_val 917504
#define TNEI_opcode_26_31_val 67108864
extern "C" void TNEI(void);
#define TRUNC_L_D_opcode_0_5_val 9
#define TRUNC_L_D_opcode_16_31_val 1176502272
extern "C" void TRUNC_L_D(void);
#define TRUNC_L_L_opcode_0_5_val 9
#define TRUNC_L_L_opcode_16_31_val 1184890880
extern "C" void TRUNC_L_L(void);
#define TRUNC_L_S_opcode_0_5_val 9
#define TRUNC_L_S_opcode_16_31_val 1174405120
extern "C" void TRUNC_L_S(void);
#define TRUNC_L_W_opcode_0_5_val 9
#define TRUNC_L_W_opcode_16_31_val 1182793728
extern "C" void TRUNC_L_W(void);
#define TRUNC_W_D_opcode_0_5_val 13
#define TRUNC_W_D_opcode_16_31_val 1176502272
extern "C" void TRUNC_W_D(void);
#define TRUNC_W_L_opcode_0_5_val 13
#define TRUNC_W_L_opcode_16_31_val 1184890880
extern "C" void TRUNC_W_L(void);
#define TRUNC_W_S_opcode_0_5_val 13
#define TRUNC_W_S_opcode_16_31_val 1174405120
extern "C" void TRUNC_W_S(void);
#define TRUNC_W_W_opcode_0_5_val 13
#define TRUNC_W_W_opcode_16_31_val 1182793728
extern "C" void TRUNC_W_W(void);
#define XOR_opcode_0_10_val 38
#define XOR_opcode_26_31_val 0
extern "C" void XOR(void);
#define XORI_opcode_26_31_val 939524096
extern "C" void XORI(void);

#define OP_MASK_0_10 2047
#define OP_MASK_0_15 65535
#define OP_MASK_0_20 2097151
#define OP_MASK_0_31 4294967295
#define OP_MASK_0_5 63
#define OP_MASK_16_20 2031616
#define OP_MASK_16_31 4294901760
#define OP_MASK_21_31 4292870144
#define OP_MASK_26_31 4227858432
#define OP_MASK_4_10 2032
#define VR4300_OP_ABS_D 0
#define VR4300_OP_ABS_L 1
#define VR4300_OP_ABS_S 2
#define VR4300_OP_ABS_W 3
#define VR4300_OP_ADD 4
#define VR4300_OP_ADDI 5
#define VR4300_OP_ADDIU 6
#define VR4300_OP_ADDU 7
#define VR4300_OP_ADD_D 8
#define VR4300_OP_ADD_L 9
#define VR4300_OP_ADD_S 10
#define VR4300_OP_ADD_W 11
#define VR4300_OP_AND 12
#define VR4300_OP_ANDI 13
#define VR4300_OP_BC0F 14
#define VR4300_OP_BC0FL 15
#define VR4300_OP_BC0T 16
#define VR4300_OP_BC0TL 17
#define VR4300_OP_BC1F 18
#define VR4300_OP_BC1FL 19
#define VR4300_OP_BC1T 20
#define VR4300_OP_BC1TL 21
#define VR4300_OP_BEQ 22
#define VR4300_OP_BEQL 23
#define VR4300_OP_BGEZ 24
#define VR4300_OP_BGEZAL 25
#define VR4300_OP_BGEZALL 26
#define VR4300_OP_BGEZL 27
#define VR4300_OP_BGTZ 28
#define VR4300_OP_BGTZL 29
#define VR4300_OP_BLEZ 30
#define VR4300_OP_BLEZL 31
#define VR4300_OP_BLTZ 32
#define VR4300_OP_BLTZAL 33
#define VR4300_OP_BLTZALL 34
#define VR4300_OP_BLTZL 35
#define VR4300_OP_BNE 36
#define VR4300_OP_BNEL 37
#define VR4300_OP_BREAK 38
#define VR4300_OP_CACHE 39
#define VR4300_OP_CEIL_L_D 40
#define VR4300_OP_CEIL_L_L 41
#define VR4300_OP_CEIL_L_S 42
#define VR4300_OP_CEIL_L_W 43
#define VR4300_OP_CEIL_W_D 44
#define VR4300_OP_CEIL_W_L 45
#define VR4300_OP_CEIL_W_S 46
#define VR4300_OP_CEIL_W_W 47
#define VR4300_OP_CFC0 48
#define VR4300_OP_CFC1 49
#define VR4300_OP_CFC2 50
#define VR4300_OP_CTC0 51
#define VR4300_OP_CTC1 52
#define VR4300_OP_CTC2 53
#define VR4300_OP_CVT_D_D 54
#define VR4300_OP_CVT_D_L 55
#define VR4300_OP_CVT_D_S 56
#define VR4300_OP_CVT_D_W 57
#define VR4300_OP_CVT_L_D 58
#define VR4300_OP_CVT_L_L 59
#define VR4300_OP_CVT_L_S 60
#define VR4300_OP_CVT_L_W 61
#define VR4300_OP_CVT_S_D 62
#define VR4300_OP_CVT_S_L 63
#define VR4300_OP_CVT_S_S 64
#define VR4300_OP_CVT_S_W 65
#define VR4300_OP_CVT_W_D 66
#define VR4300_OP_CVT_W_L 67
#define VR4300_OP_CVT_W_S 68
#define VR4300_OP_CVT_W_W 69
#define VR4300_OP_C_cond_D 70
#define VR4300_OP_C_cond_L 71
#define VR4300_OP_C_cond_S 72
#define VR4300_OP_C_cond_W 73
#define VR4300_OP_DADD 74
#define VR4300_OP_DADDI 75
#define VR4300_OP_DADDIU 76
#define VR4300_OP_DADDU 77
#define VR4300_OP_DCFC1 78
#define VR4300_OP_DCFC2 79
#define VR4300_OP_DCTC1 80
#define VR4300_OP_DCTC2 81
#define VR4300_OP_DDIV 82
#define VR4300_OP_DDIVU 83
#define VR4300_OP_DIV 84
#define VR4300_OP_DIVU 85
#define VR4300_OP_DIV_D 86
#define VR4300_OP_DIV_L 87
#define VR4300_OP_DIV_S 88
#define VR4300_OP_DIV_W 89
#define VR4300_OP_DMFC0 90
#define VR4300_OP_DMFC1 91
#define VR4300_OP_DMFC2 92
#define VR4300_OP_DMTC0 93
#define VR4300_OP_DMTC1 94
#define VR4300_OP_DMTC2 95
#define VR4300_OP_DMULT 96
#define VR4300_OP_DMULTU 97
#define VR4300_OP_DSLL 98
#define VR4300_OP_DSLL32 99
#define VR4300_OP_DSLLV 100
#define VR4300_OP_DSRA 101
#define VR4300_OP_DSRA32 102
#define VR4300_OP_DSRAV 103
#define VR4300_OP_DSRL 104
#define VR4300_OP_DSRL32 105
#define VR4300_OP_DSRLV 106
#define VR4300_OP_DSUB 107
#define VR4300_OP_DSUBU 108
#define VR4300_OP_ERET 109
#define VR4300_OP_FLOOR_L_D 110
#define VR4300_OP_FLOOR_L_L 111
#define VR4300_OP_FLOOR_L_S 112
#define VR4300_OP_FLOOR_L_W 113
#define VR4300_OP_FLOOR_W_D 114
#define VR4300_OP_FLOOR_W_L 115
#define VR4300_OP_FLOOR_W_S 116
#define VR4300_OP_FLOOR_W_W 117
#define VR4300_OP_J 118
#define VR4300_OP_JAL 119
#define VR4300_OP_JALR 120
#define VR4300_OP_JR 121
#define VR4300_OP_LB 122
#define VR4300_OP_LBU 123
#define VR4300_OP_LD 124
#define VR4300_OP_LDC1 125
#define VR4300_OP_LDL 126
#define VR4300_OP_LDR 127
#define VR4300_OP_LH 128
#define VR4300_OP_LHU 129
#define VR4300_OP_LL 130
#define VR4300_OP_LLD 131
#define VR4300_OP_LUI 132
#define VR4300_OP_LW 133
#define VR4300_OP_LWC1 134
#define VR4300_OP_LWL 135
#define VR4300_OP_LWR 136
#define VR4300_OP_LWU 137
#define VR4300_OP_MFC0 138
#define VR4300_OP_MFC1 139
#define VR4300_OP_MFC2 140
#define VR4300_OP_MFHI 141
#define VR4300_OP_MFLO 142
#define VR4300_OP_MOV_D 143
#define VR4300_OP_MOV_L 144
#define VR4300_OP_MOV_S 145
#define VR4300_OP_MOV_W 146
#define VR4300_OP_MTC0 147
#define VR4300_OP_MTC1 148
#define VR4300_OP_MTC2 149
#define VR4300_OP_MTHI 150
#define VR4300_OP_MTLO 151
#define VR4300_OP_MULT 152
#define VR4300_OP_MULTU 153
#define VR4300_OP_MUL_D 154
#define VR4300_OP_MUL_L 155
#define VR4300_OP_MUL_S 156
#define VR4300_OP_MUL_W 157
#define VR4300_OP_NEG_D 158
#define VR4300_OP_NEG_L 159
#define VR4300_OP_NEG_S 160
#define VR4300_OP_NEG_W 161
#define VR4300_OP_NOR 162
#define VR4300_OP_OR 163
#define VR4300_OP_ORI 164
#define VR4300_OP_ROUND_L_D 165
#define VR4300_OP_ROUND_L_L 166
#define VR4300_OP_ROUND_L_S 167
#define VR4300_OP_ROUND_L_W 168
#define VR4300_OP_ROUND_W_D 169
#define VR4300_OP_ROUND_W_L 170
#define VR4300_OP_ROUND_W_S 171
#define VR4300_OP_ROUND_W_W 172
#define VR4300_OP_SB 173
#define VR4300_OP_SC 174
#define VR4300_OP_SCD 175
#define VR4300_OP_SD 176
#define VR4300_OP_SDC1 177
#define VR4300_OP_SDL 178
#define VR4300_OP_SDR 179
#define VR4300_OP_SH 180
#define VR4300_OP_SLL 181
#define VR4300_OP_SLLV 182
#define VR4300_OP_SLT 183
#define VR4300_OP_SLTI 184
#define VR4300_OP_SLTIU 185
#define VR4300_OP_SLTU 186
#define VR4300_OP_SQRT_D 187
#define VR4300_OP_SQRT_L 188
#define VR4300_OP_SQRT_S 189
#define VR4300_OP_SQRT_W 190
#define VR4300_OP_SRA 191
#define VR4300_OP_SRAV 192
#define VR4300_OP_SRL 193
#define VR4300_OP_SRLV 194
#define VR4300_OP_SUB 195
#define VR4300_OP_SUBU 196
#define VR4300_OP_SUB_D 197
#define VR4300_OP_SUB_L 198
#define VR4300_OP_SUB_S 199
#define VR4300_OP_SUB_W 200
#define VR4300_OP_SW 201
#define VR4300_OP_SWC1 202
#define VR4300_OP_SWL 203
#define VR4300_OP_SWR 204
#define VR4300_OP_SYNC 205
#define VR4300_OP_SYSCALL 206
#define VR4300_OP_TEQ 207
#define VR4300_OP_TEQI 208
#define VR4300_OP_TGE 209
#define VR4300_OP_TGEI 210
#define VR4300_OP_TGEIU 211
#define VR4300_OP_TGEU 212
#define VR4300_OP_TLBP 213
#define VR4300_OP_TLBR 214
#define VR4300_OP_TLBWI 215
#define VR4300_OP_TLBWR 216
#define VR4300_OP_TLT 217
#define VR4300_OP_TLTI 218
#define VR4300_OP_TLTIU 219
#define VR4300_OP_TLTU 220
#define VR4300_OP_TNE 221
#define VR4300_OP_TNEI 222
#define VR4300_OP_TRUNC_L_D 223
#define VR4300_OP_TRUNC_L_L 224
#define VR4300_OP_TRUNC_L_S 225
#define VR4300_OP_TRUNC_L_W 226
#define VR4300_OP_TRUNC_W_D 227
#define VR4300_OP_TRUNC_W_L 228
#define VR4300_OP_TRUNC_W_S 229
#define VR4300_OP_TRUNC_W_W 230
#define VR4300_OP_XOR 231
#define VR4300_OP_XORI 232

bool decode_instruction(vr4300_instruction inst, jit_instr *instr);

#endif

