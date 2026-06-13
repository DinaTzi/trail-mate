# ESP32 LVGL Target Profiles

Phase 8.3 target profile baseline plus the closed T-Display-P4 touch UX route.

```text
Build Entrypoint invokes.
App Shell composes.
Target chooses.
Board describes.
UX Pack presents.
```

| Target | Board | Platform | Build entrypoint | App shell | UX profile | Status |
| --- | --- | --- | --- | --- | --- | --- |
| `tab5` | `tab5` | ESP32-P4 + ESP32-C6 | `builds/esp_idf` | `apps/esp32_lvgl` | `tab5_touch` | active with fallback |
| `tdisplayp4_tft` | `tdisplayp4` | ESP32-P4 + ESP32-C6 | `builds/esp_idf` | `apps/esp32_lvgl` | `tdisplayp4_touch` | active |
| `tdisplayp4_amoled` | `tdisplayp4` | ESP32-P4 + ESP32-C6 | `builds/esp_idf` | `apps/esp32_lvgl` | `tdisplayp4_touch` | active |
| `tdeck` | `tdeck` | ESP32-S3 | `builds/esp_idf` | `apps/esp32_lvgl` | `deck_full` | planned |
| `tdeck_pro` | `tdeck_pro` | ESP32-S3 | `builds/esp_idf` | `apps/esp32_lvgl` | `deck_full` | planned |
| `tlora_pager` | `tlora_pager` | ESP32 family | `builds/esp_idf` | `apps/esp32_lvgl` | `pager_compact` | planned |
| `twatchs3` | `twatchs3` | ESP32-S3 | `builds/esp_idf` | `apps/esp32_lvgl` | `watch_quick` | planned |

## Transitional Source

Current transitional path:

- `legacy/app_implementations/esp_idf`

Current target descriptors under `legacy/app_implementations/esp_idf/targets/*` remain in place until
wrapper and app shell migration are proven.

## Non-Goals

This document does not move sdkconfig defaults, ESP-IDF CMake files, app
runtime code, or LVGL screen implementations.

## C6 and Orientation Policy

`tab5`, `tdisplayp4_tft`, and `tdisplayp4_amoled` select the ESP32-C6 wireless
companion as their BLE backend. P4 remains the Trail Mate business authority,
and C6 is the BLE / ESP-NOW / Wi-Fi facade.

All three ESP32-P4 targets are motion-sensor aware but landscape locked in the
current release. Motion sensor presence is a board fact; portrait UI support is
not enabled by this target profile.
