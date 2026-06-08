#pragma once

#include <cstddef>
#include <cstdint>

namespace platform::esp::arduino_common::debug
{

struct SdDebugLogStatus
{
    bool debug_logs_enabled = false;
    bool sd_ready = false;
    bool log_open = false;
    bool coredump_supported = false;
    bool coredump_found = false;
    bool coredump_exported = false;
    bool coredump_erased = false;
    std::size_t coredump_size = 0;
    char debug_log_path[96] = {};
    char coredump_path[128] = {};
};

bool debug_logs_enabled();
bool begin_sd_debug_log();
void append_line(const char* line);
void printf(const char* format, ...);
void flush();
SdDebugLogStatus status();

bool export_previous_coredump_to_sd();

} // namespace platform::esp::arduino_common::debug
