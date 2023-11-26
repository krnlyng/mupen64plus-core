// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "Common/CommonTypes.h"

namespace Common::JitRegister
{
void Init(const std::string& perf_dir);
void Shutdown();
void Register(const void* base_address, const void* end_address, const std::string& symbol_name);
bool IsEnabled();
}  // namespace Common::JitRegister
