#include "vr4300_jitter.h"
#include "Common/x64Emitter.h"
#include "main/main.h"

#pragma once

#define HOTSTATE_OFF(i) (static_cast<int>(offsetof(struct recompiler_hot_state, i)) - 0x80)
#define HOTSTATE2_OFF(i) (static_cast<int>(offsetof(struct recompiler_hot_state, i)) - 0x180)
#define HOTSTATE3_OFF(i) (static_cast<int>(offsetof(struct recompiler_hot_state, i)) - 0x280)

constexpr Gen::X64Reg RHOTSTATE = Gen::RBP;
constexpr Gen::X64Reg RHOTSTATE2 = Gen::R11;
constexpr Gen::X64Reg RHOTSTATE3 = Gen::R10;
constexpr Gen::X64Reg RSCRATCH = Gen::RAX;
constexpr Gen::X64Reg RSCRATCH2 = Gen::RDX;
constexpr Gen::X64Reg RSCRATCH_EXTRA = Gen::RCX;
constexpr Gen::X64Reg RSCRATCH_EXTRA2 = Gen::R12;
constexpr Gen::X64Reg RSCRATCH_EXTRA3 = Gen::R14;
constexpr Gen::X64Reg RSCRATCH_PC = Gen::R13;

#if USE_REG_FOR_FLAGS
constexpr Gen::X64Reg RSTOREDPC = Gen::R15;
#endif

constexpr Gen::X64Reg RDRAM = Gen::RBX;

#define HOTSTATE_OFF_ARRAY(elem, i)                                                                \
  (HOTSTATE_OFF(elem[0]) + static_cast<int>(sizeof(recompiler_hot_state::elem[0]) * (i)))

#define HOTSTATE_OFF_REG(i) HOTSTATE_OFF_ARRAY(regs, i)
#define HOTSTATE_OFF_REG_TMP(i) HOTSTATE_OFF_ARRAY(gprs_tmp, i)

#define HOTSTATE2_OFF_ARRAY(elem, i)                                                                \
  (HOTSTATE2_OFF(elem[0]) + static_cast<int>(sizeof(recompiler_hot_state::elem[0]) * (i)))

#define HOTSTATE2_OFF_REG(i) HOTSTATE2_OFF_ARRAY(regs, i)
#define HOTSTATE2_OFF_REG_TMP(i) HOTSTATE2_OFF_ARRAY(gprs_tmp, i)

#define HOTSTATE3_OFF_ARRAY(elem, i)                                                                \
  (HOTSTATE3_OFF(elem[0]) + static_cast<int>(sizeof(recompiler_hot_state::elem[0]) * (i)))

#define HOTSTATE3_OFF_REG(i) HOTSTATE3_OFF_ARRAY(regs, i)
#define HOTSTATE3_OFF_REG_TMP(i) HOTSTATE3_OFF_ARRAY(gprs_tmp, i)

#define HOTSTATE_OFF_CP0REG(i) HOTSTATE_OFF_ARRAY(cp0_regs, i)
#define HOTSTATE_OFF_CP1REG(i) (HOTSTATE_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float64))
#define HOTSTATE_OFF_CP1REG32_LOWER(i) (HOTSTATE_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE_OFF_CP1REG32_UPPER(i) (HOTSTATE_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * (DOUBLE_HALF_XOR ^ 1))
#define HOTSTATE_OFF_CP1REG32FR(i) (HOTSTATE_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE_OFF_CP1REG32NOFR(i) (HOTSTATE_OFF_ARRAY(cp1_regs, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * (((i) & 1) ^ DOUBLE_HALF_XOR))
#define HOTSTATE_OFF_CP1REG32FR_TMP(i) (HOTSTATE_OFF_ARRAY(fprs_tmp, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE_OFF_CP1REG32NOFR_TMP(i) (HOTSTATE_OFF_ARRAY(fprs_tmp, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * (((i) & 1) ^ DOUBLE_HALF_XOR))
#define HOTSTATE_OFF_CP1REG32FR_OTHER(i) (HOTSTATE_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * (DOUBLE_HALF_XOR ^ 1))
#define HOTSTATE_OFF_CP1REG32NOFR_OTHER(i) (HOTSTATE_OFF_ARRAY(cp1_regs, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * ((((i) & 1) ^ DOUBLE_HALF_XOR) ^ 1))

#define HOTSTATE2_OFF_CP0REG(i) HOTSTATE2_OFF_ARRAY(cp0_regs, i)
#define HOTSTATE2_OFF_CP1REG(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float64))
#define HOTSTATE2_OFF_CP1REG32_LOWER(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE2_OFF_CP1REG32_UPPER(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * (DOUBLE_HALF_XOR ^ 1))
#define HOTSTATE2_OFF_CP1REG32FR(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE2_OFF_CP1REG32NOFR(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * (((i) & 1) ^ DOUBLE_HALF_XOR))
#define HOTSTATE2_OFF_CP1REG32FR_OTHER(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * (DOUBLE_HALF_XOR ^ 1))
#define HOTSTATE2_OFF_CP1REG32FR_TMP(i) (HOTSTATE2_OFF_ARRAY(fprs_tmp, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE2_OFF_CP1REG32NOFR_TMP(i) (HOTSTATE2_OFF_ARRAY(fprs_tmp, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * (((i) & 1) ^ DOUBLE_HALF_XOR))
#define HOTSTATE2_OFF_CP1REG32NOFR_OTHER(i) (HOTSTATE2_OFF_ARRAY(cp1_regs, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * ((((i) & 1) ^ DOUBLE_HALF_XOR) ^ 1))

#define HOTSTATE3_OFF_CP0REG(i) HOTSTATE3_OFF_ARRAY(cp0_regs, i)
#define HOTSTATE3_OFF_CP1REG(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float64))
#define HOTSTATE3_OFF_CP1REG32_LOWER(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE3_OFF_CP1REG32_UPPER(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * (DOUBLE_HALF_XOR ^ 1))
#define HOTSTATE3_OFF_CP1REG32FR(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE3_OFF_CP1REG32NOFR(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * (((i) & 1) ^ DOUBLE_HALF_XOR))
#define HOTSTATE3_OFF_CP1REG32FR_TMP(i) (HOTSTATE3_OFF_ARRAY(fprs_tmp, i) + offsetof(cp1_reg, float32) + sizeof(float) * DOUBLE_HALF_XOR)
#define HOTSTATE3_OFF_CP1REG32NOFR_TMP(i) (HOTSTATE3_OFF_ARRAY(fprs_tmp, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * (((i) & 1) ^ DOUBLE_HALF_XOR))
#define HOTSTATE3_OFF_CP1REG32FR_OTHER(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, i) + offsetof(cp1_reg, float32) + sizeof(float) * (DOUBLE_HALF_XOR ^ 1))
#define HOTSTATE3_OFF_CP1REG32NOFR_OTHER(i) (HOTSTATE3_OFF_ARRAY(cp1_regs, (i) & ~1) + offsetof(cp1_reg, float32) + sizeof(float) * ((((i) & 1) ^ DOUBLE_HALF_XOR) ^ 1))

#define RHOTSTATE_DISP(o1, o2, o3) ((abs((int)(o1)) >= abs((int)(o2))) && abs((int)(o3)) >= abs((int)(o2))) ? MDisp(RHOTSTATE2, o2) : (((abs((int)(o2)) >= abs((int)(o1))) && abs((int)(o3)) >= abs((int)(o1))) ? MDisp(RHOTSTATE, o1) : MDisp(RHOTSTATE3, o3))

#define HOTSTATE_REG(i) RHOTSTATE_DISP(HOTSTATE_OFF_REG(i), HOTSTATE2_OFF_REG(i), HOTSTATE3_OFF_REG(i))
#define HOTSTATE_REG_TMP(i) RHOTSTATE_DISP(HOTSTATE_OFF_REG_TMP(i), HOTSTATE2_OFF_REG_TMP(i), HOTSTATE3_OFF_REG_TMP(i))

#define HOTSTATE_VAR(i) RHOTSTATE_DISP(HOTSTATE_OFF(i), HOTSTATE2_OFF(i), HOTSTATE3_OFF(i))

#define HOTSTATE_CP0REG(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP0REG(i), HOTSTATE2_OFF_CP0REG(i), HOTSTATE3_OFF_CP0REG(i))

#define HOTSTATE_CP1REG(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG(i), HOTSTATE2_OFF_CP1REG(i), HOTSTATE3_OFF_CP1REG(i))

#define HOTSTATE_CP1REGSIMPLE(i) RHOTSTATE_DISP(HOTSTATE_OFF(cp1_regs_simple) + (i) * sizeof(float*), HOTSTATE2_OFF(cp1_regs_simple) + (i) * sizeof(float*), HOTSTATE3_OFF(cp1_regs_simple) + (i) * sizeof(float*))

#define HOTSTATE_ARRAY(elem, i) RHOTSTATE_DISP(HOTSTATE_OFF_ARRAY(elem, i), HOTSTATE2_OFF_ARRAY(elem, i), HOTSTATE3_OFF_ARRAY(elem, i))

#define HOTSTATE_CP1REG32FR(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32FR(i), HOTSTATE2_OFF_CP1REG32FR(i), HOTSTATE3_OFF_CP1REG32FR(i))
#define HOTSTATE_CP1REG32_LOWER(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32_LOWER(i), HOTSTATE2_OFF_CP1REG32_LOWER(i), HOTSTATE3_OFF_CP1REG32_LOWER(i))
#define HOTSTATE_CP1REG32_UPPER(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32_UPPER(i), HOTSTATE2_OFF_CP1REG32_UPPER(i), HOTSTATE3_OFF_CP1REG32_UPPER(i))
#define HOTSTATE_CP1REG32NOFR(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32NOFR(i), HOTSTATE2_OFF_CP1REG32NOFR(i), HOTSTATE3_OFF_CP1REG32NOFR(i))
#define HOTSTATE_CP1REG32FR_TMP(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32FR_TMP(i), HOTSTATE2_OFF_CP1REG32FR_TMP(i), HOTSTATE3_OFF_CP1REG32FR_TMP(i))
#define HOTSTATE_CP1REG32NOFR_TMP(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32NOFR_TMP(i), HOTSTATE2_OFF_CP1REG32NOFR_TMP(i), HOTSTATE3_OFF_CP1REG32NOFR_TMP(i))

#define HOTSTATE_CP1REG32FR_OTHER(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32FR_OTHER(i), HOTSTATE2_OFF_CP1REG32FR_OTHER(i), HOTSTATE3_OFF_CP1REG32FR_OTHER(i))
#define HOTSTATE_CP1REG32NOFR_OTHER(i) RHOTSTATE_DISP(HOTSTATE_OFF_CP1REG32NOFR_OTHER(i), HOTSTATE2_OFF_CP1REG32NOFR_OTHER(i), HOTSTATE3_OFF_CP1REG32NOFR_OTHER(i))

