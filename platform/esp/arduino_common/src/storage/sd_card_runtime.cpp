#include "platform/esp/arduino_common/storage/sd_card_runtime.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#ifndef DISABLE_FS_H_WARNING
#define DISABLE_FS_H_WARNING 1
#endif
#include <SdFat.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>

namespace platform::esp::arduino_common::storage
{
namespace
{

constexpr uint8_t kRuntimeCardNone = 0;
constexpr uint32_t kSdSectorSize = 512;

SdFs s_sdfat;
SdCardInfo s_info{};
bool s_sdfat_mounted = false;

uint8_t card_type_from_sdfat(SdFs& fs)
{
    SdCard* card = fs.card();
    if (card == nullptr)
    {
        return kRuntimeCardNone;
    }

    const uint8_t type = card->type();
    if (type == SD_CARD_TYPE_SD1)
    {
        return CARD_SD;
    }
    if (type == SD_CARD_TYPE_SD2)
    {
        return CARD_SD;
    }
    if (type == SD_CARD_TYPE_SDHC)
    {
        return CARD_SDHC;
    }
    return CARD_UNKNOWN;
}

bool path_empty(const char* path)
{
    return path == nullptr || path[0] == '\0';
}

const char* normalize_sd_path(const char* path)
{
    if (path_empty(path))
    {
        return "/";
    }

    if ((path[0] == 'A' || path[0] == 'a') && path[1] == ':')
    {
        path += 2;
    }

    if (path[0] == '\0')
    {
        return "/";
    }
    return path;
}

oflag_t sdfat_open_flags(const char* mode)
{
    if (mode == nullptr || std::strcmp(mode, "r") == 0 || std::strcmp(mode, "rb") == 0)
    {
        return O_RDONLY;
    }
    if (std::strcmp(mode, "w") == 0 || std::strcmp(mode, "wb") == 0)
    {
        return O_WRONLY | O_CREAT | O_TRUNC;
    }
    if (std::strcmp(mode, "a") == 0 || std::strcmp(mode, "ab") == 0)
    {
        return O_WRONLY | O_CREAT | O_APPEND;
    }
    if (std::strcmp(mode, "r+") == 0 || std::strcmp(mode, "rb+") == 0 ||
        std::strcmp(mode, "r+b") == 0)
    {
        return O_RDWR;
    }
    if (std::strcmp(mode, "w+") == 0 || std::strcmp(mode, "wb+") == 0 ||
        std::strcmp(mode, "w+b") == 0)
    {
        return O_RDWR | O_CREAT | O_TRUNC;
    }
    if (std::strcmp(mode, "a+") == 0 || std::strcmp(mode, "ab+") == 0 ||
        std::strcmp(mode, "a+b") == 0)
    {
        return O_RDWR | O_CREAT | O_APPEND;
    }
    return O_RDONLY;
}

void clear_sdfat()
{
    if (s_sdfat_mounted)
    {
        s_sdfat.end();
        s_sdfat_mounted = false;
    }
}

void reset_info()
{
    s_info = SdCardInfo{};
}

void record_arduino_info()
{
    s_info = SdCardInfo{};
    s_info.backend = SdCardBackend::ArduinoSd;
    s_info.card_type = SD.cardType();
    s_info.fat_type = 0;
    s_info.sector_size = SD.sectorSize();
    s_info.sector_count = SD.numSectors();
    s_info.card_size_bytes = SD.cardSize();
    s_info.total_bytes = SD.totalBytes();
    s_info.used_bytes = SD.usedBytes();
}

void record_sdfat_info()
{
    s_info = SdCardInfo{};
    s_info.backend = SdCardBackend::SdFat;
    s_info.card_type = card_type_from_sdfat(s_sdfat);
    s_info.fat_type = s_sdfat.fatType();
    s_info.sector_size = kSdSectorSize;
    SdCard* card = s_sdfat.card();
    if (card != nullptr)
    {
        s_info.sector_count = static_cast<uint32_t>(card->sectorCount());
        s_info.card_size_bytes = static_cast<uint64_t>(s_info.sector_count) * kSdSectorSize;
    }
    const uint64_t cluster_count = s_sdfat.clusterCount();
    const uint64_t bytes_per_cluster = s_sdfat.bytesPerCluster();
    if (cluster_count > 0 && bytes_per_cluster > 0)
    {
        s_info.total_bytes = cluster_count * bytes_per_cluster;
    }
    const int32_t free_clusters = s_sdfat.freeClusterCount();
    if (free_clusters >= 0 && bytes_per_cluster > 0)
    {
        const uint64_t free_bytes = static_cast<uint64_t>(free_clusters) * bytes_per_cluster;
        s_info.used_bytes = s_info.total_bytes > free_bytes ? s_info.total_bytes - free_bytes : 0;
    }
}

} // namespace

bool mount_sd_card(int sd_cs,
                   SPIClass& spi,
                   uint32_t spi_hz,
                   const char* mount_point,
                   uint8_t max_files)
{
    clear_sdfat();
    SD.end();
    reset_info();

    const bool arduino_ok = SD.begin(sd_cs, spi, spi_hz, mount_point, max_files, false);
    Serial.printf("[SD] Arduino SD.begin -> %d\n", arduino_ok ? 1 : 0);
    if (arduino_ok && SD.cardType() != CARD_NONE && SD.sectorSize() != 0)
    {
        record_arduino_info();
        Serial.printf("[SD] backend=arduino fs=fat card=%llu MB total=%llu MB sectors=%lu sector_size=%lu\n",
                      static_cast<unsigned long long>(s_info.card_size_bytes / (1024ULL * 1024ULL)),
                      static_cast<unsigned long long>(s_info.total_bytes / (1024ULL * 1024ULL)),
                      static_cast<unsigned long>(s_info.sector_count),
                      static_cast<unsigned long>(s_info.sector_size));
        return true;
    }

    SD.end();
    delay(10);

    const bool sdfat_ok = s_sdfat.begin(SdSpiConfig(sd_cs, SHARED_SPI, spi_hz, &spi));
    Serial.printf("[SD] SdFat.begin -> %d\n", sdfat_ok ? 1 : 0);
    if (!sdfat_ok || s_sdfat.fatType() == 0)
    {
        s_sdfat.initErrorPrint(&Serial);
        clear_sdfat();
        reset_info();
        return false;
    }

    s_sdfat_mounted = true;
    record_sdfat_info();
    Serial.printf("[SD] backend=sdfat fs=%s card=%llu MB total=%llu MB sectors=%lu sector_size=%lu\n",
                  sd_card_filesystem_name(),
                  static_cast<unsigned long long>(s_info.card_size_bytes / (1024ULL * 1024ULL)),
                  static_cast<unsigned long long>(s_info.total_bytes / (1024ULL * 1024ULL)),
                  static_cast<unsigned long>(s_info.sector_count),
                  static_cast<unsigned long>(s_info.sector_size));
    return true;
}

void unmount_sd_card()
{
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        SD.end();
    }
    clear_sdfat();
    reset_info();
}

bool sd_card_ready()
{
    return s_info.backend != SdCardBackend::None && s_info.sector_size != 0 &&
           s_info.card_type != kRuntimeCardNone;
}

bool sd_card_uses_arduino_sd()
{
    return s_info.backend == SdCardBackend::ArduinoSd;
}

bool sd_card_uses_sdfat()
{
    return s_info.backend == SdCardBackend::SdFat;
}

bool sd_card_is_exfat()
{
    return s_info.backend == SdCardBackend::SdFat && s_info.fat_type == FAT_TYPE_EXFAT;
}

SdCardBackend sd_card_backend()
{
    return s_info.backend;
}

SdCardInfo sd_card_info()
{
    return s_info;
}

const char* sd_card_backend_name()
{
    switch (s_info.backend)
    {
    case SdCardBackend::ArduinoSd:
        return "arduino";
    case SdCardBackend::SdFat:
        return "sdfat";
    case SdCardBackend::None:
    default:
        return "none";
    }
}

const char* sd_card_filesystem_name()
{
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return "fat";
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        switch (s_info.fat_type)
        {
        case FAT_TYPE_EXFAT:
            return "exfat";
        case FAT_TYPE_FAT32:
            return "fat32";
        case FAT_TYPE_FAT16:
            return "fat16";
        case FAT_TYPE_FAT12:
            return "fat12";
        default:
            return "fat";
        }
    }
    return "none";
}

bool sd_exists(const char* path)
{
    const char* normalized = normalize_sd_path(path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return SD.exists(normalized);
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        return s_sdfat.exists(normalized);
    }
    return false;
}

bool sd_is_directory(const char* path)
{
    const char* normalized = normalize_sd_path(path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        File dir = SD.open(normalized, FILE_READ);
        const bool result = dir && dir.isDirectory();
        dir.close();
        return result;
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        FsFile dir = s_sdfat.open(normalized, O_RDONLY);
        const bool result = dir && dir.isDir();
        dir.close();
        return result;
    }
    return false;
}

bool sd_mkdir(const char* path)
{
    const char* normalized = normalize_sd_path(path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return SD.mkdir(normalized);
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        return s_sdfat.mkdir(normalized, true);
    }
    return false;
}

bool sd_remove(const char* path)
{
    const char* normalized = normalize_sd_path(path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return SD.remove(normalized);
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        return s_sdfat.remove(normalized);
    }
    return false;
}

bool sd_rename(const char* old_path, const char* new_path)
{
    const char* normalized_old = normalize_sd_path(old_path);
    const char* normalized_new = normalize_sd_path(new_path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return SD.rename(normalized_old, normalized_new);
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        return s_sdfat.rename(normalized_old, normalized_new);
    }
    return false;
}

class SdRuntimeFile::Impl
{
  public:
    File arduino_file;
    FsFile sdfat_file;
    SdCardBackend backend = SdCardBackend::None;
};

SdRuntimeFile::SdRuntimeFile()
    : impl_(new (std::nothrow) Impl())
{
}

SdRuntimeFile::~SdRuntimeFile()
{
    close();
    delete impl_;
}

bool SdRuntimeFile::open(const char* path, const char* mode)
{
    close();
    if (impl_ == nullptr || path_empty(path))
    {
        return false;
    }

    const char* normalized = normalize_sd_path(path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        impl_->arduino_file = SD.open(normalized, mode ? mode : FILE_READ);
        impl_->backend = impl_->arduino_file ? SdCardBackend::ArduinoSd : SdCardBackend::None;
        return impl_->backend == SdCardBackend::ArduinoSd;
    }

    if (s_info.backend == SdCardBackend::SdFat)
    {
        impl_->sdfat_file = s_sdfat.open(normalized, sdfat_open_flags(mode));
        impl_->backend = impl_->sdfat_file ? SdCardBackend::SdFat : SdCardBackend::None;
        return impl_->backend == SdCardBackend::SdFat;
    }

    return false;
}

void SdRuntimeFile::close()
{
    if (impl_ == nullptr)
    {
        return;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        impl_->arduino_file.close();
    }
    else if (impl_->backend == SdCardBackend::SdFat)
    {
        impl_->sdfat_file.close();
    }
    impl_->backend = SdCardBackend::None;
}

bool SdRuntimeFile::is_open() const
{
    return impl_ != nullptr && impl_->backend != SdCardBackend::None;
}

int SdRuntimeFile::read(void* buffer, std::size_t bytes_to_read)
{
    if (!is_open() || buffer == nullptr || bytes_to_read == 0)
    {
        return 0;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        return impl_->arduino_file.read(static_cast<uint8_t*>(buffer), bytes_to_read);
    }
    if (impl_->backend == SdCardBackend::SdFat)
    {
        return impl_->sdfat_file.read(buffer, bytes_to_read);
    }
    return -1;
}

std::size_t SdRuntimeFile::write(const void* buffer, std::size_t bytes_to_write)
{
    if (!is_open() || buffer == nullptr || bytes_to_write == 0)
    {
        return 0;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        return impl_->arduino_file.write(static_cast<const uint8_t*>(buffer), bytes_to_write);
    }
    if (impl_->backend == SdCardBackend::SdFat)
    {
        return impl_->sdfat_file.write(buffer, bytes_to_write);
    }
    return 0;
}

bool SdRuntimeFile::seek(uint64_t offset)
{
    if (!is_open())
    {
        return false;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        return impl_->arduino_file.seek(offset);
    }
    if (impl_->backend == SdCardBackend::SdFat)
    {
        return impl_->sdfat_file.seekSet(offset);
    }
    return false;
}

uint64_t SdRuntimeFile::position() const
{
    if (!is_open())
    {
        return 0;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        return impl_->arduino_file.position();
    }
    if (impl_->backend == SdCardBackend::SdFat)
    {
        return impl_->sdfat_file.curPosition();
    }
    return 0;
}

uint64_t SdRuntimeFile::size() const
{
    if (!is_open())
    {
        return 0;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        return impl_->arduino_file.size();
    }
    if (impl_->backend == SdCardBackend::SdFat)
    {
        return impl_->sdfat_file.fileSize();
    }
    return 0;
}

bool SdRuntimeFile::flush()
{
    if (!is_open())
    {
        return false;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        impl_->arduino_file.flush();
        return true;
    }
    if (impl_->backend == SdCardBackend::SdFat)
    {
        return impl_->sdfat_file.sync();
    }
    return false;
}

class SdRuntimeDir::Impl
{
  public:
    File arduino_dir;
    FsFile sdfat_dir;
    SdCardBackend backend = SdCardBackend::None;
};

SdRuntimeDir::SdRuntimeDir()
    : impl_(new (std::nothrow) Impl())
{
}

SdRuntimeDir::~SdRuntimeDir()
{
    close();
    delete impl_;
}

bool SdRuntimeDir::open(const char* path)
{
    close();
    if (impl_ == nullptr)
    {
        return false;
    }
    const char* normalized = normalize_sd_path(path);
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        impl_->arduino_dir = SD.open(normalized, FILE_READ);
        impl_->backend = (impl_->arduino_dir && impl_->arduino_dir.isDirectory())
                             ? SdCardBackend::ArduinoSd
                             : SdCardBackend::None;
        return impl_->backend == SdCardBackend::ArduinoSd;
    }
    if (s_info.backend == SdCardBackend::SdFat)
    {
        impl_->sdfat_dir = s_sdfat.open(normalized, O_RDONLY);
        impl_->backend =
            (impl_->sdfat_dir && impl_->sdfat_dir.isDir()) ? SdCardBackend::SdFat
                                                           : SdCardBackend::None;
        return impl_->backend == SdCardBackend::SdFat;
    }
    return false;
}

void SdRuntimeDir::close()
{
    if (impl_ == nullptr)
    {
        return;
    }
    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        impl_->arduino_dir.close();
    }
    else if (impl_->backend == SdCardBackend::SdFat)
    {
        impl_->sdfat_dir.close();
    }
    impl_->backend = SdCardBackend::None;
}

bool SdRuntimeDir::is_open() const
{
    return impl_ != nullptr && impl_->backend != SdCardBackend::None;
}

bool SdRuntimeDir::read_next(char* name, std::size_t name_size, bool* is_dir)
{
    if (!is_open() || name == nullptr || name_size == 0)
    {
        return false;
    }
    name[0] = '\0';
    if (is_dir != nullptr)
    {
        *is_dir = false;
    }

    if (impl_->backend == SdCardBackend::ArduinoSd)
    {
        File entry = impl_->arduino_dir.openNextFile();
        if (!entry)
        {
            return false;
        }
        const char* raw_name = entry.name();
        std::snprintf(name, name_size, "%s", raw_name ? raw_name : "");
        if (is_dir != nullptr)
        {
            *is_dir = entry.isDirectory();
        }
        entry.close();
        return name[0] != '\0';
    }

    if (impl_->backend == SdCardBackend::SdFat)
    {
        FsFile entry = impl_->sdfat_dir.openNextFile(O_RDONLY);
        if (!entry)
        {
            return false;
        }
        entry.getName(name, name_size);
        if (is_dir != nullptr)
        {
            *is_dir = entry.isDir();
        }
        entry.close();
        return name[0] != '\0';
    }

    return false;
}

bool sd_read_raw(uint32_t lba, uint8_t* buffer)
{
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return SD.readRAW(buffer, lba);
    }
    if (s_info.backend == SdCardBackend::SdFat && s_sdfat.card() != nullptr)
    {
        return s_sdfat.card()->readSector(lba, buffer);
    }
    return false;
}

bool sd_write_raw(uint32_t lba, const uint8_t* buffer)
{
    if (s_info.backend == SdCardBackend::ArduinoSd)
    {
        return SD.writeRAW(const_cast<uint8_t*>(buffer), lba);
    }
    if (s_info.backend == SdCardBackend::SdFat && s_sdfat.card() != nullptr)
    {
        return s_sdfat.card()->writeSector(lba, buffer);
    }
    return false;
}

} // namespace platform::esp::arduino_common::storage
