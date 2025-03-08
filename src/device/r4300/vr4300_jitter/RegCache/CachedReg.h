// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <optional>

#include "Common/Assert.h"
#include "Common/CommonTypes.h"
#include "Common/x64Emitter.h"
#include "RegCache/RCMode.h"

using preg_t = ssize_t;

class VR4300CachedReg
{
public:
  enum class LocationType
  {
    /// Value is currently at its default location
    Default,
    /// Value is not stored anywhere because we know it won't be read before the next write
    Discarded,
    /// Value is currently bound to a x64 register
    Bound,
    /// Value is known as an immediate and has not been written back to its default location
    Immediate,
    /// Value is known as an immediate and is already present at its default location
    SpeculativeImmediate,
  };

  VR4300CachedReg() = default;

  explicit VR4300CachedReg(Gen::OpArg default_location_, Gen::OpArg default_location32_)
      : default_location(default_location_), default_location32(default_location32_), location(default_location_)
  {
  }

  const std::optional<Gen::OpArg>& Location() const { return location; }
  const Gen::OpArg& DefaultLocation32() const { return default_location32; }

  LocationType GetLocationType() const
  {
    if (!location.has_value())
      return LocationType::Discarded;

    if (!away)
    {
      ASSERT(!revertable);

      if (location->IsImm())
        return LocationType::SpeculativeImmediate;

      ASSERT(*location == default_location);
      return LocationType::Default;
    }

    ASSERT(location->IsImm() || location->IsSimpleReg());
    return location->IsImm() ? LocationType::Immediate : LocationType::Bound;
  }

  bool IsAway() const { return away; }
  bool IsDiscarded() const { return !location.has_value(); }
  bool IsBound() const { return GetLocationType() == LocationType::Bound; }

  void SetBoundTo(Gen::X64Reg xreg)
  {
    away = true;
    location = Gen::R(xreg);
  }

  void SetDiscarded()
  {
    ASSERT(!revertable);
    away = false;
    location = std::nullopt;
    only_32_bit = false;
  }

  void SetFlushed()
  {
    ASSERT(!revertable);
    away = false;
    location = default_location;
    only_32_bit = false;
  }

  void SetToImm64(u64 imm64, bool dirty = true)
  {
    away |= dirty;
    location = Gen::Imm64(imm64);
  }

  void Set32BitOnly(bool only_32bit)
  {
    only_32_bit = only_32bit;
  }

  void SetFlushUpper(bool flush_upper)
  {
    m_flush_upper = flush_upper;
  }

  bool Is32BitOnly() const
  {
    return only_32_bit;
  }

  bool FlushUpper()
  {
    return m_flush_upper;
  }

  bool IsRevertable() const { return revertable; }
  void SetRevertable()
  {
    ASSERT(IsBound());
    revertable = true;
  }
  void SetRevert()
  {
    ASSERT(revertable);
    revertable = false;
    SetFlushed();
  }
  void SetCommit()
  {
    ASSERT(revertable);
    revertable = false;
  }

  bool IsLocked() const { return locked > 0; }
  void Lock() { locked++; }
  void Unlock()
  {
    ASSERT(IsLocked());
    locked--;
  }

private:
  Gen::OpArg default_location{};
  Gen::OpArg default_location32{};
  std::optional<Gen::OpArg> location{};
  bool away = false;  // value not in source register
  bool revertable = false;
  size_t locked = 0;
  bool only_32_bit = false;
  bool m_flush_upper = false;
};

class X64CachedReg
{
public:
  preg_t Contents() const { return vr4300Reg; }

  void SetBoundTo(preg_t vr4300Reg_, bool dirty_)
  {
    free = false;
    vr4300Reg = vr4300Reg_;
    dirty = dirty_;
  }

  void Unbind()
  {
    vr4300Reg = static_cast<preg_t>(Gen::INVALID_REG);
    free = true;
    dirty = false;
  }

  bool IsFree() const { return free && !locked; }

  bool IsDirty() const { return dirty; }
  void MakeDirty() { dirty = true; }

  bool IsLocked() const { return locked > 0; }
  void Lock() { locked++; }
  void Unlock()
  {
    ASSERT(IsLocked());
    locked--;
  }

private:
  preg_t vr4300Reg = static_cast<preg_t>(Gen::INVALID_REG);
  bool free = true;
  bool dirty = false;
  size_t locked = 0;
};

class RCConstraint
{
public:
  bool IsRealized() const { return realized != RealizedLoc::Invalid; }
  bool IsActive() const
  {
    return IsRealized() || write || read || kill_imm || kill_mem || revertable;
  }

  bool Is32BitOnly() const
  {
    return only_32_bit;
  }

  bool ShouldLoad() const { return read; }
  bool ShouldDirty() const { return write; }
  bool ShouldBeRevertable() const { return revertable; }
  bool ShouldKillImmediate() const { return kill_imm; }
  bool ShouldKillMemory() const { return kill_mem; }
  bool ShouldFlushUpper() const { return m_flush_upper; }

  enum class RealizedLoc
  {
    Invalid,
    Bound,
    Imm,
    Mem,
  };

  void Realized(RealizedLoc loc)
  {
    realized = loc;
    ASSERT(IsRealized());
  }

  enum class ConstraintLoc
  {
    Bound,
    BoundOrImm,
    BoundOrMem,
    Any,
  };

  void AddUse(RCMode mode) { AddConstraint(mode, ConstraintLoc::Any, false, false, false); }
  void AddUseNoImm(RCMode mode) { AddConstraint(mode, ConstraintLoc::BoundOrMem, false, false, false); }
  void AddBindOrImm(RCMode mode) { AddConstraint(mode, ConstraintLoc::BoundOrImm, false, false, false); }
  void AddBind(RCMode mode) { AddConstraint(mode, ConstraintLoc::Bound, false, false, false); }
  void AddRevertableBind(RCMode mode) { AddConstraint(mode, ConstraintLoc::Bound, true, false, false); }

  void AddUse32(RCMode mode) { AddConstraint(mode, ConstraintLoc::Any, false, true, false); }
  void AddUse32NoImm(RCMode mode) { AddConstraint(mode, ConstraintLoc::BoundOrMem, false, true, false); }
  void AddBind32OrImm(RCMode mode, bool flush_upper) { AddConstraint(mode, ConstraintLoc::BoundOrImm, false, true, flush_upper); }
  void AddBind32(RCMode mode, bool flush_upper) { AddConstraint(mode, ConstraintLoc::Bound, false, true, flush_upper); }
  void AddRevertableBind32(RCMode mode, bool flush_upper) { AddConstraint(mode, ConstraintLoc::Bound, true, true, flush_upper); }

private:
  void AddConstraint(RCMode mode, ConstraintLoc loc, bool should_revertable, bool only_32bit, bool flush_upper)
  {
    if (IsRealized())
    {
      ASSERT(IsCompatible(mode, loc, should_revertable, only_32bit, flush_upper));
      return;
    }

    m_flush_upper = flush_upper;

    only_32_bit = only_32bit;

    if (should_revertable)
      revertable = true;

    switch (loc)
    {
    case ConstraintLoc::Bound:
      kill_imm = true;
      kill_mem = true;
      break;
    case ConstraintLoc::BoundOrImm:
      kill_mem = true;
      break;
    case ConstraintLoc::BoundOrMem:
      kill_imm = true;
      break;
    case ConstraintLoc::Any:
      break;
    }

    switch (mode)
    {
    case RCMode::Read:
      read = true;
      break;
    case RCMode::Write:
      write = true;
      break;
    case RCMode::ReadWrite:
      read = true;
      write = true;
      break;
    }
  }

  bool IsCompatible(RCMode mode, ConstraintLoc loc, bool should_revertable, bool only_32bit, bool flush_upper) const
  {
    if (should_revertable && !revertable)
    {
      ASSERT(false);
      return false;
    }

    if (m_flush_upper != flush_upper)
    {
      ASSERT(false);
      return false;
    }

    if (only_32bit != only_32_bit)
    {
      ASSERT(false);
      return false;
    }

    const bool is_loc_compatible = [&] {
      switch (loc)
      {
      case ConstraintLoc::Bound:
        ASSERT(realized == RealizedLoc::Bound);
        return realized == RealizedLoc::Bound;
      case ConstraintLoc::BoundOrImm:
        ASSERT(realized == RealizedLoc::Bound || realized == RealizedLoc::Imm);
        return realized == RealizedLoc::Bound || realized == RealizedLoc::Imm;
      case ConstraintLoc::BoundOrMem:
        ASSERT(realized == RealizedLoc::Bound || realized == RealizedLoc::Mem);
        return realized == RealizedLoc::Bound || realized == RealizedLoc::Mem;
      case ConstraintLoc::Any:
        return true;
      }
      ASSERT(false);
      return false;
    }();

    const bool is_mode_compatible = [&] {
      switch (mode)
      {
      case RCMode::Read:
        ASSERT(read);
        return read;
      case RCMode::Write:
        ASSERT(write);
        return write;
      case RCMode::ReadWrite:
        ASSERT(read && write);
        return read && write;
      }
      ASSERT(false);
      return false;
    }();

    return is_loc_compatible && is_mode_compatible;
  }

  RealizedLoc realized = RealizedLoc::Invalid;
  bool write = false;
  bool read = false;
  bool kill_imm = false;
  bool kill_mem = false;
  bool revertable = false;
  bool only_32_bit = false;
  bool m_flush_upper = false;
};
