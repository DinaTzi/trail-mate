# Phase 9 Fallback Containment Ledger

Phase 9.3 keeps hardcoded routing alive as contained fallback while real
runtime entries begin consuming the presentation graph. Fallback presence is
acceptable only when the current owner, new path, exit condition, and checker
status are explicit.

| Fallback | Current owner | Why fallback remains | New path | Exit condition | Checker status |
| --- | --- | --- | --- | --- | --- |
| LinuxSim hardcoded runtime routing | final `apps/linux_sim_shell` runtime entry plus historical simulator runtime | the simulator renderer still has old routing available for failed adoption | `LinuxSimRuntimeEntry -> LinuxSimRuntimeEntryAdoptionProbe -> AsciiRuntimeEntryAdoption` | simulator renderer no longer needs hardcoded routing | fallback-only after Phase 10.1 |
| GTK hardcoded page registry | final `apps/linux_uconsole_gtk` page registry adoption plus historical GTK page registry | GTK page switching still owns existing widget creation and page list behavior | `LinuxUConsoleGtkPageRegistryAdoption -> GtkRuntimeEntryAdoption -> GtkUConsoleScreenGraphPresenter` | GTK page registry consumes descriptors as primary page source | fallback-only after Phase 10.2 |
| LVGL hardcoded menu/page creation | `modules/ui_lvgl_ux_packs` compatibility runtime | device-specific LVGL renderers still own menu and screen creation | `LvglPrimaryScreenGraphRuntime -> LvglRuntimeEntryAdoption -> LvglRuntimeScreenGraphPresenter` | LVGL renderers consume descriptor runtime before creating menu/page objects | fallback-only after Phase 10.3 |
| Chat LegacyDelivery bridges | alias build include surface removed | runtime modules own all active includes | `ChatDeliveryActionPortAdapter` and `ChatDeliveryEventProjectionAdapter` in `modules/ui_chat_runtime` | keep runtime headers as the only build-visible API | retired from build include surface |
| KeyVerification legacy source/sink | alias build include surface removed | runtime modules own all active includes | `KeyVerificationPresentationSource`, `KeyVerificationActionSink`, and `KeyVerificationSessionAdapter` in `modules/ui_key_verification_runtime` | keep runtime headers as the only build-visible API | retired from build include surface |
| MapOverlay legacy source | alias build include surface removed | runtime modules own all active includes | `MapOverlaySnapshotSource` and `MapOverlayProjectionAdapter` in `modules/ui_map_runtime` | keep runtime headers as the only build-visible API | retired from build include surface |

## Phase 9.6 Final Readiness Alignment

The runtime fallbacks that carry into Phase 10 are:

- LinuxSim hardcoded runtime routing: contained fallback with owner and exit
  condition; Phase 10 first cut makes `AsciiRuntimeEntryAdoption as primary source`.
- GTK hardcoded page registry: contained fallback with owner and exit condition.
- LVGL hardcoded menu/page creation: contained fallback with owner and exit
  condition.

The legacy adapter rows are not primary UI fallback rows anymore:

- Chat LegacyDelivery bridges: main runtime callers removed; alias build include
  surface removed.
- KeyVerification legacy source/sink: main runtime callers removed; alias build
  include surface removed.
- MapOverlay legacy source: main runtime callers removed; alias build include
  surface removed.
