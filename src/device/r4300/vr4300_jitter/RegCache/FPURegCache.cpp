// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RegCache/FPURegCache.h"

#include "Common/x64Reg.h"

#include "../regs.h"
#include "../vr4300_jitter_instruction_decoder.h"
#include "../vr4300_jitter_internal.h"

using namespace Gen;

alignas(16) static const __m128i double_zero = _mm_set_epi64x(0, 0);

FPURegCache::FPURegCache() : RegCache{}
{
}

void FPURegCache::StoreRegister(preg_t preg, const OpArg& new_loc)
{
  if (preg & 1) abort();
  ASSERT_MSG(DYNA_REC, m_regs[preg].IsBound(), "Unbound register - %d", preg);
  m_emitter->MOVSD(new_loc, m_regs[preg].Location()->GetSimpleReg());
}

void FPURegCache::LoadRegister(preg_t preg, X64Reg new_loc)
{
  if (preg & 1) abort();
  ASSERT_MSG(DYNA_REC, !m_regs[preg].IsDiscarded(), "Discarded register - %d", preg);
  m_emitter->MOVSD(new_loc, m_regs[preg].Location().value());
}

void FPURegCache::StoreRegister32(preg_t preg, const OpArg& new_loc)
{
  ASSERT_MSG(DYNA_REC, m_regs[preg].IsBound(), "Unbound register - %d", preg);
  m_emitter->MOVSS(new_loc, m_regs[preg].Location()->GetSimpleReg());
}

void FPURegCache::FlushUpper(preg_t preg)
{
  ASSERT((preg & 1) == 0);
  VR4300_Jitter *jitter = VR4300_Jitter::GetInstance();
  m_emitter->MOVSD(XMM2, jitter->MConst(double_zero));
  m_emitter->MOVSS(HOTSTATE_CP1REG32_UPPER(preg / 2), XMM2);
}

void FPURegCache::LoadRegister32(preg_t preg, X64Reg new_loc, bool flush_upper)
{
  ASSERT_MSG(DYNA_REC, !m_regs[preg].IsDiscarded(), "Discarded register - %d", preg);
  if (m_regs[preg].GetLocationType() == VR4300CachedReg::LocationType::Default) {
    m_emitter->MOVSS(new_loc, GetDefaultLocation32(preg));
    if (flush_upper) {
      //TODO
      FlushUpper(preg);
    }
  } else {
    abort();
  }
}

const X64Reg* FPURegCache::GetAllocationOrder(size_t* count) const
{
  static const X64Reg allocation_order[] = {XMM6,  XMM7,  XMM8,  XMM9, XMM10, XMM11, XMM12,
                                            XMM13, XMM14, XMM15, XMM3,  XMM4,  XMM5};
  *count = sizeof(allocation_order) / sizeof(X64Reg);
  return allocation_order;
}

OpArg FPURegCache::GetDefaultLocation(preg_t preg) const
{
  return HOTSTATE_CP1REG(preg / 2);
}

OpArg FPURegCache::GetDefaultLocation32(preg_t preg) const
{
  //if (HOT_STATE->fr_is_set) {
  //  return HOTSTATE_CP1REG32FR(preg / 2);
  //} else {
    if (preg & 1) {
        return HOTSTATE_CP1REG32_UPPER(preg / 2);
    } else {
        return HOTSTATE_CP1REG32_LOWER(preg / 2);
    }
  //}
}

BitSet64 FPURegCache::GetRegUtilization() const
{
  return HOT_STATE->op->fregsInUse;
}

BitSet64 FPURegCache::CountRegsIn(preg_t preg, u32 lookahead) const
{
  BitSet64 regs_used;

  for (u32 i = 1; i < lookahead; i++)
  {
    BitSet64 regs_in = HOT_STATE->op[i].fregsIn | HOT_STATE->op[i].fregsIn32;
    regs_used |= regs_in;
    if (regs_in[preg])
      return regs_used;
  }

  return regs_used;
}
