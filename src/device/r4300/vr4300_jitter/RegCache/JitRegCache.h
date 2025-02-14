// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <variant>

#include "../regs.h"
#include "Common/x64Emitter.h"
#include "RegCache/CachedReg.h"

enum class RCMode;

class RCOpArg;
class RCX64Reg;
class RegCache;

using preg_t = ssize_t;
static constexpr size_t NUM_XREGS = 16;

class RCOpArg
{
public:
  static RCOpArg Imm64(u64 imm);
  static RCOpArg R(Gen::X64Reg xr);
  RCOpArg();
  ~RCOpArg();
  RCOpArg(RCOpArg&&) noexcept;
  RCOpArg& operator=(RCOpArg&&) noexcept;

  RCOpArg(RCX64Reg&&) noexcept;
  RCOpArg& operator=(RCX64Reg&&) noexcept;

  RCOpArg(const RCOpArg&) = delete;
  RCOpArg& operator=(const RCOpArg&) = delete;

  void Realize();
  Gen::OpArg Location() const;
  operator Gen::OpArg() const& { return Location(); }
  operator Gen::OpArg() const&& = delete;
  bool IsSimpleReg() const { return Location().IsSimpleReg(); }
  bool IsSimpleReg(Gen::X64Reg reg) const { return Location().IsSimpleReg(reg); }
  Gen::X64Reg GetSimpleReg() const { return Location().GetSimpleReg(); }

  // Use to extract bytes from a register using the regcache. offset is in bytes.
  Gen::OpArg ExtractWithByteOffset(int offset);

  void Unlock();

  bool IsImm() const;
  s64 SImm64() const;
  u64 Imm64() const;
  bool IsZero() const { return IsImm() && Imm64() == 0; }

private:
  friend class RegCache;

  explicit RCOpArg(u64 imm);
  explicit RCOpArg(Gen::X64Reg xr);
  RCOpArg(RegCache* rc_, preg_t preg);

  RegCache* rc = nullptr;
  std::variant<std::monostate, Gen::X64Reg, u64, preg_t> contents;
};

class RCX64Reg
{
public:
  RCX64Reg();
  ~RCX64Reg();
  RCX64Reg(RCX64Reg&&) noexcept;
  RCX64Reg& operator=(RCX64Reg&&) noexcept;

  RCX64Reg(const RCX64Reg&) = delete;
  RCX64Reg& operator=(const RCX64Reg&) = delete;

  void Realize();
  operator Gen::OpArg() const&;
  operator Gen::X64Reg() const&;
  operator Gen::OpArg() const&& = delete;
  operator Gen::X64Reg() const&& = delete;

  void Unlock();

private:
  friend class RegCache;
  friend class RCOpArg;

  RCX64Reg(RegCache* rc_, preg_t preg);
  RCX64Reg(RegCache* rc_, Gen::X64Reg xr);

  RegCache* rc = nullptr;
  std::variant<std::monostate, Gen::X64Reg, preg_t> contents;
};

class RCForkGuard
{
public:
  ~RCForkGuard() { EndFork(); }
  RCForkGuard(RCForkGuard&&) noexcept;

  RCForkGuard(const RCForkGuard&) = delete;
  RCForkGuard& operator=(const RCForkGuard&) = delete;
  RCForkGuard& operator=(RCForkGuard&&) = delete;

  void EndFork();

private:
  friend class RegCache;

  explicit RCForkGuard(RegCache& rc_);

  RegCache* rc;
  std::array<VR4300CachedReg, 32> m_regs;
  std::array<X64CachedReg, NUM_XREGS> m_xregs;
};

class RegCache
{
public:
  enum class FlushMode
  {
    Full,
    MaintainState,
  };

  explicit RegCache();
  virtual ~RegCache() = default;

  void Start();
  void SetEmitter(Gen::XEmitter* emitter);
  void SetReg0Unusable();
  bool SanityCheck() const;

  template <typename... Ts>
  static void Realize(Ts&... rc)
  {
    static_assert(((std::is_same<Ts, RCOpArg>() || std::is_same<Ts, RCX64Reg>()) && ...));
    (rc.Realize(), ...);
  }

  template <typename... Ts>
  static void Unlock(Ts&... rc)
  {
    static_assert(((std::is_same<Ts, RCOpArg>() || std::is_same<Ts, RCX64Reg>()) && ...));
    (rc.Unlock(), ...);
  }

  template <typename... Args>
  bool IsImm(Args... pregs) const
  {
    static_assert(sizeof...(pregs) > 0);
    return (m_regs[pregs].Location().value().IsImm() && ...);
  }
  u64 Imm64(preg_t preg) const { return R(preg).Imm64(); }
  s64 SImm64(preg_t preg) const { return R(preg).SImm64(); }

  RCOpArg Use(preg_t preg, RCMode mode, bool only_32bit = false);
  RCOpArg UseF(preg_t preg, RCMode mode, bool only_32bit = false);
  RCOpArg UseS(preg_t preg, RCMode mode, bool only_32bit = false);
  RCOpArg UseNoImm(preg_t preg, RCMode mode);
  RCOpArg BindOrImm(preg_t preg, RCMode mode, bool flush_upper = true);
  RCX64Reg Bind(preg_t preg, RCMode mode, bool only_32bit = false, bool flush_upper = true);
  RCX64Reg BindS(preg_t preg, RCMode mode, bool only_32bit = false, bool flush_upper = true);
  RCX64Reg RevertableBind(preg_t preg, RCMode mode, bool only_32bit = false, bool flush_upper = true);
  RCX64Reg Scratch();
  RCX64Reg Scratch(Gen::X64Reg xr);

  RCForkGuard Fork();
  void Discard(BitSet32 pregs);
  void Flush(BitSet32 pregs = BitSet32::AllTrue(32));
  void ConvertTo64(BitSet32 pregs);
  void ConvertTo32(BitSet32 pregs);
  void FlushDifferentUse(BitSet32 pregs);
  void Reset(BitSet32 pregs);
  void Revert();
  void Commit();
  bool Is32BitOnly(preg_t preg) const;
  bool IsBound(preg_t preg) const;
  bool IsDiscarded(preg_t preg) const;
  bool IsDirty(preg_t preg) const;

  void Steal(Gen::X64Reg reg);
  void Unsteal(Gen::X64Reg reg);

  bool IsAllUnlocked() const;

  void PreloadRegisters(BitSet32 pregs, bool only_32bit, bool is_s_use, bool is_f_use);
  BitSet32 RegistersInUse() const;

protected:
  friend class RCOpArg;
  friend class RCX64Reg;
  friend class RCForkGuard;

  void ValidateRCMode(preg_t preg, RCMode mode, bool only_32bit);

  virtual Gen::OpArg GetDefaultLocation(preg_t preg) const = 0;
  virtual Gen::OpArg GetDefaultLocation32(preg_t preg) const = 0;
  virtual Gen::OpArg GetDefaultLocation32s(preg_t preg) const = 0;
  virtual Gen::OpArg GetDefaultLocation32d(preg_t preg) const = 0;
  virtual Gen::OpArg GetDefaultLocation32f(preg_t preg) const = 0;
  virtual void StoreRegister(preg_t preg, const Gen::OpArg& new_loc) = 0;
  virtual void LoadRegister(preg_t preg, Gen::X64Reg new_loc) = 0;
  virtual void StoreRegister32(preg_t preg, const Gen::OpArg& new_loc, bool flush_upper) = 0;
  virtual void LoadRegister32(preg_t preg, Gen::X64Reg new_loc, bool is_s_use, bool is_f_use) = 0;
  virtual void FlushUpper(preg_t preg) = 0;

  virtual const Gen::X64Reg* GetAllocationOrder(size_t* count) const = 0;

  virtual BitSet32 GetRegUtilization() const = 0;
  virtual BitSet32 CountRegsIn(preg_t preg, u32 lookahead) const = 0;

  void FlushX(Gen::X64Reg reg);
  void DiscardRegContentsIfCached(preg_t preg);
  void BindToRegister(preg_t preg, bool doLoad, bool makeDirty, bool only_32bit, bool flush_upper, bool is_s_use, bool is_f_use);
  void StoreFromRegister(preg_t preg, FlushMode mode = FlushMode::Full);

  Gen::X64Reg GetFreeXReg();

  int NumFreeRegisters() const;
  float ScoreRegister(Gen::X64Reg xreg) const;

  const Gen::OpArg& R(preg_t preg) const;
  Gen::X64Reg RX(preg_t preg) const;

  void Lock(preg_t preg);
  void Unlock(preg_t preg);
  void LockX(Gen::X64Reg xr);
  void UnlockX(Gen::X64Reg xr);
  bool IsRealized(preg_t preg) const;
  void Realize(preg_t preg);

  bool IsAnyConstraintActive() const;

  std::array<VR4300CachedReg, 32> m_regs;
  std::array<X64CachedReg, NUM_XREGS> m_xregs;
  std::array<RCConstraint, 32> m_constraints;
  Gen::XEmitter* m_emitter = nullptr;
  bool m_reg_0_usable = true;
  BitSet32 m_stolen_regs = BitSet32::AllTrue(0);
};
