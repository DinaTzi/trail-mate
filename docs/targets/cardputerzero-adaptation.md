# Cardputer Zero Adaptation

Cardputer Zero is a portable Linux device target. It is not the Linux simulator
and must not inherit simulator naming, simulator demo state, or simulator UX as
product truth.

## Product Boundary

| Layer | Cardputer Zero owner |
| --- | --- |
| Target id | `cardputerzero` |
| Build entrypoint | `builds/linux_cmake` |
| App shell | `apps/linux_cardputer_zero` |
| Board facts | `boards/cardputerzero` |
| UX pack | `cardputer_compact` |
| UI profile | `cardputer_compact_ui` |
| Page manifest | `cardputer_compact_manifest` |
| Shared menu profile | `make_cardputer_zero_profile()` |

The current app shell establishes target identity and Linux session contracts.
It does not yet claim real framebuffer, evdev, launch, packaging, or per-page
screenshot closure.

## UX Baseline

Cardputer Zero uses the compact Pager visual language under a 320 x 170
keyboard-device constraint.

The shared LVGL menu profile is intentionally Pager-derived:

```text
make_cardputer_zero_profile()
  starts from make_pager_profile()
  keeps PagerFocus menu behavior
  shrinks geometry for 320 x 170
  enables keyboard directional navigation
  hides memory statistics
```

Cardputer Zero page work must continue from this baseline. A custom Linux-panel
style preview, ASCII-only view, or simulator view is not acceptable evidence for
Cardputer Zero page adaptation.

## Screen Set

`CardputerCompactUxPack::buildScreens()` and `cardputer_compact_manifest` both
own the same eight routes:

- Dashboard
- Chat
- Contacts
- Map
- GPS
- Team
- Tracker
- Settings

The Cardputer Zero CMake test slice checks this manifest alignment directly.

## Hardware Facts

Known facts currently recorded in the repository:

- display: 320 x 170 logical pixels
- input: built-in keyboard
- touch/pointer/trackball: absent in current board facts
- LoRa: SX1262 on `/dev/spidev0.1`
- SPI speed: 500000 Hz
- Reset GPIO: 26
- IRQ/DIO1 GPIO: 23
- Busy GPIO: 22
- Power GPIO: -1
- DIO2 RF switch: enabled
- DIO3 TCXO: enabled

The LoRa pin reference is a hardware-fact input only. Trail Mate must not adopt
the external Meshtastic daemon implementation as the Cardputer Zero runtime.

## Linux Session Contracts

Notifications:

- application notifications are created through
  `org.freedesktop.Notifications`
- `$XDG_RUNTIME_DIR/cardputer-zero/notifyd.sock` is status/control only
- Trail Mate must not embed or fork the notifyd implementation
- Trail Mate must not use the notifyd shell IPC as a notification creation API

Text input:

- committed text comes through the normal Linux/Fcitx5 frontend path
- expected environment includes `XMODIFIERS=@im=fcitx` and
  `SDL_IM_MODULE=fcitx`
- `$XDG_RUNTIME_DIR/cardputer-zero/ime-panel.sock` is display-only JSON Lines
  state from the Fcitx5 UI addon to the panel
- Trail Mate must not turn the IME panel socket into a text submission path
- Trail Mate must not embed the Fcitx5 UI addon, panel renderer, input method
  engine, dictionaries, or candidate policy

## Current Validation

The Cardputer Zero Linux CMake preset validates:

- app-shell target identity
- board/UX target binding
- notification and Fcitx5 boundary declarations
- `cardputer_compact_manifest` route alignment
- Cardputer Zero menu profile selection through
  `TRAIL_MATE_CARDPUTER_ZERO_LINUX`

The menu-profile smoke test protects the existing Pager-derived main-menu
adaptation. It does not render pixels.

## Required Screenshot Closure

The adaptation is not visually complete until screenshots are captured from the
real Cardputer Zero UI path. Required evidence:

- 320 x 170 Dashboard screenshot
- 320 x 170 Chat screenshot
- 320 x 170 Contacts screenshot
- 320 x 170 Map screenshot
- 320 x 170 GPS screenshot
- 320 x 170 Team screenshot
- 320 x 170 Tracker screenshot
- 320 x 170 Settings screenshot
- each screenshot resembles the Pager compact visual language
- keyboard navigation focus is visible and stable
- no text overlaps, wraps into controls, or hides adjacent labels
- modal/picker flows fit the 320 x 170 viewport
- Fcitx5 committed text reaches compose fields through the Linux input path
- notifications are emitted through freedesktop D-Bus

Screenshots produced by ad hoc drawing scripts, simulator-only ASCII renderers,
or non-Cardputer UI shells do not satisfy this closure.

## Remaining Work

- wire the Cardputer Zero app shell to the real device renderer path
- validate framebuffer ownership on hardware
- validate evdev key mapping on hardware
- validate notifyd over the live D-Bus session
- validate Fcitx5 committed text in Trail Mate fields
- connect Trail Mate SX1262 runtime to the documented Linux endpoint
- add real per-page screenshot capture to the Cardputer Zero validation flow
- revise each business page only where the Pager-derived 320 x 170 layout
  actually fails
