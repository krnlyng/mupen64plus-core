// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RegCache/JitRegCache.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <variant>

#include "Common/Assert.h"
#include "Common/BitSet.h"
#include "Common/CommonTypes.h"
#include "Common/EnumUtils.h"
#include "Common/VariantUtil.h"
#include "Common/x64Emitter.h"
#include "RegCache/CachedReg.h"
#include "RegCache/RCMode.h"
#include "../util.h"
#include "../vr4300_jitter.h"
#include "../vr4300_jitter_instruction_decoder.h"

#include <cassert>

using namespace Gen;

RCOpArg RCOpArg::Imm64(u64 imm)
{
  return RCOpArg{imm};
}

RCOpArg RCOpArg::R(X64Reg xr)
{
  return RCOpArg{xr};
}

RCOpArg::RCOpArg() = default;

RCOpArg::RCOpArg(u64 imm) : rc(nullptr), contents(imm)
{
}

RCOpArg::RCOpArg(X64Reg xr) : rc(nullptr), contents(xr)
{
}

RCOpArg::RCOpArg(RegCache* rc_, preg_t preg) : rc(rc_), contents(preg)
{
  rc->Lock(preg);
}

RCOpArg::~RCOpArg()
{
  Unlock();
}

RCOpArg::RCOpArg(RCOpArg&& other) noexcept
    : rc(std::exchange(other.rc, nullptr)),
      contents(std::exchange(other.contents, std::monostate{}))
{
}

RCOpArg& RCOpArg::operator=(RCOpArg&& other) noexcept
{
  Unlock();
  rc = std::exchange(other.rc, nullptr);
  contents = std::exchange(other.contents, std::monostate{});
  return *this;
}

RCOpArg::RCOpArg(RCX64Reg&& other) noexcept
    : rc(std::exchange(other.rc, nullptr)),
      contents(VariantCast(std::exchange(other.contents, std::monostate{})))
{
}

RCOpArg& RCOpArg::operator=(RCX64Reg&& other) noexcept
{
  Unlock();
  rc = std::exchange(other.rc, nullptr);
  contents = VariantCast(std::exchange(other.contents, std::monostate{}));
  return *this;
}

void RCOpArg::Realize()
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    rc->Realize(*preg);
  }
}

OpArg RCOpArg::Location() const
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    ASSERT(rc->IsRealized(*preg));
    return rc->R(*preg);
  }
  else if (const X64Reg* xr = std::get_if<X64Reg>(&contents))
  {
    return Gen::R(*xr);
  }
  else if (const u64* imm = std::get_if<u64>(&contents))
  {
    if (vr4300_jitter_value_fits_in_32_bit_imm_positive(*imm)) {
      return Gen::Imm32(*imm);
    } else {
      return Gen::Imm64(*imm);
    }
  }
  ASSERT(false);
  return {};
}

OpArg RCOpArg::ExtractWithByteOffset(int offset)
{
  if (offset == 0)
    return Location();

  ASSERT(rc);
  const preg_t preg = std::get<preg_t>(contents);
  rc->StoreFromRegister(preg, RegCache::FlushMode::MaintainState);
  OpArg result = rc->GetDefaultLocation(preg);
  result.AddMemOffset(offset);
  return result;
}

void RCOpArg::Unlock()
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    ASSERT(rc);
    rc->Unlock(*preg);
  }
  else if (const X64Reg* xr = std::get_if<X64Reg>(&contents))
  {
    // If rc, we got this from an RCX64Reg.
    // If !rc, we got this from RCOpArg::R.
    if (rc)
      rc->UnlockX(*xr);
  }
  else
  {
    ASSERT(!rc);
  }

  rc = nullptr;
  contents = std::monostate{};
}

bool RCOpArg::IsImm() const
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    return rc->R(*preg).IsImm();
  }
  else if (std::holds_alternative<u64>(contents))
  {
    return true;
  }
  return false;
}

s64 RCOpArg::SImm64() const
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    return rc->R(*preg).SImm64();
  }
  else if (const u64* imm = std::get_if<u64>(&contents))
  {
    return static_cast<s64>(*imm);
  }
  ASSERT(false);
  return 0;
}

u64 RCOpArg::Imm64() const
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    return rc->R(*preg).Imm64();
  }
  else if (const u64* imm = std::get_if<u64>(&contents))
  {
    return *imm;
  }
  ASSERT(false);
  return 0;
}

RCX64Reg::RCX64Reg() = default;

RCX64Reg::RCX64Reg(RegCache* rc_, preg_t preg) : rc(rc_), contents(preg)
{
  rc->Lock(preg);
}

RCX64Reg::RCX64Reg(RegCache* rc_, X64Reg xr) : rc(rc_), contents(xr)
{
  rc->LockX(xr);
}

RCX64Reg::~RCX64Reg()
{
  Unlock();
}

RCX64Reg::RCX64Reg(RCX64Reg&& other) noexcept
    : rc(std::exchange(other.rc, nullptr)),
      contents(std::exchange(other.contents, std::monostate{}))
{
}

RCX64Reg& RCX64Reg::operator=(RCX64Reg&& other) noexcept
{
  Unlock();
  rc = std::exchange(other.rc, nullptr);
  contents = std::exchange(other.contents, std::monostate{});
  return *this;
}

void RCX64Reg::Realize()
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    rc->Realize(*preg);
  }
}

RCX64Reg::operator X64Reg() const&
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    ASSERT(rc->IsRealized(*preg));
    return rc->RX(*preg);
  }
  else if (const X64Reg* xr = std::get_if<X64Reg>(&contents))
  {
    return *xr;
  }
  ASSERT(false);
  return {};
}

RCX64Reg::operator OpArg() const&
{
  return Gen::R(RCX64Reg::operator X64Reg());
}

void RCX64Reg::Unlock()
{
  if (const preg_t* preg = std::get_if<preg_t>(&contents))
  {
    ASSERT(rc);
    rc->Unlock(*preg);
  }
  else if (const X64Reg* xr = std::get_if<X64Reg>(&contents))
  {
    ASSERT(rc);
    rc->UnlockX(*xr);
  }
  else
  {
    ASSERT(!rc);
  }

  rc = nullptr;
  contents = std::monostate{};
}

RCForkGuard::RCForkGuard(RegCache& rc_) : rc(&rc_), m_regs(rc_.m_regs), m_xregs(rc_.m_xregs)
{
  ASSERT(!rc->IsAnyConstraintActive());
}

RCForkGuard::RCForkGuard(RCForkGuard&& other) noexcept
    : rc(other.rc), m_regs(std::move(other.m_regs)), m_xregs(std::move(other.m_xregs))
{
  other.rc = nullptr;
}

void RCForkGuard::EndFork()
{
  if (!rc)
    return;

  ASSERT(!rc->IsAnyConstraintActive());
  rc->m_regs = m_regs;
  rc->m_xregs = m_xregs;
  rc = nullptr;
}

RegCache::RegCache()
{
}

void RegCache::Start()
{
  m_xregs.fill({});
  for (size_t i = 0; i < m_regs.size(); i++)
  {
    m_regs[i] = VR4300CachedReg{GetDefaultLocation(i), GetDefaultLocation32(i), GetDefaultLocation32s(i), GetDefaultLocation32d(i), GetDefaultLocation32f(i)};
  }
}

void RegCache::SetEmitter(XEmitter* emitter)
{
  m_emitter = emitter;
}

bool RegCache::SanityCheck() const
{
  for (std::array<VR4300CachedReg, 32>::size_type i = 0; i < m_regs.size(); i++)
  {
    if (!m_reg_0_usable && i == 0 && m_regs[i].GetLocationType() != VR4300CachedReg::LocationType::Default) abort();
    switch (m_regs[i].GetLocationType())
    {
    case VR4300CachedReg::LocationType::Default:
    case VR4300CachedReg::LocationType::Discarded:
    case VR4300CachedReg::LocationType::SpeculativeImmediate:
    case VR4300CachedReg::LocationType::Immediate:
      break;
    case VR4300CachedReg::LocationType::Bound:
    {
      if (m_regs[i].IsLocked() || m_regs[i].IsRevertable())
        return false;

      Gen::X64Reg xr = m_regs[i].Location()->GetSimpleReg();
      if (m_xregs[xr].IsLocked())
        return false;
      if (m_xregs[xr].Contents() != i)
        return false;
      break;
    }
    }
  }
  return true;
}

void RegCache::SetReg0Unusable()
{
    m_reg_0_usable = false;
}

RCOpArg RegCache::Use(preg_t preg, RCMode mode, bool only_32bit)
{
  if (!m_reg_0_usable && preg == 0) abort();
  m_constraints[preg].AddUse(mode, only_32bit);
  return RCOpArg{this, preg};
}

RCOpArg RegCache::UseS(preg_t preg, RCMode mode, bool only_32bit)
{
  if (!m_reg_0_usable && preg == 0) abort();
  m_constraints[preg].AddUseS(mode, only_32bit);
  return RCOpArg{this, preg};
}

RCOpArg RegCache::UseF(preg_t preg, RCMode mode, bool only_32bit)
{
  if (!m_reg_0_usable && preg == 0) abort();
  m_constraints[preg].AddUseF(mode, only_32bit);
  return RCOpArg{this, preg};
}

RCOpArg RegCache::UseNoImm(preg_t preg, RCMode mode)
{
  if (!m_reg_0_usable && preg == 0) abort();
  m_constraints[preg].AddUseNoImm(mode);
  return RCOpArg{this, preg};
}

RCOpArg RegCache::BindOrImm(preg_t preg, RCMode mode, bool flush_upper)
{
  if (!m_reg_0_usable && preg == 0) abort();
  m_constraints[preg].AddBindOrImm(mode, flush_upper);
  return RCOpArg{this, preg};
}

void RegCache::ValidateRCMode(preg_t preg, RCMode mode, bool only_32bit)
{
  if (!m_reg_0_usable && preg == 0) abort();
  if (!m_reg_0_usable) { //gpr
    if (mode == RCMode::Read) {
      VALIDATE_REG_IN(HOT_STATE->op, preg);
    }
    if (mode == RCMode::Write) {
      VALIDATE_REG_OUT(HOT_STATE->op, preg);
    }
    if (mode == RCMode::ReadWrite) {
      VALIDATE_REG_IN(HOT_STATE->op, preg);
      VALIDATE_REG_OUT(HOT_STATE->op, preg);
    }
  } else { // float
    if (only_32bit) {
      if (mode == RCMode::Read) {
        VALIDATE_REG_FIN32(HOT_STATE->op, preg);
      }
      if (mode == RCMode::Write) {
        VALIDATE_REG_FOUT32(HOT_STATE->op, preg);
      }
      if (mode == RCMode::ReadWrite) {
        VALIDATE_REG_FIN32(HOT_STATE->op, preg);
        VALIDATE_REG_FOUT32(HOT_STATE->op, preg);
      }
    } else {
      if (mode == RCMode::Read) {
        VALIDATE_REG_FIN(HOT_STATE->op, preg);
      }
      if (mode == RCMode::Write) {
        VALIDATE_REG_FOUT(HOT_STATE->op, preg);
      }
      if (mode == RCMode::ReadWrite) {
        VALIDATE_REG_FIN(HOT_STATE->op, preg);
        VALIDATE_REG_FOUT(HOT_STATE->op, preg);
      }
    }
  }
}

RCX64Reg RegCache::Bind(preg_t preg, RCMode mode, bool only_32bit, bool flush_upper)
{
  ValidateRCMode(preg, mode, only_32bit);
  m_constraints[preg].AddBind(mode, only_32bit, flush_upper);
  return RCX64Reg{this, preg};
}

RCX64Reg RegCache::RevertableBind(preg_t preg, RCMode mode, bool only_32bit, bool flush_upper)
{
  ValidateRCMode(preg, mode, only_32bit);
  m_constraints[preg].AddRevertableBind(mode, only_32bit, flush_upper);
  return RCX64Reg{this, preg};
}

RCX64Reg RegCache::Scratch()
{
  return Scratch(GetFreeXReg());
}

RCX64Reg RegCache::Scratch(X64Reg xr)
{
  FlushX(xr);
  return RCX64Reg{this, xr};
}

RCForkGuard RegCache::Fork()
{
  return RCForkGuard{*this};
}

void RegCache::Discard(BitSet32 pregs)
{
  ASSERT_MSG(
      DYNA_REC,
      std::none_of(m_xregs.begin(), m_xregs.end(), [](const auto& x) { return x.IsLocked(); }),
      "Someone forgot to unlock a X64 reg");

  for (preg_t i : pregs)
  {
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsLocked(), "Someone forgot to unlock VR4300 reg %d (X64 reg %d).",
               i, Common::ToUnderlying(RX(i)));
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Register transaction is in progress for %d!",
               i);

    if (m_reg_0_usable) {
      if (m_regs[i].FlushUpper())
      {
        fprintf(stderr, "FLUSH UPPER %d\n",i);
        FlushUpper(i);
      }
    }

    if (m_regs[i].IsBound())
    {
      X64Reg xr = RX(i);
      m_xregs[xr].Unbind();
    }

    m_regs[i].SetDiscarded();
  }
}

void RegCache::ConvertTo64(BitSet32 pregs)
{
  for (preg_t i : pregs)
  {
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Register transaction is in progress for %d!",
               i);

    switch (m_regs[i].GetLocationType())
    {
    case VR4300CachedReg::LocationType::Default:
      break;
    case VR4300CachedReg::LocationType::Discarded:
      ASSERT_MSG(DYNA_REC, false, "Attempted to flush discarded VR4300 reg %d", i);
      break;
    case VR4300CachedReg::LocationType::SpeculativeImmediate:
      // We can have a cached value without a host register through speculative constants.
      // It must be cleared when flushing, otherwise it may be out of sync with VR4300STATE,
      // if VR4300STATE is modified externally (e.g. fallback to interpreter).
      m_regs[i].SetFlushed();
      break;
    case VR4300CachedReg::LocationType::Bound:
    case VR4300CachedReg::LocationType::Immediate:
      StoreFromRegister(i);
      m_regs[i].Set32BitOnly(false);
      break;
    }
  }
}

void RegCache::FlushDifferentUse(BitSet32 pregs)
{
  for (preg_t i : pregs)
  {
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Register transaction is in progress for %d!",
               i);

    switch (m_regs[i].GetLocationType())
    {
    case VR4300CachedReg::LocationType::Default:
      break;
    case VR4300CachedReg::LocationType::Discarded:
      ASSERT_MSG(DYNA_REC, false, "Attempted to flush discarded VR4300 reg %d", i);
      break;
    case VR4300CachedReg::LocationType::SpeculativeImmediate:
      // We can have a cached value without a host register through speculative constants.
      // It must be cleared when flushing, otherwise it may be out of sync with VR4300STATE,
      // if VR4300STATE is modified externally (e.g. fallback to interpreter).
      m_regs[i].SetFlushed();
      break;
    case VR4300CachedReg::LocationType::Bound:
    case VR4300CachedReg::LocationType::Immediate:
      StoreFromRegister(i);
      m_regs[i].SetIsSUse(false);
      break;
    }
  }
}

void RegCache::ConvertTo32(BitSet32 pregs)
{
  for (preg_t i : pregs)
  {
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Register transaction is in progress for %d!",
               i);

    switch (m_regs[i].GetLocationType())
    {
    case VR4300CachedReg::LocationType::Default:
      break;
    case VR4300CachedReg::LocationType::Discarded:
      ASSERT_MSG(DYNA_REC, false, "Attempted to flush discarded VR4300 reg %d", i);
      break;
    case VR4300CachedReg::LocationType::SpeculativeImmediate:
      // We can have a cached value without a host register through speculative constants.
      // It must be cleared when flushing, otherwise it may be out of sync with VR4300STATE,
      // if VR4300STATE is modified externally (e.g. fallback to interpreter).
      m_regs[i].SetFlushed();
      break;
    case VR4300CachedReg::LocationType::Bound:
    case VR4300CachedReg::LocationType::Immediate:
      StoreFromRegister(i);
      m_regs[i].Set32BitOnly(true);
      break;
    }
  }
}

void RegCache::Flush(BitSet32 pregs)
{
  ASSERT_MSG(
      DYNA_REC,
      std::none_of(m_xregs.begin(), m_xregs.end(), [](const auto& x) { return x.IsLocked(); }),
      "Someone forgot to unlock a X64 reg");

  for (preg_t i : pregs)
  {
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsLocked(), "Someone forgot to unlock VR4300 reg %d (X64 reg %d).",
               i, Common::ToUnderlying(RX(i)));
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Register transaction is in progress for %d!",
               i);

    switch (m_regs[i].GetLocationType())
    {
    case VR4300CachedReg::LocationType::Default:
      break;
    case VR4300CachedReg::LocationType::Discarded:
      ASSERT_MSG(DYNA_REC, false, "Attempted to flush discarded VR4300 reg %d", i);
      break;
    case VR4300CachedReg::LocationType::SpeculativeImmediate:
      // We can have a cached value without a host register through speculative constants.
      // It must be cleared when flushing, otherwise it may be out of sync with VR4300STATE,
      // if VR4300STATE is modified externally (e.g. fallback to interpreter).
      m_regs[i].SetFlushed();
      break;
    case VR4300CachedReg::LocationType::Bound:
    case VR4300CachedReg::LocationType::Immediate:
      StoreFromRegister(i);
      break;
    }
  }
}

void RegCache::Reset(BitSet32 pregs)
{
  for (preg_t i : pregs)
  {
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsAway(),
               "Attempted to reset a loaded register (did you mean to flush it?)");
    m_regs[i].SetFlushed();
  }
}

void RegCache::Revert()
{
  ASSERT(IsAllUnlocked());
  for (auto& reg : m_regs)
  {
    if (reg.IsRevertable())
      reg.SetRevert();
  }
}

void RegCache::Commit()
{
  ASSERT(IsAllUnlocked());
  for (auto& reg : m_regs)
  {
    if (reg.IsRevertable()) {
      reg.SetCommit();
    }
  }
}

bool RegCache::IsAllUnlocked() const
{
  return std::none_of(m_regs.begin(), m_regs.end(), [](const auto& r) { return r.IsLocked(); }) &&
         std::none_of(m_xregs.begin(), m_xregs.end(), [](const auto& x) { return x.IsLocked(); }) &&
         !IsAnyConstraintActive();
}

void RegCache::PreloadRegisters(BitSet32 to_preload, bool only32_bit, bool is_s_use, bool is_f_use)
{
  for (preg_t preg : to_preload)
  {
    if (NumFreeRegisters() < 2)
      return;
    if (!m_regs[preg].Location().value().IsImm()) {
      BindToRegister(preg, true, false, only32_bit, false, is_s_use, is_f_use);
    }
  }
}

BitSet32 RegCache::RegistersInUse() const
{
  BitSet32 result;
  for (size_t i = 0; i < m_xregs.size(); i++)
  {
    if (!m_xregs[i].IsFree())
      result[i] = true;
  }
  return result;
}

void RegCache::FlushX(X64Reg reg)
{
  ASSERT_MSG(DYNA_REC, reg < m_xregs.size(), "Flushing non-existent reg %d",
             Common::ToUnderlying(reg));
  ASSERT(!m_xregs[reg].IsLocked());
  if (!m_xregs[reg].IsFree())
  {
    StoreFromRegister(m_xregs[reg].Contents());
  }
}

void RegCache::DiscardRegContentsIfCached(preg_t preg)
{
  if (m_regs[preg].IsBound())
  {
    X64Reg xr = m_regs[preg].Location()->GetSimpleReg();
    m_xregs[xr].Unbind();
    m_regs[preg].SetFlushed();
  }
}

void RegCache::BindToRegister(preg_t i, bool doLoad, bool makeDirty, bool only_32bit, bool flush_upper, bool is_s_use, bool is_f_use)
{
#if 1
    //if (doLoad) {
    if (m_reg_0_usable) {
        if (m_regs[i].IsBound()) {
          if (makeDirty)
            m_xregs[RX(i)].MakeDirty();
        }
        if (m_regs[i].IsBound()) {
            if (flush_upper != m_regs[i].FlushUpper() && m_regs[i].FlushUpper()) {
                //FlushUpper(i);
                FlushDifferentUse(BitSet32{i});
            }
            if (is_s_use != m_regs[i].IsSUse()) {
                FlushDifferentUse(BitSet32{i});
            }
            if (is_f_use != m_regs[i].IsFUse()) {
                FlushDifferentUse(BitSet32{i});
            }
        }
        if (doLoad && m_regs[i].IsBound()/* && m_xregs[RX(i)].IsDirty()*/) {
            if (only_32bit && !Is32BitOnly(i)) {
                ConvertTo32(BitSet32{i});
            } else if (!only_32bit && Is32BitOnly(i)) {
                if (m_regs[i].IsSUse()) abort();
                ConvertTo64(BitSet32{i});
            }
        } else {
            m_regs[i].Set32BitOnly(only_32bit);
        }
    }
#endif

  if (!m_regs[i].IsBound())
  {
    X64Reg xr = GetFreeXReg();

    ASSERT_MSG(DYNA_REC, !m_xregs[xr].IsDirty(), "Xreg %d already dirty", Common::ToUnderlying(xr));
    ASSERT_MSG(DYNA_REC, !m_xregs[xr].IsLocked(), "GetFreeXReg returned locked register");
    ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Invalid transaction state");

    m_xregs[xr].SetBoundTo(i, makeDirty || m_regs[i].IsAway());

    if (doLoad)
    {
      ASSERT_MSG(DYNA_REC, !m_regs[i].IsDiscarded(), "Attempted to load a discarded value");
      if (only_32bit) {
        LoadRegister32(i, xr, is_s_use, is_f_use);
      } else {
        LoadRegister(i, xr);
      }
    }

    m_regs[i].SetIsSUse(is_s_use);
    m_regs[i].SetIsFUse(is_f_use);
    m_regs[i].SetFlushUpper(flush_upper);
    m_regs[i].Set32BitOnly(only_32bit);

    ASSERT_MSG(DYNA_REC,
               std::none_of(m_regs.begin(), m_regs.end(),
                            [xr](const auto& r) {
                              return r.Location().has_value() && r.Location()->IsSimpleReg(xr);
                            }),
               "Xreg %d already bound", Common::ToUnderlying(xr));

    m_regs[i].SetBoundTo(xr);
  }
  else
  {
    // reg location must be simplereg; memory locations
    // and immediates are taken care of above.
    if (makeDirty)
      m_xregs[RX(i)].MakeDirty();
  }

  ASSERT_MSG(DYNA_REC, !m_xregs[RX(i)].IsLocked(),
             "WTF, this reg (%d -> %d) should have been flushed", i, Common::ToUnderlying(RX(i)));
}

void RegCache::StoreFromRegister(preg_t i, FlushMode mode)
{
  if (!m_reg_0_usable && i == 0) abort();

  // When a transaction is in progress, allowing the store would overwrite the old value.
  ASSERT_MSG(DYNA_REC, !m_regs[i].IsRevertable(), "Register transaction on %d is in progress!", i);

  bool doStore = false;

  switch (m_regs[i].GetLocationType())
  {
  case VR4300CachedReg::LocationType::Default:
  case VR4300CachedReg::LocationType::Discarded:
  case VR4300CachedReg::LocationType::SpeculativeImmediate:
    return;
  case VR4300CachedReg::LocationType::Bound:
  {
    X64Reg xr = RX(i);
    doStore = m_xregs[xr].IsDirty();
    if (mode == FlushMode::Full)
      m_xregs[xr].Unbind();
    break;
  }
  case VR4300CachedReg::LocationType::Immediate:
    doStore = true;
    break;
  }

  if (doStore)
  {
    if (Is32BitOnly(i))
    {
      StoreRegister32(i, m_regs[i].IsFUse() ? GetDefaultLocation32f(i) : (m_regs[i].FlushUpper() ? GetDefaultLocation32d(i) : (m_regs[i].IsSUse() ? GetDefaultLocation32s(i) : GetDefaultLocation32(i))), m_regs[i].FlushUpper());
    }
    else
    {
      StoreRegister(i, GetDefaultLocation(i));
    }
  }
  if (mode == FlushMode::Full)
    m_regs[i].SetFlushed();
}

X64Reg RegCache::GetFreeXReg()
{
  size_t aCount;
  const X64Reg* aOrder = GetAllocationOrder(&aCount);
  for (size_t i = 0; i < aCount; i++)
  {
    X64Reg xr = aOrder[i];
    if (m_xregs[xr].IsFree() && !m_stolen_regs[xr])
    {
      return xr;
    }
  }

  // Okay, not found; run the register allocator heuristic and figure out which register we should
  // clobber.
  float min_score = std::numeric_limits<float>::max();
  X64Reg best_xreg = INVALID_REG;
  size_t best_preg = 0;
  for (size_t i = 0; i < aCount; i++)
  {
    X64Reg xreg = (X64Reg)aOrder[i];
    if (m_stolen_regs[xreg])
      continue;
    preg_t preg = m_xregs[xreg].Contents();
    if (m_xregs[xreg].IsLocked() || m_regs[preg].IsLocked())
      continue;
    float score = ScoreRegister(xreg);
    if (score < min_score)
    {
      min_score = score;
      best_xreg = xreg;
      best_preg = preg;
    }
  }

  if (best_xreg != INVALID_REG)
  {
    StoreFromRegister(best_preg);
    return best_xreg;
  }

  // Still no dice? Die!
  ASSERT_MSG(DYNA_REC, false, "Regcache ran out of regs");
  return INVALID_REG;
}

int RegCache::NumFreeRegisters() const
{
  int count = 0;
  size_t aCount;
  const X64Reg* aOrder = GetAllocationOrder(&aCount);
  for (size_t i = 0; i < aCount; i++)
    if (m_xregs[aOrder[i]].IsFree() && !m_stolen_regs[aOrder[i]])
      count++;
  return count;
}

// Estimate roughly how bad it would be to de-allocate this register. Higher score
// means more bad.
float RegCache::ScoreRegister(X64Reg xreg) const
{
  preg_t preg = m_xregs[xreg].Contents();
  float score = 0;

  // If it's not dirty, we don't need a store to write it back to the register file, so
  // bias a bit against dirty registers. Testing shows that a bias of 2 seems roughly
  // right: 3 causes too many extra clobbers, while 1 saves very few clobbers relative
  // to the number of extra stores it causes.
  if (m_xregs[xreg].IsDirty())
    score += 2;

  // If the register isn't actually needed in a physical register for a later instruction,
  // writing it back to the register file isn't quite as bad.
  if (GetRegUtilization()[preg])
  {
    // Don't look too far ahead; we don't want to have quadratic compilation times for
    // enormous block sizes!
    // This actually improves register allocation a tiny bit; I'm not sure why.
    u32 lookahead = std::min(HOT_STATE->instructionsLeft, 64);
    // Count how many other registers are going to be used before we need this one again.
    u32 regs_in_count = CountRegsIn(preg, lookahead).Count();
    // Totally ad-hoc heuristic to bias based on how many other registers we'll need
    // before this one gets used again.
    score += 1 + 2 * (5 - log2f(1 + (float)regs_in_count));
  }

  return score;
}

const OpArg& RegCache::R(preg_t preg) const
{
  ASSERT_MSG(DYNA_REC, !m_regs[preg].IsDiscarded(), "Discarded register - %d", preg);
  if (Is32BitOnly(preg) && m_regs[preg].GetLocationType() == VR4300CachedReg::LocationType::Default) {
    if (m_regs[preg].IsFUse()) {
        return m_regs[preg].DefaultLocation32f();
    }
    if (m_regs[preg].IsSUse()) {
        return m_regs[preg].DefaultLocation32s();
    }
    return m_regs[preg].DefaultLocation32();
  }
  return m_regs[preg].Location().value();
}

X64Reg RegCache::RX(preg_t preg) const
{
  ASSERT_MSG(DYNA_REC, m_regs[preg].IsBound(), "Unbound register - %d", preg);
  return m_regs[preg].Location()->GetSimpleReg();
}

void RegCache::Lock(preg_t preg)
{
  m_regs[preg].Lock();
}

void RegCache::Unlock(preg_t preg)
{
  m_regs[preg].Unlock();
  if (!m_regs[preg].IsLocked())
  {
    // Fully unlocked, reset realization state.
    m_constraints[preg] = {};
  }
}

void RegCache::LockX(X64Reg xr)
{
  m_xregs[xr].Lock();
}

void RegCache::UnlockX(X64Reg xr)
{
  m_xregs[xr].Unlock();
}

bool RegCache::IsRealized(preg_t preg) const
{
  if (!m_reg_0_usable && preg == 0) abort();
  return m_constraints[preg].IsRealized();
}

bool RegCache::Is32BitOnly(preg_t preg) const
{
  if (!m_reg_0_usable && preg == 0) abort();
  if (!m_reg_0_usable) return false;
  return m_regs[preg].Is32BitOnly();
}

void RegCache::Realize(preg_t preg)
{
  if (!m_reg_0_usable && preg == 0) abort();

  if (m_constraints[preg].IsRealized())
    return;

  const bool load = m_constraints[preg].ShouldLoad();
  const bool dirty = m_constraints[preg].ShouldDirty();
  const bool kill_imm = m_constraints[preg].ShouldKillImmediate();
  const bool kill_mem = m_constraints[preg].ShouldKillMemory();
  const bool flush_upper = m_constraints[preg].ShouldFlushUpper();
  const bool is_s_use = m_constraints[preg].IsSUse();
  const bool is_f_use = m_constraints[preg].IsFUse();

  const auto do_bind = [&] {
    BindToRegister(preg, load, dirty, m_constraints[preg].Is32BitOnly(), flush_upper, is_s_use, is_f_use);
    m_constraints[preg].Realized(RCConstraint::RealizedLoc::Bound);
  };

  if (m_constraints[preg].ShouldBeRevertable())
  {
    StoreFromRegister(preg, FlushMode::MaintainState);
    do_bind();
    m_regs[preg].SetRevertable();
    return;
  }

  switch (m_regs[preg].GetLocationType())
  {
  case VR4300CachedReg::LocationType::Default:
    if (kill_mem)
    {
      do_bind();
      return;
    }
    m_regs[preg].Set32BitOnly(m_constraints[preg].Is32BitOnly());
    m_regs[preg].SetFlushUpper(flush_upper);
    m_regs[preg].SetIsSUse(is_s_use);
    m_regs[preg].SetIsFUse(is_f_use);
    m_constraints[preg].Realized(RCConstraint::RealizedLoc::Mem);
    return;
  case VR4300CachedReg::LocationType::Discarded:
  case VR4300CachedReg::LocationType::Bound:
    do_bind();
    return;
  case VR4300CachedReg::LocationType::Immediate:
  case VR4300CachedReg::LocationType::SpeculativeImmediate:
    if (dirty || kill_imm
#if ALWAYS_BIND_64BIT_IMMEDIATES
 || !vr4300_jitter_value_fits_in_32_bit_imm_positive(Imm64(preg)))
#else
    )
#endif
    {
      do_bind();
      return;
    }
    m_constraints[preg].Realized(RCConstraint::RealizedLoc::Imm);
    break;
  }
}

bool RegCache::IsAnyConstraintActive() const
{
  return std::any_of(m_constraints.begin(), m_constraints.end(),
                     [](const auto& c) { return c.IsActive(); });
}

void RegCache::Steal(Gen::X64Reg reg)
{
  if (m_stolen_regs[reg]) abort();
  FlushX(reg);
  m_stolen_regs[reg] = true;
}

void RegCache::Unsteal(Gen::X64Reg reg)
{
  if (!m_stolen_regs[reg]) abort();
  m_stolen_regs[reg] = false;
}

bool RegCache::IsBound(preg_t preg) const
{
  return m_regs[preg].IsBound();
}

bool RegCache::IsDiscarded(preg_t preg) const
{
  return m_regs[preg].IsDiscarded();
}

bool RegCache::IsDirty(preg_t preg) const
{
  return m_xregs[RX(preg)].IsDirty();
}

