# ESP32 SD Debug Logs And Coredumps

Issue #54 separates two diagnostics paths that are easy to confuse. This is
ESP32 shared diagnostics, not a Pager-only behavior:

- Runtime debug logs are normal boot/runtime notes mirrored to SD when the
  Settings > Advanced > Debug Logs switch is enabled.
- ESP coredumps are crash artifacts. They are written by the ESP coredump
  implementation to the flash `coredump` partition during panic, then exported
  by Trail Mate to SD on the next normal boot.

This contract applies to SD-capable ESP32 product paths such as Pager, T-Deck,
Tab5, and T-Display-P4. Boards without a ready SD card keep the flash coredump
copy for a later boot instead of dropping it.

Trail Mate does not write FAT/exFAT files from the panic handler. SD card,
SPI, allocation, and filesystem calls are not safe in that context.

## Build Preconditions

PlatformIO Arduino ESP32 builds use `board_build.partitions = partitions.csv`.
ESP-IDF builds use the same custom partition file through each target's
`sdkconfig.defaults`. That partition table must keep the final flash coredump
partition:

```text
coredump, data, coredump, 0xFF0000, 0x10000,
```

The active ESP32 build configuration must provide flash ELF coredumps:

```text
CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y
CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=y
```

For Arduino PlatformIO builds, do not treat the repository-root `sdkconfig`
file as the source of truth. The active framework SDK configuration is the
one that matters.

## SD Layout

When an SD card is mounted, Trail Mate uses these paths:

| Path | Owner | Meaning |
| --- | --- | --- |
| `/trailmate/debug/debug.log` | Debug Logs switch | Current rolling runtime debug log |
| `/trailmate/debug/debug.prev.log` | Debug Logs switch | Previous runtime debug log after rotation |
| `/trailmate/coredumps/core-*.elf` | Startup coredump export | Binary ESP coredump copied from flash |
| `/trailmate/coredumps/core-*.elf.txt` | Startup coredump export | Metadata for the exported coredump |

`debug.log` rotates at 256 KiB. Coredumps are not rotated by the runtime; they
are field evidence and should be copied off the card before long test sessions.

## Export Rules

On startup, after the board has mounted SD, Trail Mate checks the ESP flash
coredump partition:

1. If no coredump exists, startup continues normally.
2. If a valid coredump exists and SD is ready, Trail Mate writes the binary
   image to `/trailmate/coredumps/core-*.elf`.
3. Trail Mate writes a sidecar `.txt` metadata file with size, flash address,
   reset reason, and available ELF summary fields.
4. Only after a successful SD write does Trail Mate erase the coredump image
   from flash.
5. If SD is absent or export fails, the flash copy is kept for the next boot.

The Debug Logs setting controls the live `debug.log` mirror. Coredump export is
attempted whenever SD is available because the crash artifact is the primary
field-debug record promised for issue #54.

## Ownership

Arduino ESP32 exports live under `platform/esp/arduino_common/debug`. ESP-IDF
exports live under `platform/esp/idf_common/debug`. Both are platform services
used by the ESP32 app shell at startup; neither path belongs to Cardputer Zero,
Linux package generation, or any legacy compatibility layer.
