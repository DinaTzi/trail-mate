# App Shell Manifest: linux_cardputer_zero

## Role

Product app shell / target app shell.

## Target Family

Cardputer Zero portable Linux device.

## Renderer Family

Linux device renderer. The first build slice uses the shared ASCII/runtime
adoption path so target identity and UX selection are build-closed before the
framebuffer and evdev owner are promoted into this shell.

The shared LVGL main-menu profile is not simulator-owned. Cardputer Zero selects
`make_cardputer_zero_profile()`, which is derived from the Pager profile and
then constrained for the 320 x 170 keyboard device.

## Board Facts

- board id: `cardputerzero`
- display: 320 x 170 logical pixels
- input: built-in keyboard, real key mapping still needs device sampling
- pointer/touch/trackball: not present in current board facts
- LoRa: SX1262 on `/dev/spidev0.1`, 500000 Hz, Reset=26, IRQ/DIO1=23,
  Busy=22, DIO2 RF switch enabled, DIO3 TCXO enabled

## Linux Session Component Facts

- notifications: standard `org.freedesktop.Notifications` D-Bus provider
  supplied by the Cardputer Zero user session
- notification shell IPC: `$XDG_RUNTIME_DIR/cardputer-zero/notifyd.sock`,
  status/control only, never a notification creation path
- input method: Fcitx5 text input through the normal Linux frontend path
- IME display bridge: `cardputerzero-ui` Fcitx5 UI addon exports panel state to
  `$XDG_RUNTIME_DIR/cardputer-zero/ime-panel.sock`
- IME panel: `cardputer-zero-ime-panel` renders candidate/preedit state as a
  Wayland layer-shell projection and does not submit text
- expected session environment: `XMODIFIERS=@im=fcitx`,
  `SDL_IM_MODULE=fcitx`

## Build Entrypoint

- `builds/linux_cmake`

## Responsibilities

May:

- select the `cardputerzero` target profile
- select the Cardputer Zero device UX profile
- bind Cardputer Zero board facts into Linux runtime startup
- own future framebuffer, evdev, packaging, and launch details for this device
- own future Trail Mate Linux SX1262 packet-radio wiring for the documented
  Cardputer Zero LoRa endpoint
- route application notifications through the standard freedesktop notification
  contract
- respect the Cardputer Zero Fcitx5 user-session boundary for text input

Must not:

- identify itself as a Linux simulator
- depend on simulator demo data as product truth
- define protocol, chat, map, or storage semantics
- absorb shared Linux runtime code that belongs in `platform/linux/common`
- hide missing real-device hardware integration behind simulator naming
- use Cardputer Zero notifyd shell IPC to create notifications
- embed or fork the Cardputer Zero notifyd store, toast, or notification-center
  implementation
- turn the IME panel socket into a text submission path
- embed the Cardputer Zero Fcitx5 addon, panel renderer, input method engine,
  dictionary, or candidate selection policy inside Trail Mate

## Thin App Shell Entrypoint Declaration

```text
trail_mate_linux_cardputer_zero_start(target_profile)
```

## Current Status

Device app shell baseline. This establishes the product target, board facts, UX
selection, CMake wiring, Linux session contracts, page manifest alignment, and
the Pager-derived main-menu profile guard. Real framebuffer, evdev, per-page
screenshots, and device packaging are the next hardware-closure slices.
