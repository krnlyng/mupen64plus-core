// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RegCache/GPRRegCache.h"

#include "Common/x64Reg.h"

#include "../regs.h"
#include "../vr4300_jitter_instruction_decoder.h"
#include "../vr4300_jitter.h"
#include "../vr4300_jitter_internal.h"

using namespace Gen;

GPRRegCache::GPRRegCache() : RegCache{}
{
}

void GPRRegCache::StoreRegister(preg_t preg, const OpArg& new_loc)
{
  ASSERT_MSG(DYNA_REC, !m_regs[preg].IsDiscarded(), "Discarded register - %d", preg);

  if (preg == 0) {
      return;
  }

  if (IsImm(preg)) {
    if (vr4300_jitter_value_fits_in_32_bit_imm_positive(m_regs[preg].Location().value().Imm64())) {
        m_emitter->MOV(64, new_loc, ::Gen::Imm32(m_regs[preg].Location().value().Imm64()));
    } else {
        m_emitter->MOV(64, ::Gen::R(RSCRATCH2), m_regs[preg].Location().value());
        m_emitter->MOV(64, new_loc, ::Gen::R(RSCRATCH2));
    }
  } else {
    m_emitter->MOV(64, new_loc, m_regs[preg].Location().value());
  }
}

void GPRRegCache::LoadRegister(preg_t preg, X64Reg new_loc)
{
  ASSERT_MSG(DYNA_REC, !m_regs[preg].IsDiscarded(), "Discarded register - %d", preg);
  if (preg >= 32) abort();
  if (preg == 0) {
      return;
  }
  m_emitter->MOV(64, ::Gen::R(new_loc), m_regs[preg].Location().value());
}

OpArg GPRRegCache::GetDefaultLocation(preg_t preg) const
{
  if (preg >= 32) abort();
  return HOTSTATE_REG(preg);
}

OpArg GPRRegCache::GetDefaultLocation32(preg_t preg) const
{
  return HOTSTATE_REG(preg);
}

const X64Reg* GPRRegCache::GetAllocationOrder(size_t* count) const
{
  static const X64Reg allocation_order[] = {
// R12, when used as base register, for example in a LEA, can generate bad code! Need to look into
// this.
#ifdef _WIN32
      RSI, RDI, R13, R14, R15,
      R8,
      R9, R12, RCX
#else
      R12, R13, R14, R15,
      RSI, RDI,
      R8,  R9, RCX
#endif
  };
  *count = sizeof(allocation_order) / sizeof(X64Reg);
  return allocation_order;
}

void GPRRegCache::SetImmediate64(preg_t preg, u64 imm_value, bool dirty)
{
  // "dirty" can be false to avoid redundantly flushing an immediate when
  // processing speculative constants.
  DiscardRegContentsIfCached(preg);
  m_regs[preg].SetToImm64(imm_value, dirty);
}

BitSet64 GPRRegCache::GetRegUtilization() const
{
  return BiggerBitSet(HOT_STATE->op->regsInUse);
}

BitSet64 GPRRegCache::CountRegsIn(preg_t preg, u32 lookahead) const
{
  BitSet64 regs_used;

  for (u32 i = 1; i < lookahead; i++)
  {
    BitSet64 regs_in = BiggerBitSet(HOT_STATE->op[i].regsIn);
    regs_used |= regs_in;
    if (regs_in[preg])
      return regs_used;
  }

  return regs_used;
}

void GPRRegCache::StoreRegister32(preg_t preg, const OpArg& new_loc)
{
    abort();
}

void GPRRegCache::LoadRegister32(preg_t preg, X64Reg new_loc, bool flush_upper)
{
    abort();
}

void GPRRegCache::FlushUpper(preg_t preg)
{
    abort();
}

