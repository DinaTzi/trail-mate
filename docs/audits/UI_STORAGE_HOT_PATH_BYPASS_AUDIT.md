# UI Storage Hot-Path Bypass Audit

Status date: 2026-06-16

This audit is the separate map / SD / UI burn-down cut requested after the ESP
UI freeze regressions. It lists active UI hot-path entries that can touch SD,
LVGL FS, or display SPI, then records whether each entry is already behind a
runtime/worker boundary or still needs deletion/consolidation.

The rule from `UI_STORAGE_EVENT_RUNTIME_DESIGN_SPEC.md` is tightened here:

```text
UI owner context may build value objects, update renderer objects, and submit
runtime intents. It must not synchronously open/read/write/list storage, call
LVGL FS for SD-backed files, or wait on a display-shared SPI resource.
```

## Entry Inventory

| Entry | Resource touched | Current risk | Decision |
| --- | --- | --- | --- |
| `platform/esp/arduino_common/src/ui/widgets/map/map_tiles.cpp` UI tile source | LVGL FS path was present in ESP map source | High confusion risk: ESP UI source appeared able to read through LVGL FS | Burned in this cut. ESP UI source is path-only; SD reads remain in the worker `SdMapTileFileSystem`. |
| `platform/esp/arduino_common/src/ui/widgets/map/map_tiles.cpp` worker tile source | SD through `SdRuntimeFile` | Accepted only in worker domain | Keep. This is the map storage adapter behind the tile async runtime. |
| `platform/esp/arduino_common/src/ui/widgets/map/map_tiles.cpp` tile decode | LVGL image decoder, CPU/memory work | High when executed from UI event drain | Burned from the UI hot path in this cut. The ESP map worker now publishes already-decoded native image descriptors after releasing the SD bus. |
| `modules/ui_shared/src/ui/i18n/resource_pack_registry.cpp::locale_preview_font` | External font load via `lv_binfont_create` / LVGL FS | High. Locale/settings UI preview could synchronously load SD-backed font packs | Burned in this cut for ESP. Preview uses only already-loaded fonts on ESP. |
| `modules/ui_shared/src/ui/i18n/resource_pack_registry.cpp::activate_locale_internal` | External UI font load | High on ESP if locale UI font is SD-backed | Burned from the ESP UI hot path in this cut. ESP defers unloaded external UI fonts and falls back to the builtin latin UI font chain. |
| `modules/ui_shared/src/ui/i18n/resource_pack_registry.cpp` external pack scan | LVGL FS dir/file reads | Medium. Usually registry/startup path, but still synchronous | Remaining legacy. Needs pack catalog worker or startup preflight outside visible UI frame budget. |
| `modules/ui_shared/src/ui/screens/gps/gps_page_runtime.cpp` route loaders | `ui::fs::read_text_file` | Medium to high when user opens/loads a route | Remaining legacy. Move GPX/CSV parse into route/track runtime worker and return overlay points by event. |
| `platform/esp/arduino_common/src/ui/screens/team/team_ui_store.cpp` | SD exists/open/write/remove/rename | High. A UI screen store owns durable persistence | Remaining legacy. Move team snapshot/key/event/chat/position persistence behind `TEAM_ACTION_RUNTIME_SPEC` storage worker. |
| `platform/esp/arduino_common/src/ui/runtime/pack_repository.cpp` | Flash/LVGL FS open/dir/read/write | Medium. Pack management can block UI actions | Remaining legacy. Keep as explicit pack runtime adapter only after UI callers submit commands; remove direct page calls. |
| `modules/ui_shared/src/ui/widgets/busy_overlay.cpp` | `lv_refr_now` display flush | Medium if called while storage owns display-shared SPI | Burned in this cut. Busy overlay now invalidates and lets the normal UI tick render. |
| startup shells / boot log | `lv_refr_now` / manual `lv_timer_handler` display flush | Medium. Startup could bypass normal UI tick and contend with early SD/display setup | Burned in this cut. Startup paths now invalidate and let the normal UI tick render. |
| `platform/esp/arduino_common/src/LV_Helper_v9.cpp` | LVGL FS adapter to SD/flash | Adapter risk, not business risk | Keep as platform adapter. UI hot paths must not use it for SD-backed files. |
| `platform/esp/boards/src/display/DisplayInterface.cpp` | display SPI lock/flush | Frame-critical adapter | Keep. Other workers must yield to this domain through bounded wait, hold, and cooldown policy. |

## Burned In This Cut

### ESP map UI source no longer owns a storage path

The ESP map file now has two explicit source roles:

- UI path source: path-only, no `exists`, `isDirectory`, or `readFile` behavior.
- Worker source: SD-backed `SdMapTileFileSystem`, used by the async tile runtime.

This removes the misleading ESP `LvglMapTileFileSystem` implementation from the
active map source file. Path resolution remains in `FilesystemMapTileSource`;
file reads stay in the worker adapter.

### Locale preview cannot load external fonts on ESP

`locale_preview_font()` no longer calls `ensure_font_pack_loaded()` on ESP.
Preview UI can compose with an external font only if it is already loaded.
Otherwise it falls back to the base font and returns immediately.

This prevents settings/list rendering from repeatedly entering
`lv_binfont_create()` and LVGL FS just because Chinese text appeared in a preview.

### Locale activation cannot synchronously load external UI fonts on ESP

`activate_locale_internal()` no longer calls external `lv_binfont_create()` from
ESP activation/startup paths. If the selected locale points at an unloaded
external UI font pack, the locale remains active but the UI font chain falls back
to `builtin-latin-ui` and logs the external font as deferred.

This prioritizes UI liveness over synchronous Chinese UI font availability. The
proper recovery path is an i18n font-load runtime command that loads the
external font outside the UI owner context and publishes a font-chain update
event.

### Busy overlay no longer forces immediate display refresh

`busy_overlay::show/update/set_progress/hide` no longer call `lv_refr_now()`.
They invalidate the overlay or active screen and let the normal UI tick perform
rendering.

This keeps slow-operation feedback from directly competing with SD or other
display-shared SPI work in the same call stack.

### Startup paths no longer force display refresh

Boot log updates and startup shell boot presentation no longer call
`lv_refr_now()` or manually drive `lv_timer_handler()`. They invalidate the
affected layer/object and return.

This removes startup display refresh re-entry from the same period where SD,
radio, font catalog, and display initialization can still be contending for
shared resources.

### Map tile decode moved out of UI event drain

The ESP map active path previously read tile bytes in the worker but decoded the
PNG/JPG payload in `apply_map_tile_event()` on the UI owner context.

The ESP event sink now decodes the compressed payload before enqueueing the
event. The UI owner receives an already-decoded native image descriptor and only
adopts it into the decoded cache / LVGL image object.

This removes the active UI call to `lv_image_decoder_open()`. The remaining
technical debt is that the worker-side decoder still uses LVGL's decoder API as
a platform decoder adapter. That is no longer a UI hot-path bypass, but a later
decoder-adapter cut should replace it with a decoder that is explicitly safe to
run outside the UI owner context.

## Remaining Burn-Down Order

1. Move i18n deferred external font activation into an event-driven font-load
   runtime. Preserve Chinese UI by loading packs asynchronously and applying the
   new font chain on a UI-owner event.
2. Move GPS GPX/CSV route loading into a route runtime worker. UI submits a path
   and receives parsed/downsampled overlay points.
3. Move team UI persistence out of `platform/esp/.../ui/screens` into a storage
   worker owned by the team action runtime.
4. Convert pack repository actions into commands/events. Keep LVGL FS and flash
   APIs only in platform adapters.
5. Replace worker-side LVGL image decoding with a platform decoder adapter that
   has no dependency on LVGL global decoder state.

## Verification Gate For Each Follow-Up Cut

Each follow-up must pass these checks before commit:

- GitNexus impact analysis for every edited symbol.
- `rg` audit showing the edited UI hot path no longer calls SD, LVGL FS, or
  display SPI primitives directly.
- A compile check for one ESP target.
- `gitnexus detect-changes` to confirm the affected surface matches the planned
  runtime/worker slice.
