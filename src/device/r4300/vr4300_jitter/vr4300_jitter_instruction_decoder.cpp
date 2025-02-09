/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - vr4300_jitter_instruction_decoder.cpp                   *
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


#include "vr4300_jitter_instruction_decoder.h"


void decode_ABS_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ABS_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "ABS_D";
}
void decode_ABS_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ABS_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "ABS_L";
}
void decode_ABS_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ABS_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "ABS_S";
}
void decode_ABS_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ABS_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "ABS_W";
}
void decode_ADD(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ADD;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "ADD";
}
void decode_ADDI(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ADDI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "ADDI";
}
void decode_ADDIU(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ADDIU;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "ADDIU";
}
void decode_ADDU(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ADDU;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "ADDU";
}
void decode_ADD_D(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_ADD_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "ADD_D";
}
void decode_ADD_L(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_ADD_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "ADD_L";
}
void decode_ADD_S(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_ADD_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "ADD_S";
}
void decode_ADD_W(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_ADD_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "ADD_W";
}
void decode_AND(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_AND;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "AND";
}
void decode_ANDI(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ANDI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "ANDI";
}
void decode_BC0F(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC0F;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "BC0F";
}
void decode_BC0FL(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC0FL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "BC0FL";
}
void decode_BC0T(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC0T;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "BC0T";
}
void decode_BC0TL(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC0TL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "BC0TL";
}
void decode_BC1F(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC1F;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "BC1F";
}
void decode_BC1FL(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC1FL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "BC1FL";
}
void decode_BC1T(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC1T;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "BC1T";
}
void decode_BC1TL(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC1TL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "BC1TL";
}
void decode_BC3F(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC3F;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "BC3F";
}
void decode_BC3FL(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC3FL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "BC3FL";
}
void decode_BC3T(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC3T;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "BC3T";
}
void decode_BC3TL(int f, jit_instr *instr) {
    instr->operation = VR4300_OP_BC3TL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "BC3TL";
}
void decode_BEQ(int f, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BEQ;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BEQ";
}
void decode_BEQL(int f, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BEQL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BEQL";
}
void decode_BGEZ(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BGEZ;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BGEZ";
}
void decode_BGEZAL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BGEZAL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BGEZAL";
}
void decode_BGEZALL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BGEZALL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BGEZALL";
}
void decode_BGEZL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BGEZL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BGEZL";
}
void decode_BGTZ(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BGTZ;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BGTZ";
}
void decode_BGTZL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BGTZL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BGTZL";
}
void decode_BLEZ(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BLEZ;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BLEZ";
}
void decode_BLEZL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BLEZL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BLEZL";
}
void decode_BLTZ(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BLTZ;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BLTZ";
}
void decode_BLTZAL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BLTZAL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BLTZAL";
}
void decode_BLTZALL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BLTZALL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BLTZALL";
}
void decode_BLTZL(int f, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BLTZL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BLTZL";
}
void decode_BNE(int f, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BNE;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BNE";
}
void decode_BNEL(int f, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_BNEL;
    instr->is_branch_or_jump = true;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "BNEL";
}
void decode_BREAK(int k, jit_instr *instr) {
    instr->operation = VR4300_OP_BREAK;
    instr->k = k;
    instr->has_k = true;
    instr->name = "BREAK";
}
void decode_CACHE(int f, int k, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_CACHE;
    instr->f = f;
    instr->has_f = true;
    instr->k = k;
    instr->has_k = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "CACHE";
}
void decode_CEIL_L_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_L_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "CEIL_L_D";
}
void decode_CEIL_L_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_L_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "CEIL_L_L";
}
void decode_CEIL_L_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_L_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "CEIL_L_S";
}
void decode_CEIL_L_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_L_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "CEIL_L_W";
}
void decode_CEIL_W_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_W_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "CEIL_W_D";
}
void decode_CEIL_W_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_W_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "CEIL_W_L";
}
void decode_CEIL_W_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_W_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "CEIL_W_S";
}
void decode_CEIL_W_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CEIL_W_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "CEIL_W_W";
}
void decode_CFC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CFC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "CFC0";
}
void decode_CFC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CFC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "CFC1";
}
void decode_CFC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CFC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "CFC2";
}
void decode_CFC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CFC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "CFC3";
}
void decode_CTC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CTC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "CTC0";
}
void decode_CTC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CTC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "CTC1";
}
void decode_CTC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CTC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "CTC2";
}
void decode_CTC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_CTC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "CTC3";
}
void decode_CVT_D_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_D_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "CVT_D_D";
}
void decode_CVT_D_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_D_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "CVT_D_L";
}
void decode_CVT_D_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_D_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "CVT_D_S";
}
void decode_CVT_D_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_D_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "CVT_D_W";
}
void decode_CVT_L_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_L_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "CVT_L_D";
}
void decode_CVT_L_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_L_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "CVT_L_L";
}
void decode_CVT_L_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_L_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "CVT_L_S";
}
void decode_CVT_L_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_L_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "CVT_L_W";
}
void decode_CVT_S_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_S_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "CVT_S_D";
}
void decode_CVT_S_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_S_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "CVT_S_L";
}
void decode_CVT_S_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_S_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "CVT_S_S";
}
void decode_CVT_S_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_S_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "CVT_S_W";
}
void decode_CVT_W_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_W_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "CVT_W_D";
}
void decode_CVT_W_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_W_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "CVT_W_L";
}
void decode_CVT_W_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_W_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "CVT_W_S";
}
void decode_CVT_W_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_CVT_W_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "CVT_W_W";
}
void decode_C_cond_D(int c, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_C_cond_D;
    instr->c = c;
    instr->has_c = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "C_cond_D";
}
void decode_C_cond_L(int c, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_C_cond_L;
    instr->c = c;
    instr->has_c = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "C_cond_L";
}
void decode_C_cond_S(int c, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_C_cond_S;
    instr->c = c;
    instr->has_c = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "C_cond_S";
}
void decode_C_cond_W(int c, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_C_cond_W;
    instr->c = c;
    instr->has_c = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "C_cond_W";
}
void decode_DADD(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DADD;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DADD";
}
void decode_DADDI(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DADDI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DADDI";
}
void decode_DADDIU(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DADDIU;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DADDIU";
}
void decode_DADDU(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DADDU;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DADDU";
}
void decode_DCFC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCFC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "DCFC0";
}
void decode_DCFC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCFC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "DCFC1";
}
void decode_DCFC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCFC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "DCFC2";
}
void decode_DCFC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCFC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "DCFC3";
}
void decode_DCTC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCTC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "DCTC0";
}
void decode_DCTC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCTC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "DCTC1";
}
void decode_DCTC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCTC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "DCTC2";
}
void decode_DCTC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DCTC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "DCTC3";
}
void decode_DDIV(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DDIV;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DDIV";
}
void decode_DDIVU(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DDIVU;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DDIVU";
}
void decode_DIV(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DIV;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DIV";
}
void decode_DIVU(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DIVU;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DIVU";
}
void decode_DIV_D(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DIV_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "DIV_D";
}
void decode_DIV_L(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DIV_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "DIV_L";
}
void decode_DIV_S(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DIV_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "DIV_S";
}
void decode_DIV_W(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DIV_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "DIV_W";
}
void decode_DMFC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMFC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "DMFC0";
}
void decode_DMFC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMFC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "DMFC1";
}
void decode_DMFC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMFC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "DMFC2";
}
void decode_DMFC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMFC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "DMFC3";
}
void decode_DMTC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMTC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "DMTC0";
}
void decode_DMTC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMTC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "DMTC1";
}
void decode_DMTC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMTC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "DMTC2";
}
void decode_DMTC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DMTC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "DMTC3";
}
void decode_DMULT(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DMULT;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DMULT";
}
void decode_DMULTU(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DMULTU;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DMULTU";
}
void decode_DSLL(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DSLL;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "DSLL";
}
void decode_DSLL32(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DSLL32;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "DSLL32";
}
void decode_DSLLV(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DSLLV;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DSLLV";
}
void decode_DSRA(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DSRA;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "DSRA";
}
void decode_DSRA32(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DSRA32;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "DSRA32";
}
void decode_DSRAV(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DSRAV;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DSRAV";
}
void decode_DSRL(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DSRL;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "DSRL";
}
void decode_DSRL32(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_DSRL32;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "DSRL32";
}
void decode_DSRLV(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DSRLV;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DSRLV";
}
void decode_DSUB(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DSUB;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DSUB";
}
void decode_DSUBU(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_DSUBU;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "DSUBU";
}
void decode_ERET(jit_instr *instr) {
    instr->operation = VR4300_OP_ERET;
    instr->name = "ERET";
}
void decode_FLOOR_L_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_L_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "FLOOR_L_D";
}
void decode_FLOOR_L_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_L_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "FLOOR_L_L";
}
void decode_FLOOR_L_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_L_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "FLOOR_L_S";
}
void decode_FLOOR_L_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_L_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "FLOOR_L_W";
}
void decode_FLOOR_W_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_W_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "FLOOR_W_D";
}
void decode_FLOOR_W_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_W_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "FLOOR_W_L";
}
void decode_FLOOR_W_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_W_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "FLOOR_W_S";
}
void decode_FLOOR_W_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_FLOOR_W_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "FLOOR_W_W";
}
void decode_J(int k, jit_instr *instr) {
    instr->operation = VR4300_OP_J;
    instr->is_branch_or_jump = true;
    instr->k = k;
    instr->has_k = true;
    instr->name = "J";
}
void decode_JAL(int k, jit_instr *instr) {
    instr->operation = VR4300_OP_JAL;
    instr->is_branch_or_jump = true;
    instr->k = k;
    instr->has_k = true;
    instr->name = "JAL";
}
void decode_JALR(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_JALR;
    instr->is_branch_or_jump = true;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "JALR";
}
void decode_JR(int s, jit_instr *instr) {
    instr->operation = VR4300_OP_JR;
    instr->is_branch_or_jump = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "JR";
}
void decode_LB(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LB;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LB";
}
void decode_LBU(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LBU;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LBU";
}
void decode_LD(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LD;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LD";
}
void decode_LDC1(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LDC1;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "LDC1";
}
void decode_LDC2(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LDC2;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "LDC2";
}
void decode_LDL(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LDL;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LDL";
}
void decode_LDR(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LDR;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LDR";
}
void decode_LH(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LH;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LH";
}
void decode_LHU(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LHU;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LHU";
}
void decode_LL(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LL;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LL";
}
void decode_LLD(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LLD;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LLD";
}
void decode_LUI(int k, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_LUI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "LUI";
}
void decode_LW(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LW;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LW";
}
void decode_LWC1(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LWC1;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "LWC1";
}
void decode_LWC2(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LWC2;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "LWC2";
}
void decode_LWL(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LWL;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LWL";
}
void decode_LWR(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LWR;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LWR";
}
void decode_LWU(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_LWU;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "LWU";
}
void decode_MFC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MFC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "MFC0";
}
void decode_MFC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MFC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "MFC1";
}
void decode_MFC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MFC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "MFC2";
}
void decode_MFC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MFC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "MFC3";
}
void decode_MFHI(int d, jit_instr *instr) {
    instr->operation = VR4300_OP_MFHI;
    instr->d = d;
    instr->has_d = true;
    instr->name = "MFHI";
}
void decode_MFLO(int d, jit_instr *instr) {
    instr->operation = VR4300_OP_MFLO;
    instr->d = d;
    instr->has_d = true;
    instr->name = "MFLO";
}
void decode_MOV_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MOV_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "MOV_D";
}
void decode_MOV_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MOV_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "MOV_L";
}
void decode_MOV_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MOV_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "MOV_S";
}
void decode_MOV_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MOV_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "MOV_W";
}
void decode_MTC0(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MTC0;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 0;
    instr->name = "MTC0";
}
void decode_MTC1(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MTC1;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "MTC1";
}
void decode_MTC2(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MTC2;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "MTC2";
}
void decode_MTC3(int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MTC3;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_x = true;
    instr->x = 3;
    instr->name = "MTC3";
}
void decode_MTHI(int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MTHI;
    instr->s = s;
    instr->has_s = true;
    instr->name = "MTHI";
}
void decode_MTLO(int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MTLO;
    instr->s = s;
    instr->has_s = true;
    instr->name = "MTLO";
}
void decode_MULT(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MULT;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "MULT";
}
void decode_MULTU(int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_MULTU;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "MULTU";
}
void decode_MUL_D(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MUL_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "MUL_D";
}
void decode_MUL_L(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MUL_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "MUL_L";
}
void decode_MUL_S(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MUL_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "MUL_S";
}
void decode_MUL_W(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_MUL_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "MUL_W";
}
void decode_NEG_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_NEG_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "NEG_D";
}
void decode_NEG_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_NEG_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "NEG_L";
}
void decode_NEG_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_NEG_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "NEG_S";
}
void decode_NEG_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_NEG_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "NEG_W";
}
void decode_NOR(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_NOR;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "NOR";
}
void decode_OR(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_OR;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "OR";
}
void decode_ORI(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ORI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "ORI";
}
void decode_RESERVED31(jit_instr *instr) {
    instr->operation = VR4300_OP_RESERVED31;
    instr->name = "RESERVED31";
}
void decode_ROUND_L_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_L_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "ROUND_L_D";
}
void decode_ROUND_L_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_L_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "ROUND_L_L";
}
void decode_ROUND_L_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_L_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "ROUND_L_S";
}
void decode_ROUND_L_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_L_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "ROUND_L_W";
}
void decode_ROUND_W_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_W_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "ROUND_W_D";
}
void decode_ROUND_W_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_W_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "ROUND_W_L";
}
void decode_ROUND_W_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_W_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "ROUND_W_S";
}
void decode_ROUND_W_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_ROUND_W_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "ROUND_W_W";
}
void decode_SB(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SB;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SB";
}
void decode_SC(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SC;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SC";
}
void decode_SCD(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SCD;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SCD";
}
void decode_SD(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SD;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SD";
}
void decode_SDC1(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SDC1;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "SDC1";
}
void decode_SDC2(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SDC2;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "SDC2";
}
void decode_SDL(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SDL;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SDL";
}
void decode_SDR(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SDR;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SDR";
}
void decode_SH(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SH;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SH";
}
void decode_SLL(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SLL;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "SLL";
}
void decode_SLLV(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SLLV;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SLLV";
}
void decode_SLT(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SLT;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SLT";
}
void decode_SLTI(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SLTI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SLTI";
}
void decode_SLTIU(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SLTIU;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SLTIU";
}
void decode_SLTU(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SLTU;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SLTU";
}
void decode_SQRT_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SQRT_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "SQRT_D";
}
void decode_SQRT_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SQRT_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "SQRT_L";
}
void decode_SQRT_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SQRT_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "SQRT_S";
}
void decode_SQRT_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SQRT_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "SQRT_W";
}
void decode_SRA(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SRA;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "SRA";
}
void decode_SRAV(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SRAV;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SRAV";
}
void decode_SRL(int k, int d, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SRL;
    instr->k = k;
    instr->has_k = true;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->name = "SRL";
}
void decode_SRLV(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SRLV;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SRLV";
}
void decode_SUB(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SUB;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SUB";
}
void decode_SUBU(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_SUBU;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "SUBU";
}
void decode_SUB_D(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SUB_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "SUB_D";
}
void decode_SUB_L(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SUB_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "SUB_L";
}
void decode_SUB_S(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SUB_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "SUB_S";
}
void decode_SUB_W(int d, int s, int t, jit_instr *instr) {
    instr->operation = VR4300_OP_SUB_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->t = t;
    instr->has_t = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "SUB_W";
}
void decode_SW(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SW;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SW";
}
void decode_SWC1(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SWC1;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 1;
    instr->name = "SWC1";
}
void decode_SWC2(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SWC2;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->has_x = true;
    instr->x = 2;
    instr->name = "SWC2";
}
void decode_SWL(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SWL;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SWL";
}
void decode_SWR(int f, int t, int b, jit_instr *instr) {
    instr->operation = VR4300_OP_SWR;
    instr->f = f;
    instr->has_f = true;
    instr->t = t;
    instr->has_t = true;
    instr->b = b;
    instr->has_b = true;
    instr->name = "SWR";
}
void decode_SYNC(jit_instr *instr) {
    instr->operation = VR4300_OP_SYNC;
    instr->name = "SYNC";
}
void decode_SYSCALL(int k, jit_instr *instr) {
    instr->operation = VR4300_OP_SYSCALL;
    instr->k = k;
    instr->has_k = true;
    instr->name = "SYSCALL";
}
void decode_TEQ(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TEQ;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TEQ";
}
void decode_TEQI(int k, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TEQI;
    instr->k = k;
    instr->has_k = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TEQI";
}
void decode_TGE(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TGE;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TGE";
}
void decode_TGEI(int k, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TGEI;
    instr->k = k;
    instr->has_k = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TGEI";
}
void decode_TGEIU(int k, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TGEIU;
    instr->k = k;
    instr->has_k = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TGEIU";
}
void decode_TGEU(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TGEU;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TGEU";
}
void decode_TLBP(jit_instr *instr) {
    instr->operation = VR4300_OP_TLBP;
    instr->name = "TLBP";
}
void decode_TLBR(jit_instr *instr) {
    instr->operation = VR4300_OP_TLBR;
    instr->name = "TLBR";
}
void decode_TLBWI(jit_instr *instr) {
    instr->operation = VR4300_OP_TLBWI;
    instr->name = "TLBWI";
}
void decode_TLBWR(jit_instr *instr) {
    instr->operation = VR4300_OP_TLBWR;
    instr->name = "TLBWR";
}
void decode_TLT(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TLT;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TLT";
}
void decode_TLTI(int k, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TLTI;
    instr->k = k;
    instr->has_k = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TLTI";
}
void decode_TLTIU(int k, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TLTIU;
    instr->k = k;
    instr->has_k = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TLTIU";
}
void decode_TLTU(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TLTU;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TLTU";
}
void decode_TNE(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TNE;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TNE";
}
void decode_TNEI(int k, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TNEI;
    instr->k = k;
    instr->has_k = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "TNEI";
}
void decode_TRUNC_L_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_L_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "TRUNC_L_D";
}
void decode_TRUNC_L_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_L_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "TRUNC_L_L";
}
void decode_TRUNC_L_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_L_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "TRUNC_L_S";
}
void decode_TRUNC_L_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_L_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "TRUNC_L_W";
}
void decode_TRUNC_W_D(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_W_D;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 17;
    instr->name = "TRUNC_W_D";
}
void decode_TRUNC_W_L(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_W_L;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 21;
    instr->name = "TRUNC_W_L";
}
void decode_TRUNC_W_S(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_W_S;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 16;
    instr->name = "TRUNC_W_S";
}
void decode_TRUNC_W_W(int d, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_TRUNC_W_W;
    instr->d = d;
    instr->has_d = true;
    instr->s = s;
    instr->has_s = true;
    instr->has_a = true;
    instr->a = 20;
    instr->name = "TRUNC_W_W";
}
void decode_XOR(int d, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_XOR;
    instr->d = d;
    instr->has_d = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "XOR";
}
void decode_XORI(int k, int t, int s, jit_instr *instr) {
    instr->operation = VR4300_OP_XORI;
    instr->k = k;
    instr->has_k = true;
    instr->t = t;
    instr->has_t = true;
    instr->s = s;
    instr->has_s = true;
    instr->name = "XORI";
}

bool decode_instruction(vr4300_instruction inst, jit_instr *instr) {
    switch(inst.raw & (OP_MASK_0_10 | OP_MASK_16_20 | OP_MASK_26_31)) {
        case 9:
            decode_JALR(inst.JALR_d, inst.JALR_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_10 | OP_MASK_16_31)) {
        case 16:
            decode_MFHI(inst.MFHI_d, instr);
            return true;
        case 18:
            decode_MFLO(inst.MFLO_d, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_10 | OP_MASK_21_31)) {
        case 1077936128:
            decode_CFC0(inst.CFC0_d, inst.CFC0_t, instr);
            return true;
        case 1145044992:
            decode_CFC1(inst.CFC1_d, inst.CFC1_t, instr);
            return true;
        case 1212153856:
            decode_CFC2(inst.CFC2_d, inst.CFC2_t, instr);
            return true;
        case 1279262720:
            decode_CFC3(inst.CFC3_d, inst.CFC3_t, instr);
            return true;
        case 1086324736:
            decode_CTC0(inst.CTC0_d, inst.CTC0_t, instr);
            return true;
        case 1153433600:
            decode_CTC1(inst.CTC1_d, inst.CTC1_t, instr);
            return true;
        case 1220542464:
            decode_CTC2(inst.CTC2_d, inst.CTC2_t, instr);
            return true;
        case 1287651328:
            decode_CTC3(inst.CTC3_d, inst.CTC3_t, instr);
            return true;
        case 1080033280:
            decode_DCFC0(inst.DCFC0_d, inst.DCFC0_t, instr);
            return true;
        case 1147142144:
            decode_DCFC1(inst.DCFC1_d, inst.DCFC1_t, instr);
            return true;
        case 1214251008:
            decode_DCFC2(inst.DCFC2_d, inst.DCFC2_t, instr);
            return true;
        case 1281359872:
            decode_DCFC3(inst.DCFC3_d, inst.DCFC3_t, instr);
            return true;
        case 1088421888:
            decode_DCTC0(inst.DCTC0_d, inst.DCTC0_t, instr);
            return true;
        case 1155530752:
            decode_DCTC1(inst.DCTC1_d, inst.DCTC1_t, instr);
            return true;
        case 1222639616:
            decode_DCTC2(inst.DCTC2_d, inst.DCTC2_t, instr);
            return true;
        case 1289748480:
            decode_DCTC3(inst.DCTC3_d, inst.DCTC3_t, instr);
            return true;
        case 1075838976:
            decode_DMFC0(inst.DMFC0_d, inst.DMFC0_t, instr);
            return true;
        case 1142947840:
            decode_DMFC1(inst.DMFC1_d, inst.DMFC1_t, instr);
            return true;
        case 1210056704:
            decode_DMFC2(inst.DMFC2_d, inst.DMFC2_t, instr);
            return true;
        case 1277165568:
            decode_DMFC3(inst.DMFC3_d, inst.DMFC3_t, instr);
            return true;
        case 1084227584:
            decode_DMTC0(inst.DMTC0_d, inst.DMTC0_t, instr);
            return true;
        case 1151336448:
            decode_DMTC1(inst.DMTC1_d, inst.DMTC1_t, instr);
            return true;
        case 1218445312:
            decode_DMTC2(inst.DMTC2_d, inst.DMTC2_t, instr);
            return true;
        case 1285554176:
            decode_DMTC3(inst.DMTC3_d, inst.DMTC3_t, instr);
            return true;
        case 1073741824:
            decode_MFC0(inst.MFC0_d, inst.MFC0_t, instr);
            return true;
        case 1140850688:
            decode_MFC1(inst.MFC1_d, inst.MFC1_t, instr);
            return true;
        case 1207959552:
            decode_MFC2(inst.MFC2_d, inst.MFC2_t, instr);
            return true;
        case 1275068416:
            decode_MFC3(inst.MFC3_d, inst.MFC3_t, instr);
            return true;
        case 1082130432:
            decode_MTC0(inst.MTC0_d, inst.MTC0_t, instr);
            return true;
        case 1149239296:
            decode_MTC1(inst.MTC1_d, inst.MTC1_t, instr);
            return true;
        case 1216348160:
            decode_MTC2(inst.MTC2_d, inst.MTC2_t, instr);
            return true;
        case 1283457024:
            decode_MTC3(inst.MTC3_d, inst.MTC3_t, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_10 | OP_MASK_26_31)) {
        case 33:
            decode_ADDU(inst.ADDU_d, inst.ADDU_t, inst.ADDU_s, instr);
            return true;
        case 32:
            decode_ADD(inst.ADD_d, inst.ADD_t, inst.ADD_s, instr);
            return true;
        case 36:
            decode_AND(inst.AND_d, inst.AND_t, inst.AND_s, instr);
            return true;
        case 45:
            decode_DADDU(inst.DADDU_d, inst.DADDU_t, inst.DADDU_s, instr);
            return true;
        case 44:
            decode_DADD(inst.DADD_d, inst.DADD_t, inst.DADD_s, instr);
            return true;
        case 20:
            decode_DSLLV(inst.DSLLV_d, inst.DSLLV_t, inst.DSLLV_s, instr);
            return true;
        case 23:
            decode_DSRAV(inst.DSRAV_d, inst.DSRAV_t, inst.DSRAV_s, instr);
            return true;
        case 22:
            decode_DSRLV(inst.DSRLV_d, inst.DSRLV_t, inst.DSRLV_s, instr);
            return true;
        case 47:
            decode_DSUBU(inst.DSUBU_d, inst.DSUBU_t, inst.DSUBU_s, instr);
            return true;
        case 46:
            decode_DSUB(inst.DSUB_d, inst.DSUB_t, inst.DSUB_s, instr);
            return true;
        case 39:
            decode_NOR(inst.NOR_d, inst.NOR_t, inst.NOR_s, instr);
            return true;
        case 37:
            decode_OR(inst.OR_d, inst.OR_t, inst.OR_s, instr);
            return true;
        case 4:
            decode_SLLV(inst.SLLV_d, inst.SLLV_t, inst.SLLV_s, instr);
            return true;
        case 43:
            decode_SLTU(inst.SLTU_d, inst.SLTU_t, inst.SLTU_s, instr);
            return true;
        case 42:
            decode_SLT(inst.SLT_d, inst.SLT_t, inst.SLT_s, instr);
            return true;
        case 7:
            decode_SRAV(inst.SRAV_d, inst.SRAV_t, inst.SRAV_s, instr);
            return true;
        case 6:
            decode_SRLV(inst.SRLV_d, inst.SRLV_t, inst.SRLV_s, instr);
            return true;
        case 35:
            decode_SUBU(inst.SUBU_d, inst.SUBU_t, inst.SUBU_s, instr);
            return true;
        case 34:
            decode_SUB(inst.SUB_d, inst.SUB_t, inst.SUB_s, instr);
            return true;
        case 38:
            decode_XOR(inst.XOR_d, inst.XOR_t, inst.XOR_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_15 | OP_MASK_26_31)) {
        case 31:
            decode_DDIVU(inst.DDIVU_t, inst.DDIVU_s, instr);
            return true;
        case 30:
            decode_DDIV(inst.DDIV_t, inst.DDIV_s, instr);
            return true;
        case 27:
            decode_DIVU(inst.DIVU_t, inst.DIVU_s, instr);
            return true;
        case 26:
            decode_DIV(inst.DIV_t, inst.DIV_s, instr);
            return true;
        case 29:
            decode_DMULTU(inst.DMULTU_t, inst.DMULTU_s, instr);
            return true;
        case 28:
            decode_DMULT(inst.DMULT_t, inst.DMULT_s, instr);
            return true;
        case 25:
            decode_MULTU(inst.MULTU_t, inst.MULTU_s, instr);
            return true;
        case 24:
            decode_MULT(inst.MULT_t, inst.MULT_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_20 | OP_MASK_26_31)) {
        case 8:
            decode_JR(inst.JR_s, instr);
            return true;
        case 17:
            decode_MTHI(inst.MTHI_s, instr);
            return true;
        case 19:
            decode_MTLO(inst.MTLO_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_31)) {
        case 1107296280:
            decode_ERET(instr);
            return true;
        case 2080630843:
            decode_RESERVED31(instr);
            return true;
        case 15:
            decode_SYNC(instr);
            return true;
        case 1107296264:
            decode_TLBP(instr);
            return true;
        case 1107296257:
            decode_TLBR(instr);
            return true;
        case 1107296258:
            decode_TLBWI(instr);
            return true;
        case 1107296262:
            decode_TLBWR(instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_5 | OP_MASK_16_31)) {
        case 1176502277:
            decode_ABS_D(inst.ABS_D_d, inst.ABS_D_s, instr);
            return true;
        case 1184890885:
            decode_ABS_L(inst.ABS_L_d, inst.ABS_L_s, instr);
            return true;
        case 1174405125:
            decode_ABS_S(inst.ABS_S_d, inst.ABS_S_s, instr);
            return true;
        case 1182793733:
            decode_ABS_W(inst.ABS_W_d, inst.ABS_W_s, instr);
            return true;
        case 1176502282:
            decode_CEIL_L_D(inst.CEIL_L_D_d, inst.CEIL_L_D_s, instr);
            return true;
        case 1184890890:
            decode_CEIL_L_L(inst.CEIL_L_L_d, inst.CEIL_L_L_s, instr);
            return true;
        case 1174405130:
            decode_CEIL_L_S(inst.CEIL_L_S_d, inst.CEIL_L_S_s, instr);
            return true;
        case 1182793738:
            decode_CEIL_L_W(inst.CEIL_L_W_d, inst.CEIL_L_W_s, instr);
            return true;
        case 1176502286:
            decode_CEIL_W_D(inst.CEIL_W_D_d, inst.CEIL_W_D_s, instr);
            return true;
        case 1184890894:
            decode_CEIL_W_L(inst.CEIL_W_L_d, inst.CEIL_W_L_s, instr);
            return true;
        case 1174405134:
            decode_CEIL_W_S(inst.CEIL_W_S_d, inst.CEIL_W_S_s, instr);
            return true;
        case 1182793742:
            decode_CEIL_W_W(inst.CEIL_W_W_d, inst.CEIL_W_W_s, instr);
            return true;
        case 1176502305:
            decode_CVT_D_D(inst.CVT_D_D_d, inst.CVT_D_D_s, instr);
            return true;
        case 1184890913:
            decode_CVT_D_L(inst.CVT_D_L_d, inst.CVT_D_L_s, instr);
            return true;
        case 1174405153:
            decode_CVT_D_S(inst.CVT_D_S_d, inst.CVT_D_S_s, instr);
            return true;
        case 1182793761:
            decode_CVT_D_W(inst.CVT_D_W_d, inst.CVT_D_W_s, instr);
            return true;
        case 1176502309:
            decode_CVT_L_D(inst.CVT_L_D_d, inst.CVT_L_D_s, instr);
            return true;
        case 1184890917:
            decode_CVT_L_L(inst.CVT_L_L_d, inst.CVT_L_L_s, instr);
            return true;
        case 1174405157:
            decode_CVT_L_S(inst.CVT_L_S_d, inst.CVT_L_S_s, instr);
            return true;
        case 1182793765:
            decode_CVT_L_W(inst.CVT_L_W_d, inst.CVT_L_W_s, instr);
            return true;
        case 1176502304:
            decode_CVT_S_D(inst.CVT_S_D_d, inst.CVT_S_D_s, instr);
            return true;
        case 1184890912:
            decode_CVT_S_L(inst.CVT_S_L_d, inst.CVT_S_L_s, instr);
            return true;
        case 1174405152:
            decode_CVT_S_S(inst.CVT_S_S_d, inst.CVT_S_S_s, instr);
            return true;
        case 1182793760:
            decode_CVT_S_W(inst.CVT_S_W_d, inst.CVT_S_W_s, instr);
            return true;
        case 1176502308:
            decode_CVT_W_D(inst.CVT_W_D_d, inst.CVT_W_D_s, instr);
            return true;
        case 1184890916:
            decode_CVT_W_L(inst.CVT_W_L_d, inst.CVT_W_L_s, instr);
            return true;
        case 1174405156:
            decode_CVT_W_S(inst.CVT_W_S_d, inst.CVT_W_S_s, instr);
            return true;
        case 1182793764:
            decode_CVT_W_W(inst.CVT_W_W_d, inst.CVT_W_W_s, instr);
            return true;
        case 1176502283:
            decode_FLOOR_L_D(inst.FLOOR_L_D_d, inst.FLOOR_L_D_s, instr);
            return true;
        case 1184890891:
            decode_FLOOR_L_L(inst.FLOOR_L_L_d, inst.FLOOR_L_L_s, instr);
            return true;
        case 1174405131:
            decode_FLOOR_L_S(inst.FLOOR_L_S_d, inst.FLOOR_L_S_s, instr);
            return true;
        case 1182793739:
            decode_FLOOR_L_W(inst.FLOOR_L_W_d, inst.FLOOR_L_W_s, instr);
            return true;
        case 1176502287:
            decode_FLOOR_W_D(inst.FLOOR_W_D_d, inst.FLOOR_W_D_s, instr);
            return true;
        case 1184890895:
            decode_FLOOR_W_L(inst.FLOOR_W_L_d, inst.FLOOR_W_L_s, instr);
            return true;
        case 1174405135:
            decode_FLOOR_W_S(inst.FLOOR_W_S_d, inst.FLOOR_W_S_s, instr);
            return true;
        case 1182793743:
            decode_FLOOR_W_W(inst.FLOOR_W_W_d, inst.FLOOR_W_W_s, instr);
            return true;
        case 1176502278:
            decode_MOV_D(inst.MOV_D_d, inst.MOV_D_s, instr);
            return true;
        case 1184890886:
            decode_MOV_L(inst.MOV_L_d, inst.MOV_L_s, instr);
            return true;
        case 1174405126:
            decode_MOV_S(inst.MOV_S_d, inst.MOV_S_s, instr);
            return true;
        case 1182793734:
            decode_MOV_W(inst.MOV_W_d, inst.MOV_W_s, instr);
            return true;
        case 1176502279:
            decode_NEG_D(inst.NEG_D_d, inst.NEG_D_s, instr);
            return true;
        case 1184890887:
            decode_NEG_L(inst.NEG_L_d, inst.NEG_L_s, instr);
            return true;
        case 1174405127:
            decode_NEG_S(inst.NEG_S_d, inst.NEG_S_s, instr);
            return true;
        case 1182793735:
            decode_NEG_W(inst.NEG_W_d, inst.NEG_W_s, instr);
            return true;
        case 1176502280:
            decode_ROUND_L_D(inst.ROUND_L_D_d, inst.ROUND_L_D_s, instr);
            return true;
        case 1184890888:
            decode_ROUND_L_L(inst.ROUND_L_L_d, inst.ROUND_L_L_s, instr);
            return true;
        case 1174405128:
            decode_ROUND_L_S(inst.ROUND_L_S_d, inst.ROUND_L_S_s, instr);
            return true;
        case 1182793736:
            decode_ROUND_L_W(inst.ROUND_L_W_d, inst.ROUND_L_W_s, instr);
            return true;
        case 1176502284:
            decode_ROUND_W_D(inst.ROUND_W_D_d, inst.ROUND_W_D_s, instr);
            return true;
        case 1184890892:
            decode_ROUND_W_L(inst.ROUND_W_L_d, inst.ROUND_W_L_s, instr);
            return true;
        case 1174405132:
            decode_ROUND_W_S(inst.ROUND_W_S_d, inst.ROUND_W_S_s, instr);
            return true;
        case 1182793740:
            decode_ROUND_W_W(inst.ROUND_W_W_d, inst.ROUND_W_W_s, instr);
            return true;
        case 1176502276:
            decode_SQRT_D(inst.SQRT_D_d, inst.SQRT_D_s, instr);
            return true;
        case 1184890884:
            decode_SQRT_L(inst.SQRT_L_d, inst.SQRT_L_s, instr);
            return true;
        case 1174405124:
            decode_SQRT_S(inst.SQRT_S_d, inst.SQRT_S_s, instr);
            return true;
        case 1182793732:
            decode_SQRT_W(inst.SQRT_W_d, inst.SQRT_W_s, instr);
            return true;
        case 1176502281:
            decode_TRUNC_L_D(inst.TRUNC_L_D_d, inst.TRUNC_L_D_s, instr);
            return true;
        case 1184890889:
            decode_TRUNC_L_L(inst.TRUNC_L_L_d, inst.TRUNC_L_L_s, instr);
            return true;
        case 1174405129:
            decode_TRUNC_L_S(inst.TRUNC_L_S_d, inst.TRUNC_L_S_s, instr);
            return true;
        case 1182793737:
            decode_TRUNC_L_W(inst.TRUNC_L_W_d, inst.TRUNC_L_W_s, instr);
            return true;
        case 1176502285:
            decode_TRUNC_W_D(inst.TRUNC_W_D_d, inst.TRUNC_W_D_s, instr);
            return true;
        case 1184890893:
            decode_TRUNC_W_L(inst.TRUNC_W_L_d, inst.TRUNC_W_L_s, instr);
            return true;
        case 1174405133:
            decode_TRUNC_W_S(inst.TRUNC_W_S_d, inst.TRUNC_W_S_s, instr);
            return true;
        case 1182793741:
            decode_TRUNC_W_W(inst.TRUNC_W_W_d, inst.TRUNC_W_W_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_5 | OP_MASK_21_31)) {
        case 1176502272:
            decode_ADD_D(inst.ADD_D_d, inst.ADD_D_s, inst.ADD_D_t, instr);
            return true;
        case 1184890880:
            decode_ADD_L(inst.ADD_L_d, inst.ADD_L_s, inst.ADD_L_t, instr);
            return true;
        case 1174405120:
            decode_ADD_S(inst.ADD_S_d, inst.ADD_S_s, inst.ADD_S_t, instr);
            return true;
        case 1182793728:
            decode_ADD_W(inst.ADD_W_d, inst.ADD_W_s, inst.ADD_W_t, instr);
            return true;
        case 1176502275:
            decode_DIV_D(inst.DIV_D_d, inst.DIV_D_s, inst.DIV_D_t, instr);
            return true;
        case 1184890883:
            decode_DIV_L(inst.DIV_L_d, inst.DIV_L_s, inst.DIV_L_t, instr);
            return true;
        case 1174405123:
            decode_DIV_S(inst.DIV_S_d, inst.DIV_S_s, inst.DIV_S_t, instr);
            return true;
        case 1182793731:
            decode_DIV_W(inst.DIV_W_d, inst.DIV_W_s, inst.DIV_W_t, instr);
            return true;
        case 60:
            decode_DSLL32(inst.DSLL32_k, inst.DSLL32_d, inst.DSLL32_t, instr);
            return true;
        case 56:
            decode_DSLL(inst.DSLL_k, inst.DSLL_d, inst.DSLL_t, instr);
            return true;
        case 63:
            decode_DSRA32(inst.DSRA32_k, inst.DSRA32_d, inst.DSRA32_t, instr);
            return true;
        case 59:
            decode_DSRA(inst.DSRA_k, inst.DSRA_d, inst.DSRA_t, instr);
            return true;
        case 62:
            decode_DSRL32(inst.DSRL32_k, inst.DSRL32_d, inst.DSRL32_t, instr);
            return true;
        case 58:
            decode_DSRL(inst.DSRL_k, inst.DSRL_d, inst.DSRL_t, instr);
            return true;
        case 1176502274:
            decode_MUL_D(inst.MUL_D_d, inst.MUL_D_s, inst.MUL_D_t, instr);
            return true;
        case 1184890882:
            decode_MUL_L(inst.MUL_L_d, inst.MUL_L_s, inst.MUL_L_t, instr);
            return true;
        case 1174405122:
            decode_MUL_S(inst.MUL_S_d, inst.MUL_S_s, inst.MUL_S_t, instr);
            return true;
        case 1182793730:
            decode_MUL_W(inst.MUL_W_d, inst.MUL_W_s, inst.MUL_W_t, instr);
            return true;
        case 0:
            decode_SLL(inst.SLL_k, inst.SLL_d, inst.SLL_t, instr);
            return true;
        case 3:
            decode_SRA(inst.SRA_k, inst.SRA_d, inst.SRA_t, instr);
            return true;
        case 2:
            decode_SRL(inst.SRL_k, inst.SRL_d, inst.SRL_t, instr);
            return true;
        case 1176502273:
            decode_SUB_D(inst.SUB_D_d, inst.SUB_D_s, inst.SUB_D_t, instr);
            return true;
        case 1184890881:
            decode_SUB_L(inst.SUB_L_d, inst.SUB_L_s, inst.SUB_L_t, instr);
            return true;
        case 1174405121:
            decode_SUB_S(inst.SUB_S_d, inst.SUB_S_s, inst.SUB_S_t, instr);
            return true;
        case 1182793729:
            decode_SUB_W(inst.SUB_W_d, inst.SUB_W_s, inst.SUB_W_t, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_0_5 | OP_MASK_26_31)) {
        case 13:
            decode_BREAK(inst.BREAK_k, instr);
            return true;
        case 12:
            decode_SYSCALL(inst.SYSCALL_k, instr);
            return true;
        case 52:
            decode_TEQ(inst.TEQ_k, inst.TEQ_t, inst.TEQ_s, instr);
            return true;
        case 49:
            decode_TGEU(inst.TGEU_k, inst.TGEU_t, inst.TGEU_s, instr);
            return true;
        case 48:
            decode_TGE(inst.TGE_k, inst.TGE_t, inst.TGE_s, instr);
            return true;
        case 51:
            decode_TLTU(inst.TLTU_k, inst.TLTU_t, inst.TLTU_s, instr);
            return true;
        case 50:
            decode_TLT(inst.TLT_k, inst.TLT_t, inst.TLT_s, instr);
            return true;
        case 54:
            decode_TNE(inst.TNE_k, inst.TNE_t, inst.TNE_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_16_20 | OP_MASK_26_31)) {
        case 68354048:
            decode_BGEZALL(inst.BGEZALL_f, inst.BGEZALL_s, instr);
            return true;
        case 68222976:
            decode_BGEZAL(inst.BGEZAL_f, inst.BGEZAL_s, instr);
            return true;
        case 67305472:
            decode_BGEZL(inst.BGEZL_f, inst.BGEZL_s, instr);
            return true;
        case 67174400:
            decode_BGEZ(inst.BGEZ_f, inst.BGEZ_s, instr);
            return true;
        case 1543503872:
            decode_BGTZL(inst.BGTZL_f, inst.BGTZL_s, instr);
            return true;
        case 469762048:
            decode_BGTZ(inst.BGTZ_f, inst.BGTZ_s, instr);
            return true;
        case 1476395008:
            decode_BLEZL(inst.BLEZL_f, inst.BLEZL_s, instr);
            return true;
        case 402653184:
            decode_BLEZ(inst.BLEZ_f, inst.BLEZ_s, instr);
            return true;
        case 68288512:
            decode_BLTZALL(inst.BLTZALL_f, inst.BLTZALL_s, instr);
            return true;
        case 68157440:
            decode_BLTZAL(inst.BLTZAL_f, inst.BLTZAL_s, instr);
            return true;
        case 67239936:
            decode_BLTZL(inst.BLTZL_f, inst.BLTZL_s, instr);
            return true;
        case 67108864:
            decode_BLTZ(inst.BLTZ_f, inst.BLTZ_s, instr);
            return true;
        case 67895296:
            decode_TEQI(inst.TEQI_k, inst.TEQI_s, instr);
            return true;
        case 67698688:
            decode_TGEIU(inst.TGEIU_k, inst.TGEIU_s, instr);
            return true;
        case 67633152:
            decode_TGEI(inst.TGEI_k, inst.TGEI_s, instr);
            return true;
        case 67829760:
            decode_TLTIU(inst.TLTIU_k, inst.TLTIU_s, instr);
            return true;
        case 67764224:
            decode_TLTI(inst.TLTI_k, inst.TLTI_s, instr);
            return true;
        case 68026368:
            decode_TNEI(inst.TNEI_k, inst.TNEI_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_16_31)) {
        case 1090650112:
            decode_BC0FL(inst.BC0FL_f, instr);
            return true;
        case 1090519040:
            decode_BC0F(inst.BC0F_f, instr);
            return true;
        case 1090715648:
            decode_BC0TL(inst.BC0TL_f, instr);
            return true;
        case 1090584576:
            decode_BC0T(inst.BC0T_f, instr);
            return true;
        case 1157758976:
            decode_BC1FL(inst.BC1FL_f, instr);
            return true;
        case 1157627904:
            decode_BC1F(inst.BC1F_f, instr);
            return true;
        case 1157824512:
            decode_BC1TL(inst.BC1TL_f, instr);
            return true;
        case 1157693440:
            decode_BC1T(inst.BC1T_f, instr);
            return true;
        case 1291976704:
            decode_BC3FL(inst.BC3FL_f, instr);
            return true;
        case 1291845632:
            decode_BC3F(inst.BC3F_f, instr);
            return true;
        case 1292042240:
            decode_BC3TL(inst.BC3TL_f, instr);
            return true;
        case 1291911168:
            decode_BC3T(inst.BC3T_f, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_21_31)) {
        case 1006632960:
            decode_LUI(inst.LUI_k, inst.LUI_t, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_26_31)) {
        case 603979776:
            decode_ADDIU(inst.ADDIU_k, inst.ADDIU_t, inst.ADDIU_s, instr);
            return true;
        case 536870912:
            decode_ADDI(inst.ADDI_k, inst.ADDI_t, inst.ADDI_s, instr);
            return true;
        case 805306368:
            decode_ANDI(inst.ANDI_k, inst.ANDI_t, inst.ANDI_s, instr);
            return true;
        case 1342177280:
            decode_BEQL(inst.BEQL_f, inst.BEQL_t, inst.BEQL_s, instr);
            return true;
        case 268435456:
            decode_BEQ(inst.BEQ_f, inst.BEQ_t, inst.BEQ_s, instr);
            return true;
        case 1409286144:
            decode_BNEL(inst.BNEL_f, inst.BNEL_t, inst.BNEL_s, instr);
            return true;
        case 335544320:
            decode_BNE(inst.BNE_f, inst.BNE_t, inst.BNE_s, instr);
            return true;
        case 3154116608:
            decode_CACHE(inst.CACHE_f, inst.CACHE_k, inst.CACHE_b, instr);
            return true;
        case 1677721600:
            decode_DADDIU(inst.DADDIU_k, inst.DADDIU_t, inst.DADDIU_s, instr);
            return true;
        case 1610612736:
            decode_DADDI(inst.DADDI_k, inst.DADDI_t, inst.DADDI_s, instr);
            return true;
        case 201326592:
            decode_JAL(inst.JAL_k, instr);
            return true;
        case 134217728:
            decode_J(inst.J_k, instr);
            return true;
        case 2415919104:
            decode_LBU(inst.LBU_f, inst.LBU_t, inst.LBU_b, instr);
            return true;
        case 2147483648:
            decode_LB(inst.LB_f, inst.LB_t, inst.LB_b, instr);
            return true;
        case 3556769792:
            decode_LDC1(inst.LDC1_f, inst.LDC1_t, inst.LDC1_b, instr);
            return true;
        case 3623878656:
            decode_LDC2(inst.LDC2_f, inst.LDC2_t, inst.LDC2_b, instr);
            return true;
        case 1744830464:
            decode_LDL(inst.LDL_f, inst.LDL_t, inst.LDL_b, instr);
            return true;
        case 1811939328:
            decode_LDR(inst.LDR_f, inst.LDR_t, inst.LDR_b, instr);
            return true;
        case 3690987520:
            decode_LD(inst.LD_f, inst.LD_t, inst.LD_b, instr);
            return true;
        case 2483027968:
            decode_LHU(inst.LHU_f, inst.LHU_t, inst.LHU_b, instr);
            return true;
        case 2214592512:
            decode_LH(inst.LH_f, inst.LH_t, inst.LH_b, instr);
            return true;
        case 3489660928:
            decode_LLD(inst.LLD_f, inst.LLD_t, inst.LLD_b, instr);
            return true;
        case 3221225472:
            decode_LL(inst.LL_f, inst.LL_t, inst.LL_b, instr);
            return true;
        case 3288334336:
            decode_LWC1(inst.LWC1_f, inst.LWC1_t, inst.LWC1_b, instr);
            return true;
        case 3355443200:
            decode_LWC2(inst.LWC2_f, inst.LWC2_t, inst.LWC2_b, instr);
            return true;
        case 2281701376:
            decode_LWL(inst.LWL_f, inst.LWL_t, inst.LWL_b, instr);
            return true;
        case 2550136832:
            decode_LWR(inst.LWR_f, inst.LWR_t, inst.LWR_b, instr);
            return true;
        case 2617245696:
            decode_LWU(inst.LWU_f, inst.LWU_t, inst.LWU_b, instr);
            return true;
        case 2348810240:
            decode_LW(inst.LW_f, inst.LW_t, inst.LW_b, instr);
            return true;
        case 872415232:
            decode_ORI(inst.ORI_k, inst.ORI_t, inst.ORI_s, instr);
            return true;
        case 2684354560:
            decode_SB(inst.SB_f, inst.SB_t, inst.SB_b, instr);
            return true;
        case 4026531840:
            decode_SCD(inst.SCD_f, inst.SCD_t, inst.SCD_b, instr);
            return true;
        case 3758096384:
            decode_SC(inst.SC_f, inst.SC_t, inst.SC_b, instr);
            return true;
        case 4093640704:
            decode_SDC1(inst.SDC1_f, inst.SDC1_t, inst.SDC1_b, instr);
            return true;
        case 4160749568:
            decode_SDC2(inst.SDC2_f, inst.SDC2_t, inst.SDC2_b, instr);
            return true;
        case 2952790016:
            decode_SDL(inst.SDL_f, inst.SDL_t, inst.SDL_b, instr);
            return true;
        case 3019898880:
            decode_SDR(inst.SDR_f, inst.SDR_t, inst.SDR_b, instr);
            return true;
        case 4227858432:
            decode_SD(inst.SD_f, inst.SD_t, inst.SD_b, instr);
            return true;
        case 2751463424:
            decode_SH(inst.SH_f, inst.SH_t, inst.SH_b, instr);
            return true;
        case 738197504:
            decode_SLTIU(inst.SLTIU_k, inst.SLTIU_t, inst.SLTIU_s, instr);
            return true;
        case 671088640:
            decode_SLTI(inst.SLTI_k, inst.SLTI_t, inst.SLTI_s, instr);
            return true;
        case 3825205248:
            decode_SWC1(inst.SWC1_f, inst.SWC1_t, inst.SWC1_b, instr);
            return true;
        case 3892314112:
            decode_SWC2(inst.SWC2_f, inst.SWC2_t, inst.SWC2_b, instr);
            return true;
        case 2818572288:
            decode_SWL(inst.SWL_f, inst.SWL_t, inst.SWL_b, instr);
            return true;
        case 3087007744:
            decode_SWR(inst.SWR_f, inst.SWR_t, inst.SWR_b, instr);
            return true;
        case 2885681152:
            decode_SW(inst.SW_f, inst.SW_t, inst.SW_b, instr);
            return true;
        case 939524096:
            decode_XORI(inst.XORI_k, inst.XORI_t, inst.XORI_s, instr);
            return true;
    };
    switch(inst.raw & (OP_MASK_4_10 | OP_MASK_21_31)) {
        case 1176502320:
            decode_C_cond_D(inst.C_cond_D_c, inst.C_cond_D_s, inst.C_cond_D_t, instr);
            return true;
        case 1184890928:
            decode_C_cond_L(inst.C_cond_L_c, inst.C_cond_L_s, inst.C_cond_L_t, instr);
            return true;
        case 1174405168:
            decode_C_cond_S(inst.C_cond_S_c, inst.C_cond_S_s, inst.C_cond_S_t, instr);
            return true;
        case 1182793776:
            decode_C_cond_W(inst.C_cond_W_c, inst.C_cond_W_s, inst.C_cond_W_t, instr);
            return true;
    };
    return false;
}
