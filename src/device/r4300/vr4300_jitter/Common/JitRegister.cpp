// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/JitRegister.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "Common/CommonTypes.h"
#include "Common/StringUtil.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#if defined USE_OPROFILE && USE_OPROFILE
#include <opagent.h>
#endif

#if defined USE_VTUNE
#include <jitprofiling.h>
#pragma comment(lib, "jitprofiling.lib")
#endif

#if defined USE_OPROFILE && USE_OPROFILE
static op_agent_t s_agent = nullptr;
#endif

static std::ofstream s_perf_map_file;

namespace Common::JitRegister
{
static bool s_is_enabled = false;

void Init(const std::string& perf_dir)
{
#if defined USE_OPROFILE && USE_OPROFILE
  s_agent = op_open_agent();
  s_is_enabled = true;
#endif

  if (!perf_dir.empty() || getenv("PERF_BUILDID_DIR"))
  {
    const std::string dir = perf_dir.empty() ? "/tmp" : perf_dir;
    const std::string filename = dir + "/perf-" + std::to_string(getpid()) + ".map";
    s_perf_map_file.open(filename, std::ofstream::out);
    s_is_enabled = true;
  }
}

void Shutdown()
{
#if defined USE_OPROFILE && USE_OPROFILE
  op_close_agent(s_agent);
  s_agent = nullptr;
#endif

#ifdef USE_VTUNE
  iJIT_NotifyEvent(iJVM_EVENT_TYPE_SHUTDOWN, nullptr);
#endif

  if (s_perf_map_file.is_open())
    s_perf_map_file.close();

  s_is_enabled = false;
}

bool IsEnabled()
{
  return s_is_enabled;
}

void Register(const void* base_address, const void* end_address, const std::string& symbol_name)
{
  u32 code_size = (u64)end_address - (u64)base_address;
#if !(defined USE_OPROFILE && USE_OPROFILE) && !defined(USE_VTUNE)
  if (!s_perf_map_file.is_open())
    return;
#endif

#if defined USE_OPROFILE && USE_OPROFILE
  op_write_native_code(s_agent, symbol_name.c_str(), (u64)base_address, base_address, code_size);
#endif

#ifdef USE_VTUNE
  iJIT_Method_Load jmethod = {0};
  jmethod.method_id = iJIT_GetNewMethodID();
  jmethod.method_load_address = const_cast<void*>(base_address);
  jmethod.method_size = code_size;
  jmethod.method_name = const_cast<char*>(symbol_name.c_str());
  iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, (void*)&jmethod);
#endif

  // Linux perf /tmp/perf-$pid.map:
  if (!s_perf_map_file.is_open())
    return;

  s_perf_map_file << std::hex << std::setfill('0') << std::setw(sizeof(void*) * 2) << (u64)base_address << " " << std::setw(0) << code_size << " " << symbol_name << std::endl;
  s_perf_map_file.flush();
}
}  // namespace Common::JitRegister
