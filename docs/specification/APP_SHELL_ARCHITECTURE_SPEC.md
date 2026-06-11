# App Shell Architecture Specification

## Purpose

Phase 8.3 establishes real app shell semantics before moving build files or
rewiring runtime startup.

An app shell is a product-facing startup and composition boundary. It is not a
build system project, board package, platform adapter, renderer implementation,
or runtime service container.

No behavior change in Phase 8.3.

Phase 8.3 adds skeletons and manifests only. It must not change ESP-IDF,
PlatformIO, Linux CMake, runtime behavior, UI screen behavior, or composition
root implementation.

## Core Rule

```text
Build Entrypoint invokes.
App Shell composes.
Target chooses.
Board describes.
UX Pack presents.
Renderer draws.
```

## AppShell

`AppShell` is a composition root consumer for a product target family.

An app shell may:

- select target profile
- select board package
- select platform family
- select UX profile / UX pack
- hand off to product composition
- start runtime facade
- own product startup/lifecycle handoff
- bind renderer family to presentation workspace

An app shell must not:

- own build host files
- define board facts
- contain SDK/HAL implementation
- implement screen internals
- assemble Chat/Map/GPS details behind hidden globals
- replace `builds/` as the build-system entrypoint
- become a service locator

## AppShellManifest

`AppShellManifest` is an architecture manifest that declares app shell intent.

It records:

- role
- target family
- renderer family
- future authoritative build entrypoint
- current transitional path
- future responsibilities
- forbidden responsibilities
- current status

It does not:

- decide board pins
- create runtime object graphs
- contain SDK/HAL code
- carry CMake, PlatformIO, or ESP-IDF mechanics
- replace target capability YAML files

## TargetProfile

`TargetProfile` is a product selection descriptor.

It may describe:

- target name
- board package
- platform family
- build entrypoint
- app shell
- UX profile placeholder
- status
- transitional source path

It must not:

- create runtime object graph
- define board pins
- implement protocol policy
- implement UX pack classes
- replace board manifests
- replace build entrypoints

`TargetProfile` answers "which product combination is selected?" It does not
answer "how does the runtime service graph get constructed?"

## AppShellLifecycle

`AppShellLifecycle` describes the app shell's startup handoff.

Expected lifecycle:

```text
Build entrypoint starts host runtime
  -> invokes app shell entrypoint
    -> app shell selects target profile
      -> app shell selects board/platform/UX profile
        -> app shell invokes product composition
          -> runtime facade starts
            -> renderer shell runs
```

The app shell owns the product startup boundary, not the low-level build host.

## Historical Entrypoint Burn-Down

Historical app entrypoint roots are not product architecture. Once a target has
a final app shell and build entrypoint, new runtime or build work must land in
the final owner and must not extend the historical root.

Current burn-down posture:

- ESP-IDF work must land in `builds/esp_idf`, `apps/esp32_lvgl`, `platform/`,
  `boards/`, or `modules/` according to ownership.
- Restoring `apps/esp_idf` as a compatibility source is forbidden.
- Restoring root `legacy/` or `legacy/app_implementations/` is forbidden.
- Historical names may appear only in documentation that explains removed
  history or in explicitly scoped deletion-readiness audits.

The purpose of any remaining compatibility reference is deletion planning, not
feature delivery.

## Phase App Registry Rule

A final app shell may expose a narrow diagnostic app for a bring-up phase when
the full settings/app catalog runtime has not yet been migrated into that final
entrypoint.

For ESP32-P4 C6 Phase 1, `apps/esp32_lvgl` may register a lightweight
`C6 Companion` diagnostics app that reads `platform::ui::wireless_companion`.
That app must not pull historical settings shells, old USB/CDC HostLink
services, or legacy entrypoint roots into the final ESP-IDF build.

## Skeleton App Shells

Phase 8.3 introduces these skeleton app shell names:

```text
apps/esp32_lvgl/
apps/nrf52_node/
apps/linux_uconsole_gtk/
apps/linux_sim_shell/
```

They are semantic anchors only. They do not build, register targets, or change
runtime behavior in Phase 8.3.

## Dependency Direction

Allowed direction:

```text
builds -> apps
apps -> modules + platform + boards
apps -> docs/targets + docs/ux_profiles
```

Forbidden direction:

```text
apps -> builds
apps -> generated build outputs
apps -> hidden SDK/HAL implementation
modules -> apps
platform -> apps
boards -> apps
```

## Thin App Shell Entrypoint Declaration

Phase 8.3 app shells may declare future entrypoint names in README/manifest
files. Those declarations are not implementation.

Future implementations should be thin and explicit, for example:

```text
apps/esp32_lvgl: trail_mate_esp32_lvgl_start(target_profile)
apps/nrf52_node: trail_mate_nrf52_node_start(target_profile)
apps/linux_uconsole_gtk: trail_mate_linux_uconsole_gtk_start(target_profile)
apps/linux_sim_shell: trail_mate_linux_sim_shell_start(target_profile)
```

These names are placeholders until a later phase creates real source files.

## Non-Goals

Phase 8.3 does not:

- restore historical app entrypoint roots
- add new runtime behavior to historical app paths
- modify build entrypoints outside the phase scope
- split `ui_shared`
- implement UX Pack classes
- change runtime behavior
- create a real composition root
- compile any new app shell code

## Review Rules

When adding app shell work, ask:

- Is this product startup/lifecycle handoff, or build-host mechanics?
- Is this target selection, or board hardware fact?
- Is this UX profile selection, or renderer internals?
- Is this app composition, or hidden Chat/Map/GPS service assembly?
- Is a historical entrypoint being mistaken for a final app shell?

The app shell may choose. The build entrypoint may invoke. Neither should hide
business runtime construction in a compatibility wrapper.
