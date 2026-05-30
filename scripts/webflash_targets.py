from __future__ import annotations

from typing import Final

WEBFLASH_TARGETS: Final = (
    {
        "id": "tlora-pager-sx1262",
        "env": "tlora_pager_sx1262",
        "name": "LilyGo T-LoRa Pager SX1262",
        "subtitle": "SX1262 build for the 480x222 keyboard pager",
        "chip_family": "ESP32-S3",
        "flash_mode": "dio",
        "flash_freq": "80m",
        "flash_size": "16MB",
        "merged_asset_name": "trail-mate-tlora-pager-sx1262-webflash.bin",
    },
    {
        "id": "tlora-pager-lr1121",
        "env": "tlora_pager_lr1121",
        "name": "LilyGo T-LoRa Pager LR1121",
        "subtitle": "LR1121 build for the Sub-GHz + 2.4 GHz keyboard pager",
        "chip_family": "ESP32-S3",
        "flash_mode": "dio",
        "flash_freq": "80m",
        "flash_size": "16MB",
        "merged_asset_name": "trail-mate-tlora-pager-lr1121-webflash.bin",
    },
    {
        "id": "tdeck",
        "env": "tdeck",
        "name": "LilyGo T-Deck",
        "subtitle": "ESP32-S3 handheld build with keyboard and touch",
        "chip_family": "ESP32-S3",
        "flash_mode": "dio",
        "flash_freq": "80m",
        "flash_size": "16MB",
        "merged_asset_name": "trail-mate-tdeck-webflash.bin",
    },
    {
        "id": "lilygo-twatch-s3",
        "env": "lilygo_twatch_s3",
        "name": "LilyGo T-Watch-S3",
        "subtitle": "Touch-first ESP32-S3 watch build",
        "chip_family": "ESP32-S3",
        "flash_mode": "dio",
        "flash_freq": "80m",
        "flash_size": "16MB",
        "merged_asset_name": "trail-mate-lilygo-twatch-s3-webflash.bin",
    },
)

TARGETS_BY_ENV: Final = {target["env"]: target for target in WEBFLASH_TARGETS}
TARGETS_BY_ID: Final = {target["id"]: target for target in WEBFLASH_TARGETS}
