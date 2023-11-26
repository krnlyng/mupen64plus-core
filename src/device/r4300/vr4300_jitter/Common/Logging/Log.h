// Copyright 2009 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <string_view>

#include "api/callbacks.h"

namespace Common::Log
{
enum class LogType : int
{
  MEMMAP,
  COMMON,
  DYNA_REC,

  NUMBER_OF_LOGS  // Must be last
};

constexpr LogType LAST_LOG_TYPE =
    static_cast<LogType>(static_cast<int>(LogType::NUMBER_OF_LOGS) - 1);

enum class LogLevel : int
{
  LNOTICE = 1,   // VERY important information that is NOT errors. Like startup and OSReports.
  LERROR = 2,    // Critical errors
  LWARNING = 3,  // Something is suspicious.
  LINFO = 4,     // General information.
  LDEBUG = 5,    // Detailed debugging - might make things slow.
};

#if defined(_DEBUG) || defined(DEBUGFAST)
constexpr auto MAX_LOGLEVEL = Common::Log::LogLevel::LDEBUG;
#else
constexpr auto MAX_LOGLEVEL = Common::Log::LogLevel::LINFO;
#endif  // logging

static const char LOG_LEVEL_TO_CHAR[7] = "-NEWID";

#define GENERIC_LOG_FMT(t, v, format, ...)                                                         \
  do                                                                                               \
  {                                                                                                \
    if (v <= Common::Log::MAX_LOGLEVEL)                                                            \
    {                                                                                              \
      switch(v) {                                                                                  \
        case Common::Log::LogLevel::LNOTICE:                                                       \
          DebugMessage(M64MSG_ERROR, format __VA_OPT__(, ) __VA_ARGS__);                           \
          break;                                                                                   \
        case Common::Log::LogLevel::LERROR:                                                        \
          DebugMessage(M64MSG_ERROR, format __VA_OPT__(, ) __VA_ARGS__);                           \
          break;                                                                                   \
        case Common::Log::LogLevel::LWARNING:                                                      \
          DebugMessage(M64MSG_WARNING, format __VA_OPT__(, ) __VA_ARGS__);                         \
          break;                                                                                   \
        case Common::Log::LogLevel::LINFO:                                                         \
          DebugMessage(M64MSG_INFO, format __VA_OPT__(, ) __VA_ARGS__);                            \
          break;                                                                                   \
        case Common::Log::LogLevel::LDEBUG:                                                        \
          DebugMessage(M64MSG_VERBOSE, format __VA_OPT__(, ) __VA_ARGS__);                         \
          break;                                                                                   \
      };                                                                                           \
    }                                                                                              \
  } while (0)

#define ERROR_LOG_FMT(t, ...)                                                                      \
  do                                                                                               \
  {                                                                                                \
    GENERIC_LOG_FMT(Common::Log::LogType::t,                                                       \
                    Common::Log::LogLevel::LERROR __VA_OPT__(, ) __VA_ARGS__);                     \
  } while (0)
#define WARN_LOG_FMT(t, ...)                                                                       \
  do                                                                                               \
  {                                                                                                \
    GENERIC_LOG_FMT(Common::Log::LogType::t,                                                       \
                    Common::Log::LogLevel::LWARNING __VA_OPT__(, ) __VA_ARGS__);                   \
  } while (0)
#define NOTICE_LOG_FMT(t, ...)                                                                     \
  do                                                                                               \
  {                                                                                                \
    GENERIC_LOG_FMT(Common::Log::LogType::t,                                                       \
                    Common::Log::LogLevel::LNOTICE __VA_OPT__(, ) __VA_ARGS__);                    \
  } while (0)
#define INFO_LOG_FMT(t, ...)                                                                       \
  do                                                                                               \
  {                                                                                                \
    GENERIC_LOG_FMT(Common::Log::LogType::t,                                                       \
                    Common::Log::LogLevel::LINFO __VA_OPT__(, ) __VA_ARGS__);                      \
  } while (0)
#define DEBUG_LOG_FMT(t, ...)                                                                      \
  do                                                                                               \
  {                                                                                                \
    GENERIC_LOG_FMT(Common::Log::LogType::t,                                                       \
                    Common::Log::LogLevel::LDEBUG __VA_OPT__(, ) __VA_ARGS__);                     \
  } while (0)
}  // namespace Common::Log

