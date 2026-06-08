#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <curl/curl.h>

namespace platform::linux_runtime
{

std::filesystem::path map_diagnostic_log_path();
void append_map_diagnostic(std::string_view category,
                           std::string_view message);

std::string map_curl_doh_url();
std::string map_curl_ip_resolve_mode();
void apply_map_curl_resolver(CURL* curl);
std::string curl_error_message(CURLcode code, const char* error_buffer);

} // namespace platform::linux_runtime
