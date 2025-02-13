// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RegCache/JitRegCache.h"

class GPRRegCache final : public RegCache
{
public:
  explicit GPRRegCache();
  void SetImmediate64(preg_t preg, u64 imm_value, bool dirty = true);

protected:
  Gen::OpArg GetDefaultLocation(preg_t preg) const override;
  Gen::OpArg GetDefaultLocation32(preg_t preg) const override;
  void StoreRegister(preg_t preg, const Gen::OpArg& new_loc) override;
  void LoadRegister(preg_t preg, Gen::X64Reg new_loc) override;
  void StoreRegister32(preg_t preg, const Gen::OpArg& new_loc, bool flush_upper) override;
  void LoadRegister32(preg_t preg, Gen::X64Reg new_loc) override;
  const Gen::X64Reg* GetAllocationOrder(size_t* count) const override;
  BitSet32 GetRegUtilization() const override;
  BitSet32 CountRegsIn(preg_t preg, u32 lookahead) const override;
};
