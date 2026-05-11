#include "platform/linux/map_contour_tile_generator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

#include <curl/curl.h>

#include "platform/linux/map_diagnostics.h"
#include "platform/linux/runtime_paths.h"

namespace platform::linux_runtime
{
namespace
{

constexpr int kTileSize = 256;
constexpr double kPi = 3.14159265358979323846;
constexpr long kCmrTimeoutSeconds = 30;
constexpr long kDemDownloadTimeoutSeconds = 300;
constexpr const char* kUserAgent = "TrailMate-uConsole/0.1";
constexpr const char* kCmrGranulesUrl =
    "https://cmr.earthdata.nasa.gov/search/granules.json";

struct TileBounds
{
    double west = 0.0;
    double south = 0.0;
    double east = 0.0;
    double north = 0.0;
};

struct HttpResult
{
    bool ok = false;
    long status = 0;
    std::string body{};
    std::string error{};
};

std::filesystem::path dem_root()
{
    return resolve_paths().sd_root / "maps" / "dem";
}

std::filesystem::path contour_work_root()
{
    return resolve_paths().cache_root / "contour-work";
}

std::string trim_copy(std::string value)
{
    const auto not_space = [](unsigned char ch)
    {
        return std::isspace(ch) == 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
                value.end());
    return value;
}

std::string fmt_double(double value)
{
    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    return std::string(buffer);
}

std::string shell_quote(std::string_view value)
{
    if (value.empty())
    {
        return "''";
    }

    std::string out = "'";
    for (const char ch : value)
    {
        if (ch == '\'')
        {
            out += "'\\''";
        }
        else
        {
            out.push_back(ch);
        }
    }
    out.push_back('\'');
    return out;
}

std::string path_quote(const std::filesystem::path& path)
{
    return shell_quote(path.string());
}

int run_command(const std::string& command)
{
    const int rc = std::system(command.c_str());
    if (rc == -1)
    {
        append_map_diagnostic("contour",
                              "command failed to start: " + command);
        return -1;
    }
#if !defined(_WIN32)
    if (WIFEXITED(rc))
    {
        const int exit_code = WEXITSTATUS(rc);
        if (exit_code != 0)
        {
            append_map_diagnostic(
                "contour",
                "command exit " + std::to_string(exit_code) + ": " +
                    command);
        }
        return exit_code;
    }
#endif
    if (rc != 0)
    {
        append_map_diagnostic("contour",
                              "command rc " + std::to_string(rc) + ": " +
                                  command);
    }
    return rc;
}

bool command_available(const char* name)
{
    std::string command = "command -v ";
    command += shell_quote(name);
    command += " >/dev/null 2>&1";
    return run_command(command) == 0;
}

TileBounds tile_to_bounds(int x, int y, int zoom)
{
    const double n = std::pow(2.0, static_cast<double>(zoom));
    TileBounds out{};
    out.west = static_cast<double>(x) / n * 360.0 - 180.0;
    out.east = static_cast<double>(x + 1) / n * 360.0 - 180.0;
    out.north =
        std::atan(std::sinh(kPi * (1.0 - 2.0 * y / n))) * 180.0 /
        kPi;
    out.south =
        std::atan(std::sinh(kPi * (1.0 - 2.0 * (y + 1) / n))) *
        180.0 / kPi;
    return out;
}

std::size_t write_string_callback(char* ptr,
                                  std::size_t size,
                                  std::size_t nmemb,
                                  void* userdata)
{
    auto* out = static_cast<std::string*>(userdata);
    const std::size_t bytes = size * nmemb;
    if (out == nullptr)
    {
        return 0;
    }
    out->append(ptr, bytes);
    return bytes;
}

struct FileDownloadContext
{
    std::ofstream stream;
};

std::size_t write_file_callback(char* ptr,
                                std::size_t size,
                                std::size_t nmemb,
                                void* userdata)
{
    auto* ctx = static_cast<FileDownloadContext*>(userdata);
    const std::size_t bytes = size * nmemb;
    if (ctx == nullptr || !ctx->stream.is_open())
    {
        return 0;
    }
    ctx->stream.write(ptr, static_cast<std::streamsize>(bytes));
    return ctx->stream.good() ? bytes : 0;
}

std::uintmax_t file_size_or_zero(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    return ec ? 0U : size;
}

std::string curl_escape(const std::string& value)
{
    CURL* curl = curl_easy_init();
    if (curl == nullptr)
    {
        return value;
    }
    char* escaped = curl_easy_escape(curl,
                                     value.c_str(),
                                     static_cast<int>(value.size()));
    std::string out = escaped != nullptr ? escaped : value;
    if (escaped != nullptr)
    {
        curl_free(escaped);
    }
    curl_easy_cleanup(curl);
    return out;
}

HttpResult fetch_string(const std::string& url)
{
    HttpResult out{};
    CURL* curl = curl_easy_init();
    if (curl == nullptr)
    {
        out.error = "Cannot initialize libcurl.";
        return out;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    char error_buffer[CURL_ERROR_SIZE] = {};
    apply_map_curl_resolver(curl);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, kCmrTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out.body);

    const CURLcode rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out.status);
    if (headers != nullptr)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK)
    {
        out.error = curl_error_message(rc, error_buffer);
        append_map_diagnostic("contour",
                              "CMR GET failed: " + out.error + " / " +
                                  url);
        return out;
    }
    if (out.status < 200 || out.status >= 300)
    {
        out.error = "HTTP " + std::to_string(out.status);
        append_map_diagnostic("contour",
                              "CMR GET returned " + out.error + " / " +
                                  url);
        return out;
    }
    out.ok = true;
    append_map_diagnostic("contour",
                          "CMR GET ok HTTP " + std::to_string(out.status) +
                              " / " + std::to_string(out.body.size()) +
                              " bytes / " + url);
    return out;
}

HttpResult download_file(const std::string& url,
                         const std::filesystem::path& path,
                         const std::string& bearer_token)
{
    HttpResult out{};
    if (!ensure_directory(path.parent_path()))
    {
        out.error = "Cannot create DEM cache directory.";
        return out;
    }

    const auto temp_path = path.string() + ".tmp";
    FileDownloadContext ctx{};
    ctx.stream.open(temp_path, std::ios::binary | std::ios::trunc);
    if (!ctx.stream.is_open())
    {
        out.error = "Cannot create DEM temp file.";
        return out;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr)
    {
        ctx.stream.close();
        std::error_code ec;
        std::filesystem::remove(temp_path, ec);
        out.error = "Cannot initialize libcurl.";
        return out;
    }

    struct curl_slist* headers = nullptr;
    if (!bearer_token.empty())
    {
        const std::string auth = "Authorization: Bearer " + bearer_token;
        headers = curl_slist_append(headers, auth.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    char error_buffer[CURL_ERROR_SIZE] = {};
    apply_map_curl_resolver(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, kDemDownloadTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    const CURLcode rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out.status);
    if (headers != nullptr)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    ctx.stream.close();

    if (rc != CURLE_OK)
    {
        std::error_code ec;
        std::filesystem::remove(temp_path, ec);
        out.error = curl_error_message(rc, error_buffer);
        append_map_diagnostic("contour",
                              "DEM download failed: " + out.error + " / " +
                                  url);
        return out;
    }
    if (out.status < 200 || out.status >= 300)
    {
        std::error_code ec;
        std::filesystem::remove(temp_path, ec);
        out.error = "HTTP " + std::to_string(out.status);
        append_map_diagnostic("contour",
                              "DEM download returned " + out.error + " / " +
                                  url);
        return out;
    }

    std::error_code ec;
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(temp_path, path, ec);
    if (ec)
    {
        std::filesystem::remove(temp_path, ec);
        out.error = "Cannot commit DEM file.";
        append_map_diagnostic("contour",
                              out.error + " / " + path.string());
        return out;
    }

    out.ok = true;
    append_map_diagnostic("contour",
                          "DEM cached HTTP " + std::to_string(out.status) +
                              " / " + std::to_string(file_size_or_zero(path)) +
                              " bytes / " + path.string());
    return out;
}

bool file_nonempty(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) &&
           std::filesystem::file_size(path, ec) > 0U && !ec;
}

std::string json_unescape_url(std::string value)
{
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] != '\\' || i + 1 >= value.size())
        {
            out.push_back(value[i]);
            continue;
        }

        const char escaped = value[++i];
        switch (escaped)
        {
        case '/':
        case '\\':
        case '"':
            out.push_back(escaped);
            break;
        case 'n':
            out.push_back('\n');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case 't':
            out.push_back('\t');
            break;
        default:
            out.push_back(escaped);
            break;
        }
    }
    return out;
}

bool ends_with_ci(std::string_view text, std::string_view suffix)
{
    if (text.size() < suffix.size())
    {
        return false;
    }
    const auto start = text.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i)
    {
        const auto lhs =
            static_cast<unsigned char>(text[start + i]);
        const auto rhs = static_cast<unsigned char>(suffix[i]);
        if (std::tolower(lhs) != std::tolower(rhs))
        {
            return false;
        }
    }
    return true;
}

bool contains_ci(std::string_view text, std::string_view needle)
{
    auto it = std::search(text.begin(),
                          text.end(),
                          needle.begin(),
                          needle.end(),
                          [](char lhs, char rhs)
                          {
                              return std::tolower(
                                         static_cast<unsigned char>(lhs)) ==
                                     std::tolower(
                                         static_cast<unsigned char>(rhs));
                          });
    return it != text.end();
}

bool likely_dem_url(const std::string& url)
{
    return (ends_with_ci(url, ".zip") || ends_with_ci(url, ".hgt") ||
            ends_with_ci(url, ".tif")) &&
           (contains_ci(url, "lpdaac") || contains_ci(url, "e4ftl") ||
            contains_ci(url, "data.lpdaac"));
}

int score_dem_url(const std::string& url)
{
    if ((contains_ci(url, "e4ftl") || contains_ci(url, "lpdaac")) &&
        !contains_ci(url, "earthdatacloud"))
    {
        return 0;
    }
    if (contains_ci(url, "earthdatacloud"))
    {
        return 2;
    }
    return 1;
}

std::vector<std::string> parse_dem_urls(const std::string& json)
{
    static const std::regex kHrefPattern(
        R"TM("href"\s*:\s*"((?:[^"\\]|\\.)*)")TM",
        std::regex_constants::icase);
    std::vector<std::string> urls;
    std::set<std::string> seen;

    for (auto it = std::sregex_iterator(json.begin(), json.end(), kHrefPattern);
         it != std::sregex_iterator();
         ++it)
    {
        std::string url = json_unescape_url((*it)[1].str());
        if (!contains_ci(url, "https://") || !likely_dem_url(url))
        {
            continue;
        }
        if (seen.insert(url).second)
        {
            urls.push_back(std::move(url));
        }
    }

    std::sort(urls.begin(),
              urls.end(),
              [](const std::string& lhs, const std::string& rhs)
              {
                  const int lhs_score = score_dem_url(lhs);
                  const int rhs_score = score_dem_url(rhs);
                  if (lhs_score != rhs_score)
                  {
                      return lhs_score < rhs_score;
                  }
                  return lhs < rhs;
              });
    return urls;
}

std::vector<std::string> find_dem_urls(const TileBounds& bounds,
                                       std::string& error)
{
    const std::string bbox = fmt_double(bounds.west) + "," +
                             fmt_double(bounds.south) + "," +
                             fmt_double(bounds.east) + "," +
                             fmt_double(bounds.north);
    const std::string url =
        std::string(kCmrGranulesUrl) +
        "?short_name=NASADEM_HGT&version=001&bounding_box=" +
        curl_escape(bbox) + "&page_size=2000";
    append_map_diagnostic("contour", "CMR search bbox " + bbox);
    const HttpResult response = fetch_string(url);
    if (!response.ok)
    {
        error = "CMR search failed: " + response.error;
        return {};
    }
    auto urls = parse_dem_urls(response.body);
    if (urls.empty())
    {
        error = "No NASADEM granules in current tile.";
    }
    append_map_diagnostic("contour",
                          "CMR search found " + std::to_string(urls.size()) +
                              " DEM URL(s) for bbox " + bbox);
    return urls;
}

std::string file_name_from_url(std::string url)
{
    const auto hash = url.find('#');
    if (hash != std::string::npos)
    {
        url.erase(hash);
    }
    const auto query = url.find('?');
    if (query != std::string::npos)
    {
        url.erase(query);
    }
    const auto slash = url.find_last_of('/');
    return slash == std::string::npos ? url : url.substr(slash + 1);
}

std::vector<std::filesystem::path> find_files_with_extensions(
    const std::filesystem::path& root,
    const std::vector<std::string>& extensions)
{
    std::vector<std::filesystem::path> out;
    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
    {
        return out;
    }

    for (std::filesystem::recursive_directory_iterator it(root, ec), end;
         it != end && !ec;
         it.increment(ec))
    {
        if (!it->is_regular_file(ec))
        {
            continue;
        }
        const std::string ext = it->path().extension().string();
        if (std::find_if(extensions.begin(),
                         extensions.end(),
                         [&](const std::string& allowed)
                         {
                             return ends_with_ci(ext, allowed);
                         }) != extensions.end())
        {
            out.push_back(it->path());
        }
    }
    return out;
}

std::vector<std::filesystem::path> resolve_dem_files(
    const std::filesystem::path& path,
    std::string& error)
{
    if (ends_with_ci(path.string(), ".hgt") || ends_with_ci(path.string(), ".tif"))
    {
        return {path};
    }
    if (!ends_with_ci(path.string(), ".zip"))
    {
        return {};
    }
    if (!command_available("unzip"))
    {
        error = "unzip not found. Install unzip to extract NASADEM archives.";
        return {};
    }

    const auto extracted_dir =
        dem_root() / "extracted" / path.stem().string();
    if (!ensure_directory(extracted_dir))
    {
        error = "Cannot create DEM extract directory.";
        return {};
    }

    auto files = find_files_with_extensions(extracted_dir, {".hgt", ".tif"});
    if (!files.empty())
    {
        return files;
    }

    const std::string command = "unzip -o -q " + path_quote(path) + " -d " +
                                path_quote(extracted_dir);
    if (run_command(command) != 0)
    {
        error = "Failed to extract NASADEM archive.";
        append_map_diagnostic("contour",
                              error + " / " + path.string());
        return {};
    }
    files = find_files_with_extensions(extracted_dir, {".hgt", ".tif"});
    if (files.empty())
    {
        error = "NASADEM archive did not contain HGT/TIF files.";
        append_map_diagnostic("contour",
                              error + " / " + path.string());
    }
    return files;
}

std::vector<std::filesystem::path> ensure_dem_files(const TileBounds& bounds,
                                                    const std::string& token,
                                                    std::string& error)
{
    if (!ensure_directory(dem_root()))
    {
        error = "Cannot create DEM cache directory.";
        return {};
    }

    auto urls = find_dem_urls(bounds, error);
    if (urls.empty())
    {
        return {};
    }

    std::vector<std::filesystem::path> files;
    std::set<std::string> seen;
    for (const auto& url : urls)
    {
        const std::string file_name = file_name_from_url(url);
        if (file_name.empty())
        {
            continue;
        }

        const std::filesystem::path local_path = dem_root() / file_name;
        if (!file_nonempty(local_path))
        {
            append_map_diagnostic("contour",
                                  "DEM selected " + file_name + " / " + url);
            HttpResult download = download_file(url, local_path, token);
            if (!download.ok && download.status == 400 && !token.empty())
            {
                append_map_diagnostic(
                    "contour",
                    "DEM bearer download returned HTTP 400; retrying without token / " +
                        url);
                download = download_file(url, local_path, "");
            }
            if (!download.ok)
            {
                error = "DEM download failed: " + download.error;
                return {};
            }
        }

        std::string resolve_error{};
        auto local_files = resolve_dem_files(local_path, resolve_error);
        if (!resolve_error.empty())
        {
            error = resolve_error;
            return {};
        }
        for (const auto& file : local_files)
        {
            const std::string key = file.string();
            if (seen.insert(key).second)
            {
                files.push_back(file);
            }
        }
    }

    if (files.empty() && error.empty())
    {
        error = "No usable DEM files for current tile.";
    }
    return files;
}

std::pair<int, std::string> resolve_contour_source(
    const MapContourProfile& profile,
    const std::vector<MapContourProfile>& profiles)
{
    if (profile.kind != MapContourKind::Major)
    {
        return {profile.interval_m, ""};
    }

    int minor_interval = 0;
    for (const auto& candidate : profiles)
    {
        if (candidate.kind == MapContourKind::Minor &&
            candidate.interval_m < profile.interval_m &&
            profile.interval_m % candidate.interval_m == 0)
        {
            minor_interval = minor_interval == 0
                                 ? candidate.interval_m
                                 : std::min(minor_interval,
                                            candidate.interval_m);
        }
    }

    if (minor_interval <= 0)
    {
        return {profile.interval_m, ""};
    }
    return {minor_interval,
            "CAST(elev AS INTEGER) % " + std::to_string(profile.interval_m) +
                " = 0"};
}

std::string profile_label(const MapContourProfile& profile)
{
    return map_contour_profile_key(profile);
}

std::filesystem::path tile_work_dir(int z, int x, int y)
{
    return contour_work_root() /
           (std::to_string(z) + "_" + std::to_string(x) + "_" +
            std::to_string(y));
}

bool ensure_gdal_available(std::string& error)
{
    static const std::array<const char*, 4> kTools = {
        "gdalbuildvrt",
        "gdal_contour",
        "gdal_rasterize",
        "gdal_translate",
    };
    if (!command_available("gdalinfo"))
    {
        error = "GDAL not found. Install gdal-bin.";
        return false;
    }
    for (const auto* tool : kTools)
    {
        if (!command_available(tool))
        {
            error = std::string(tool) + " not found. Install gdal-bin.";
            return false;
        }
    }
    return true;
}

bool build_vrt(const TileBounds& bounds,
               const std::filesystem::path& vrt_path,
               const std::vector<std::filesystem::path>& dem_files,
               std::string& error)
{
    std::ostringstream command;
    command << "gdalbuildvrt -q -overwrite -te " << fmt_double(bounds.west)
            << ' ' << fmt_double(bounds.south) << ' '
            << fmt_double(bounds.east) << ' ' << fmt_double(bounds.north)
            << ' ' << path_quote(vrt_path);
    for (const auto& file : dem_files)
    {
        command << ' ' << path_quote(file);
    }

    if (run_command(command.str()) != 0)
    {
        error = "gdalbuildvrt failed.";
        return false;
    }
    return true;
}

bool generate_profile_png(const MapContourProfile& profile,
                          const std::vector<MapContourProfile>& profiles,
                          const std::filesystem::path& work_dir,
                          const std::filesystem::path& vrt_path,
                          const TileBounds& bounds,
                          const std::filesystem::path& output_path,
                          std::map<int, std::filesystem::path>& contour_paths,
                          std::string& error)
{
    const auto [contour_interval, where_clause] =
        resolve_contour_source(profile, profiles);
    auto contour_it = contour_paths.find(contour_interval);
    if (contour_it == contour_paths.end())
    {
        const auto contour_path =
            work_dir / ("contours_i" + std::to_string(contour_interval) +
                        ".geojson");
        if (!file_nonempty(contour_path))
        {
            const std::string command =
                "gdal_contour -q -i " + std::to_string(contour_interval) +
                " -a elev -f GeoJSON " + path_quote(vrt_path) + " " +
                path_quote(contour_path);
            if (run_command(command) != 0)
            {
                error = "gdal_contour failed for " +
                        std::to_string(contour_interval) + " m.";
                return false;
            }
        }
        contour_it = contour_paths.emplace(contour_interval, contour_path).first;
    }

    if (!ensure_directory(output_path.parent_path()))
    {
        error = "Cannot create contour output directory.";
        return false;
    }

    const auto raster_path =
        work_dir / ("contours_" + profile_label(profile) + ".tif");
    const auto [r, g, b, a] =
        profile.kind == MapContourKind::Major
            ? std::array<int, 4>{214, 193, 145, 220}
            : std::array<int, 4>{167, 149, 108, 190};

    std::ostringstream rasterize;
    rasterize << "gdal_rasterize -q ";
    if (!where_clause.empty())
    {
        rasterize << "-where " << shell_quote(where_clause) << ' ';
    }
    rasterize << "-burn " << r << " -burn " << g << " -burn " << b
              << " -burn " << a
              << " -init 0 0 0 0 -ts " << kTileSize << ' ' << kTileSize
              << " -te " << fmt_double(bounds.west) << ' '
              << fmt_double(bounds.south) << ' ' << fmt_double(bounds.east)
              << ' ' << fmt_double(bounds.north)
              << " -a_nodata 0 -ot Byte -of GTiff "
              << path_quote(contour_it->second) << ' '
              << path_quote(raster_path);
    if (run_command(rasterize.str()) != 0)
    {
        error = "gdal_rasterize failed for " + profile_label(profile) + ".";
        return false;
    }

    const std::string translate =
        "gdal_translate -q -of PNG -co ZLEVEL=1 " + path_quote(raster_path) +
        " " + path_quote(output_path);
    if (run_command(translate) != 0)
    {
        error = "gdal_translate failed for " + profile_label(profile) + ".";
        return false;
    }
    return true;
}

struct TileBatch
{
    int z = 0;
    int x = 0;
    int y = 0;
    std::vector<MapContourProfile> profiles{};
};

bool same_profile(const MapContourProfile& lhs, const MapContourProfile& rhs)
{
    return lhs.kind == rhs.kind && lhs.interval_m == rhs.interval_m;
}

std::vector<TileBatch> group_tiles(const std::vector<MapContourTileId>& tiles)
{
    std::map<std::tuple<int, int, int>, std::vector<MapContourProfile>> grouped;
    for (auto tile : tiles)
    {
        normalize_map_contour_tile(tile);
        auto& profiles = grouped[{tile.z, tile.x, tile.y}];
        if (std::find_if(profiles.begin(),
                         profiles.end(),
                         [&](const MapContourProfile& existing)
                         {
                             return same_profile(existing, tile.profile);
                         }) == profiles.end())
        {
            profiles.push_back(tile.profile);
        }
    }

    std::vector<TileBatch> out;
    out.reserve(grouped.size());
    for (auto& [key, profiles] : grouped)
    {
        std::sort(profiles.begin(),
                  profiles.end(),
                  [](const MapContourProfile& lhs,
                     const MapContourProfile& rhs)
                  {
                      if (lhs.interval_m != rhs.interval_m)
                      {
                          return lhs.interval_m < rhs.interval_m;
                      }
                      return static_cast<int>(lhs.kind) <
                             static_cast<int>(rhs.kind);
                  });
        auto [z, x, y] = key;
        out.push_back(TileBatch{
            .z = z,
            .x = x,
            .y = y,
            .profiles = std::move(profiles),
        });
    }
    return out;
}

} // namespace

MapContourTileGenerator::MapContourTileGenerator() = default;

MapContourGenerationResult MapContourTileGenerator::ensure_tiles(
    const std::vector<MapContourTileId>& tiles,
    const std::string& earthdata_token) const
{
    MapContourGenerationResult result{};
    result.requested_tiles = tiles.size();
    if (tiles.empty())
    {
        result.message = "No visible contour tiles to fill.";
        return result;
    }

    const std::string token = trim_copy(earthdata_token);
    if (token.empty())
    {
        result.failed_tiles = tiles.size();
        result.message = "Earthdata token missing.";
        return result;
    }

    std::string dependency_error{};
    if (!ensure_gdal_available(dependency_error))
    {
        result.failed_tiles = tiles.size();
        result.message = dependency_error;
        return result;
    }

    std::string last_error{};
    for (const auto& batch : group_tiles(tiles))
    {
        const auto bounds = tile_to_bounds(batch.x, batch.y, batch.z);
        const auto work_dir = tile_work_dir(batch.z, batch.x, batch.y);
        const auto vrt_path = work_dir / "dem.vrt";
        if (!ensure_directory(work_dir))
        {
            last_error = "Cannot create contour work directory.";
            result.failed_tiles += batch.profiles.size();
            continue;
        }

        std::vector<MapContourProfile> missing_profiles;
        missing_profiles.reserve(batch.profiles.size());
        for (const auto& profile : batch.profiles)
        {
            MapContourTileId id{.profile = profile,
                                .z = batch.z,
                                .x = batch.x,
                                .y = batch.y};
            if (store_.tile_available(id))
            {
                ++result.cached_tiles;
            }
            else
            {
                missing_profiles.push_back(profile);
            }
        }
        if (missing_profiles.empty())
        {
            continue;
        }

        std::string dem_error{};
        const auto dem_files = ensure_dem_files(bounds, token, dem_error);
        if (dem_files.empty())
        {
            last_error = dem_error.empty() ? "No DEM files available."
                                           : dem_error;
            result.failed_tiles += missing_profiles.size();
            continue;
        }

        if (!file_nonempty(vrt_path) &&
            !build_vrt(bounds, vrt_path, dem_files, last_error))
        {
            result.failed_tiles += missing_profiles.size();
            continue;
        }

        std::map<int, std::filesystem::path> contour_paths;
        for (const auto& profile : missing_profiles)
        {
            MapContourTileId id{.profile = profile,
                                .z = batch.z,
                                .x = batch.x,
                                .y = batch.y};
            const auto output_path = store_.tile_path(id);
            std::string profile_error{};
            if (generate_profile_png(profile,
                                     missing_profiles,
                                     work_dir,
                                     vrt_path,
                                     bounds,
                                     output_path,
                                     contour_paths,
                                     profile_error))
            {
                ++result.generated_tiles;
            }
            else
            {
                last_error = profile_error;
                ++result.failed_tiles;
            }
        }
    }

    std::ostringstream message;
    message << "Contour fill: generated " << result.generated_tiles
            << ", cached " << result.cached_tiles << ", failed "
            << result.failed_tiles << ".";
    if (!last_error.empty())
    {
        message << ' ' << last_error;
    }
    result.message = message.str();
    return result;
}

} // namespace platform::linux_runtime
