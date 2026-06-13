# Phase 11 Renderer Descriptor Consumption Report

## Scope

Phase 11 moves primary descriptors into renderer-facing consumption surfaces.
The LinuxSim/uConsole burn-down follow-up deletes their renderer fallback
branches. It still does not rewrite GTK widgets, rewrite LVGL menu/page
renderers, implement a full navigation stack, add UX packs, or let renderers
choose UX packs.

The Phase 11 distinction is:

- Phase 10 made descriptor/adoption paths primary.
- Phase 11 makes renderer-facing surfaces consume those primary descriptors.

## LinuxSim

renderer descriptor path:

`LinuxSimRuntimeEntry -> LinuxSimRuntimeRenderer -> AsciiDescriptorRenderer -> AsciiRuntimeEntryAdoption`

descriptor source:

`AsciiRuntimeEntryAdoption -> AsciiRuntimeScreenGraphPresenter -> AsciiMenuLine / AsciiScreenDescriptor`

Status:

- `AsciiDescriptorRenderer` consumes `AsciiRuntimeEntryAdoption` descriptors.
- `LinuxSimRuntimeRenderer` consumes `LinuxSimRuntimeEntry`.
- When `LinuxSimRuntimeEntry::usingPrimaryScreenGraph()` is true, the renderer
  renders from `entry.adoption()`.

LinuxSim fallback burned down:

- `LinuxSimRuntimeRenderer` no longer exposes `renderFallback`,
  `fallbackUsed`, or `usedFallback`.
- failed adoption returns false and leaves the renderer not ready.

fallback status:

deleted after LinuxSim/uConsole fallback burn-down

Not done:

- hardcoded ASCII renderer deletion
- full navigation stack replacement

## GTK

Page registry descriptor path:

`LinuxUConsoleGtkPageRegistryAdoption -> LinuxUConsoleGtkPageRegistryRenderer -> GtkDescriptorPageRegistry -> GtkRuntimeEntryAdoption`

descriptor source:

`GtkRuntimeEntryAdoption -> GtkUConsoleScreenGraphPresenter -> GtkMenuDescriptor / GtkScreenDescriptor`

Status:

- `GtkDescriptorPageRegistry` consumes `GtkRuntimeEntryAdoption` descriptors
  and produces `GtkDescriptorPage` rows.
- `LinuxUConsoleGtkPageRegistryRenderer` consumes
  `LinuxUConsoleGtkPageRegistryAdoption`.
- When `LinuxUConsoleGtkPageRegistryAdoption::usingPrimaryScreenGraph()` is
  true, the renderer consumes descriptor pages.

GTK fallback burned down:

- `LinuxUConsoleGtkPageRegistryRenderer` no longer exposes `renderFallback`,
  `fallbackUsed`, or `usedFallback`.
- failed adoption returns false and leaves the page registry not ready.

fallback status:

deleted after LinuxSim/uConsole fallback burn-down

Not done:

- GTK widget tree rewrite
- full page switching replacement

## LVGL

Descriptor renderer path:

`LvglPrimaryScreenGraphRuntime -> LvglDescriptorRendererProbe -> LvglDescriptorMenuModel -> LvglRuntimeEntryAdoption`

descriptor source:

`LvglPrimaryScreenGraphRuntime -> LvglRuntimeEntryAdoption -> LvglRuntimeScreenGraphPresenter -> LvglMenuEntry / LvglScreenHostEntry`

Status:

- `LvglDescriptorMenuModel` consumes `LvglPrimaryScreenGraphRuntime`.
- `LvglDescriptorRendererProbe` consumes `LvglPrimaryScreenGraphRuntime` and
  exposes renderer-safe `LvglDescriptorMenuItem` rows.
- The path does not include `lvgl.h`, does not create `lv_obj_t`, and does not
  branch on `BOARD_`.

Fallback path:

- `LvglDescriptorRendererProbe::loadFallback(...)` is reached only when
  `runtime.fallbackUsed()` is true.
- hardcoded LVGL menu/page creation remains fallback-only.

LVGL fallback status:

fallback-only

Not done:

- real LVGL widget/menu rewrite
- device-specific renderer migration
- LVGL fallback deletion

## Guardrails

Renderer consumption paths must not call:

- `UxPackRegistry`
- `findUxPackById`
- `buildMenuForUxPack`

Renderer consumption paths must not create:

- `GtkWidget`
- `lv_obj_t`

Renderer consumption paths must not construct `MenuModel` or select UX packs.
UX selection and `PresentationBundle` construction remain upstream.

## Explicit Non-goals

Phase 11 does not rewrite real GTK widgets.
Phase 11 does not create LVGL widgets.
LinuxSim and GTK renderer fallbacks are deleted. LVGL fallback remains
contained until a real LVGL target consumes descriptor menu/page data before
widget creation.

## Phase 12 Recommendation

Phase 12 should focus on fallback deletion readiness and architecture freeze:

- list which fallback branches are now provably unused by default
- decide which alias/fallback surfaces can be deleted safely
- freeze the directory and checker rules that prevent app shell, renderer, and
  legacy implementation concerns from drifting back together
