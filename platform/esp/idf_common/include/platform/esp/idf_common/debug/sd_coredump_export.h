#pragma once

#include <cstddef>

namespace platform::esp::idf_common::debug
{

struct SdCoredumpExportStatus
{
    bool supported = false;
    bool sd_ready = false;
    bool found = false;
    bool exported = false;
    bool erased = false;
    std::size_t size = 0;
    char path[160] = {};
};

SdCoredumpExportStatus sd_coredump_export_status();
bool export_previous_coredump_to_sd();

} // namespace platform::esp::idf_common::debug
