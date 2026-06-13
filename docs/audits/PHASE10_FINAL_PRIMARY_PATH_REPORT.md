# Phase 10 Final Primary Path Report

## Primary Path Status

| Runtime | Primary path | Source enum | Status |
| --- | --- | --- | --- |
| LinuxSim | `LinuxSimRuntimeEntry -> LinuxSimRuntimeEntryAdoptionProbe -> AsciiRuntimeEntryAdoption` | `LinuxSimRuntimeSource::ScreenGraphAdoption` | primary screen graph adoption; Unavailable on failed adoption |
| GTK | `LinuxUConsoleGtkPageRegistryAdoption -> GtkRuntimeEntryAdoption` | `LinuxUConsoleGtkPageRegistrySource::ScreenGraphAdoption` | primary page registry descriptors; Unavailable on failed adoption |
| LVGL | `LvglPrimaryScreenGraphRuntime -> LvglRuntimeEntryAdoption` | `LvglScreenGraphRuntimeSource::ScreenGraphAdoption` | primary descriptor runtime |

## Fallback Status

| Runtime | Fallback source | Status |
| --- | --- | --- |
| LinuxSim | deleted | deleted after LinuxSim/uConsole fallback burn-down |
| GTK | deleted | deleted after LinuxSim/uConsole fallback burn-down |
| LVGL | `LvglScreenGraphRuntimeSource::HardcodedFallback` | fallback-only descriptor fallback |

LinuxSim/uConsole failed adoption remains testable through source enum checks
and renderer false-return assertions. LVGL fallback remains testable through
`fallbackUsed()` until the LVGL renderer burn-down lands.

## Not Done

Phase 10 intentionally does not complete these migrations:

- real GTK widget rewrite
- real LVGL widget/menu rewrite
- LVGL fallback deletion
- full navigation stack replacement
- complete screen/page migration

## Guardrail

Runtime primary paths must not call `findUxPackById`, `UxPackRegistry`, or
`buildMenuForUxPack`. UX selection and `PresentationBundle` construction stay
in app-shell/probe/composition code. Runtime primary paths consume the
descriptor graph only.

LVGL primary descriptor runtime must not include `lvgl.h`, create `lv_obj_t`,
or branch on `BOARD_`.

## Phase 11 Recommendation

Phase 11 should be Real Renderer Descriptor Consumption. The first target
should stay on LinuxSim / ASCII because it proves renderer behavior with the
lowest device and widget-lifecycle risk. GTK and LVGL should follow only after
the ASCII renderer consumes descriptors as its concrete route source.
