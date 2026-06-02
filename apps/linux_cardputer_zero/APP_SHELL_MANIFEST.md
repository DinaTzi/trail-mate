# App Shell Manifest: linux_cardputer_zero

## Role

Product app shell / target app shell.

## Target Family

Cardputer Zero portable Linux device.

## Renderer Family

Linux device renderer. The first build slice uses the shared ASCII/runtime
adoption path so target identity and UX selection are build-closed before the
framebuffer and evdev owner are promoted into this shell.

## Board Facts

- board id: `cardputerzero`
- display: 320 x 170 logical pixels
- input: built-in keyboard, real key mapping still needs device sampling
- pointer/touch/trackball: not present in current board facts

## Build Entrypoint

- `builds/linux_cmake`

## Responsibilities

May:

- select the `cardputerzero` target profile
- select the Cardputer Zero device UX profile
- bind Cardputer Zero board facts into Linux runtime startup
- own future framebuffer, evdev, packaging, and launch details for this device

Must not:

- identify itself as a Linux simulator
- depend on simulator demo data as product truth
- define protocol, chat, map, or storage semantics
- absorb shared Linux runtime code that belongs in `platform/linux/common`
- hide missing real-device hardware integration behind simulator naming

## Thin App Shell Entrypoint Declaration

```text
trail_mate_linux_cardputer_zero_start(target_profile)
```

## Current Status

Device app shell baseline. This establishes the product target, board facts, UX
selection, and CMake wiring. Real framebuffer, evdev, and device packaging are
the next hardware-closure slices.
