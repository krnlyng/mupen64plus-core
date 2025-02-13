// Copyright 2016 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RegCache/JitRegCache.h"

class FPURegCache final : public RegCache
{
public:
  explicit FPURegCache();

protected:
  Gen::OpArg GetDefaultLocation(preg_t preg) const override;
  Gen::OpArg GetDefaultLocation32(preg_t preg) const override;
  void StoreRegister(preg_t preg, const Gen::OpArg& newLoc) override;
  void LoadRegister(preg_t preg, Gen::X64Reg newLoc) override;
  void StoreRegister32(preg_t preg, const Gen::OpArg& newLoc, bool flush_upper) override;
  void LoadRegister32(preg_t preg, Gen::X64Reg newLoc) override;
  const Gen::X64Reg* GetAllocationOrder(size_t* count) const override;
  BitSet32 GetRegUtilization() const override;
  BitSet32 CountRegsIn(preg_t preg, u32 lookahead) const override;
};
