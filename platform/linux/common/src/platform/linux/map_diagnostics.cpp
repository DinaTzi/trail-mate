#include "platform/linux/map_diagnostics.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>

#include "platform/linux/runtime_paths.h"

namespace platform::linux_runtime
{
namespace
{

std::mutex s_log_mutex;

std::string lower_copy(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool disables_doh(const std::string& value)
{
    const std::string lower = lower_copy(value);
    return lower == "0" || lower == "false" || lower == "off" ||
           lower == "none" || lower == "disabled";
}

std::string optional_env_value(const char* specific_name,
                               const char* shared_name)
{
    if (const char* specific = std::getenv(specific_name))
    {
        return specific;
    }
    if (const char* shared = std::getenv(shared_name))
    {
        return shared;
    }
    return {};
}

std::string timestamp_utc()
{
    using clock = std::chrono::system_clock;
    const std::time_t now = clock::to_time_t(clock::now());
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

} // namespace

std::filesystem::path map_diagnostic_log_path()
{
    return resolve_paths().settings_root / "logs" / "map.log";
}

void append_map_diagnostic(std::string_view category,
                           std::string_view message)
{
    const auto path = map_diagnostic_log_path();
    if (!ensure_directory(path.parent_path()))
    {
        return;
    }

    std::lock_guard<std::mutex> lock(s_log_mutex);
    std::ofstream out(path, std::ios::app);
    if (!out.is_open())
    {
        return;
    }
    out << timestamp_utc() << " [" << category << "] " << message << '\n';
}

std::string map_curl_doh_url()
{
    const std::string value =
        optional_env_value("TRAIL_MATE_MAP_DOH_URL",
                           "TRAIL_MATE_CURL_DOH_URL");
    return disables_doh(value) ? std::string() : value;
}

std::string map_curl_ip_resolve_mode()
{
    const std::string value =
        optional_env_value("TRAIL_MATE_MAP_IP_RESOLVE",
                           "TRAIL_MATE_CURL_IP_RESOLVE");
    const std::string lower = lower_copy(value);
    if (lower == "4" || lower == "v4" || lower == "ipv4")
    {
        return "ipv4";
    }
    if (lower == "6" || lower == "v6" || lower == "ipv6")
    {
        return "ipv6";
    }
    return {};
}

void apply_map_curl_resolver(CURL* curl)
{
    if (curl == nullptr)
    {
        return;
    }
    const std::string doh = map_curl_doh_url();
    if (!doh.empty())
    {
#if LIBCURL_VERSION_NUM >= 0x073E00
        curl_easy_setopt(curl, CURLOPT_DOH_URL, doh.c_str());
#else
        (void)doh;
#endif
    }

    const std::string ip_mode = map_curl_ip_resolve_mode();
    if (ip_mode == "ipv4")
    {
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    }
    else if (ip_mode == "ipv6")
    {
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
    }
}

std::string curl_error_message(CURLcode code, const char* error_buffer)
{
    if (error_buffer != nullptr && error_buffer[0] != '\0')
    {
        return error_buffer;
    }
    return curl_easy_strerror(code);
}

} // namespace platform::linux_runtime
