# 0.1.28-alpha Fix Audit

## Scope

This audit records the local verification target for the post-0.1.27 fixes
covering GitHub issues #43 through #51.

Issue #45 is a merged 0.1.27-alpha preparation pull request, not an open bug in
this range.

## Issue Mapping

| Issue | Status in this branch | Owning path |
| --- | --- | --- |
| #43 TrailMate and MeshCore not working yet | Covered by the MeshCore fixes split across #49, #50, and #51. Public group send, passive adverts, discovery diagnostics, and send-result propagation are fixed. Full multi-channel MeshCore companion management remains outside this bug-fix slice. | MeshCore protocol/core adapter |
| #44 Meshtastic custom settings do not work | Fixed by routing phone custom channel settings to Meshtastic primary/secondary channel state instead of MeshCore config aliases. Covered by phone-core smoke tests for Custom plus short `AQ==` PSK and manual LoRa settings. | Meshtastic phone core facade |
| #45 Prepare 0.1.27-alpha fixes | Already merged release-prep PR. No new bug-fix ownership in this branch. | Release history |
| #46 Chat/Contacts Team navigation | Fixed by rebinding shared two-pane navigation after list refresh and by keeping Team rows reachable on first entry. | UI presentation/source plus LVGL screen shell |
| #47 Random Pager SD crash | Fixed by guarding tracker SD start/stop/append/list/active-state transactions with the shared SPI lock and recorder mutex, plus fixing the Pager SD-ready timeout conversion. | ESP storage/runtime boundary |
| #48 Map settings not saved | Fixed by preserving real Pager GPS map zoom, center, pan, and follow state across menu exit/re-entry outside the reset page lifecycle. | GPS/map page runtime |
| #49 MeshCore discovery actions fail | Fixed in layers: detailed send failures, compatible zero-hop local discover frame, post-TX RX restart logging, shared radio-task RX state synchronization, RX guard window, and deferred adverts during scan. If hardware still shows no RX after `2E0080FF...`, the request frame is no longer the failing shape; next suspects are peer response, RF config, or IRQ/RX reception. | MeshCore ESP adapter |
| #50 MeshCore group messages fail without group key | Fixed by falling back to the MeshCore public PSK when no group channel key is configured, with explicit failure mapping when a provided key is malformed. Covered by MeshCore protocol strategy tests. | MeshCore protocol strategy |
| #51 MeshCore flood adverts do not populate Nearby with names | Fixed by parsing signed MeshCore advert appdata for name, node type, and location, and by giving discover responses stable MC placeholders until advert/node-info names arrive. Covered by MeshCore protocol and UI presentation tests. | MeshCore protocol strategy and contact projection |

## Compatibility Boundary Check

The fixes keep protocol interoperability where it belongs:

- MeshCore on-air control-frame shape, zero-hop discover behavior, ACK
  signature matching, RX-guard timing, peer public-key handling, and advert
  parsing stay in MeshCore protocol helpers/strategy or the ESP MeshCore
  adapter.
- MeshCore direct transmit paths that bypass the shared radio TX queue still
  report RX ownership back through `AppTasks`, matching the existing
  Meshtastic adapter radio-state contract instead of inventing a MeshCore-only
  compatibility lane.
- Chat delivery status flows through `MeshSendResult`, `ChatService`, EventBus
  send-result events, and the existing delivery projection path. UI renderers do
  not infer MeshCore ACK semantics.
- Chat and Contacts layout fixes stay in presentation/screen code. They do not
  define MeshCore protocol truth.
- Map view persistence is stored in the GPS/map runtime state, not in page-local
  widget state alone.
- Tracker crash mitigation uses the shared SPI/storage boundary, not random
  timing delays near the UI.

No compatibility layer was burned down in this bug-fix branch. No broad new
compatibility shim was introduced to hide the MeshCore breakage. Remaining
`ui_shared` LVGL screen edits are changes to existing transitional screen
surfaces, not new ownership for protocol or delivery semantics.

## Historical Paths Checked

- `docs/audits/UI_SHARED_COMPATIBILITY_SHIM_POLICY.md`
- `docs/audits/LEGACY_COMPAT_TEMP_SURFACE_INVENTORY.md`
- `docs/audits/CHAT_UI_CONTROLLER_BURNDOWN_AUDIT.md`
- `docs/specification/CHAT_PRESENTATION_IDENTITY_SPEC.md`
- `docs/specification/CHAT_DELIVERY_RUNTIME_SPEC.md`
- Local upstream MeshCore source under `.tmp/MeshCore`

## Release And Pages Surface Check

- `scripts/webflash_targets.py` still owns the browser-flashing target list and
  exposes both `tlora-pager-sx1262` and `tlora-pager-lr1121`, each with its own
  merged web-flasher asset name.
- `.github/workflows/ci.yml` builds both Pager variants and prepares web-flasher
  artifacts for both variants before release upload.
- `scripts/prepare_pages_site.py` derives Pages release metadata and manifests
  from `WEBFLASH_TARGETS`, so the SX1262 and LR1121 Pager cards remain separate
  release targets instead of relying on hand-edited download metadata.
- `site/index.html` and `site/main.js` now name the SX1262 and LR1121 Pager
  Web Flasher cards explicitly and warn users to match the card to the hardware
  radio chip.

## Local Validation Targets

Local validation performed for this slice:

- `python scripts/check_platform_ui_boundaries.py` passed.
- `node --check site/main.js` passed.
- `git diff --check` passed with only the unrelated existing `.gitignore`
  line-ending warning.
- CI-format cleanup was applied with Windows `clang-format` 14.0.6 and committed
  separately. Re-running WSL `clang-format` 18.1.3 against the CI file list
  fails on pre-existing third-party/historical files that are not part of this
  bug-fix slice; it is not equivalent to the CI `clang-format-14` job.
- `pio run -e tlora_pager_sx1262` passed after the final #49 RX-state fix.
- `pio run -e tlora_pager_lr1121` passed after the final #49 RX-state fix.
- `pio run -e tdeck` passed after the final #49 RX-state fix.
- `pio run -e lilygo_twatch_s3` passed after the final #49 RX-state fix.
- `pio run -e gat562_mesh_evb_pro` passed after the final #49 RX-state fix.
- WSL Ubuntu 24.04 `cmake --preset linux-simulator-debug` passed.
- WSL Ubuntu 24.04 `cmake --build --preset linux-simulator-debug-build` passed.
- WSL Ubuntu 24.04 `ctest --preset linux-simulator-debug-test` passed: 68/68
  tests.
- WSL Ubuntu 24.04 `cmake --preset linux-uconsole-debug` passed.
- WSL Ubuntu 24.04 `cmake --build --preset linux-uconsole-debug-build` passed.
- WSL Ubuntu 24.04 `ctest --preset linux-uconsole-debug-test` passed.
- WSL Ubuntu 24.04 `cmake --preset linux-uconsole-release` passed.
- WSL Ubuntu 24.04 `cmake --build --preset linux-uconsole-deb` passed.
