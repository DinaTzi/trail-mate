# Cardputer Zero Linux App Shell

This app shell is the Trail Mate product route for the Cardputer Zero portable
Linux device. It is not the Linux simulator.

The first slice intentionally keeps the runtime entry small: the shell validates
the `cardputerzero` target profile, consumes the `boards/cardputerzero` facts,
selects the `cardputer_compact` UX pack, and builds through
`builds/linux_cmake`. That gives the device a real product boundary before the
framebuffer, evdev, launch, and package owners are filled in.

The shared LVGL main-menu profile for this target is already Cardputer Zero
specific and Pager-derived: `make_cardputer_zero_profile()` starts from
`make_pager_profile()` and then shrinks geometry for the 320 x 170 display.
The remaining UI work is page-by-page Cardputer Zero closure from that visual
baseline, not a Linux-simulator or custom Linux-panel redesign.

Known hardware facts in this repo:

- display: 320 x 170 logical pixels
- input: built-in keyboard
- touch, pointer, and trackball: absent in current board facts
- LoRa: SX1262 on `/dev/spidev0.1`, 500000 Hz, Reset=26, IRQ/DIO1=23,
  Busy=22, DIO2 RF switch and DIO3 TCXO enabled

Known Cardputer Zero Linux session facts in this shell:

- notifications are created through the standard
  `org.freedesktop.Notifications` D-Bus interface; the
  `$XDG_RUNTIME_DIR/cardputer-zero/notifyd.sock` channel is status/control only
- text entry uses Fcitx5 through the normal Linux toolkit/frontend path
- the Cardputer Zero IME addon/panel pair uses
  `$XDG_RUNTIME_DIR/cardputer-zero/ime-panel.sock` only to display preedit,
  candidates, and input-method state
- Trail Mate does not own the notification daemon implementation, Fcitx5 UI
  addon, IME panel renderer, input method engine, dictionaries, or text commit
  path
- the shared embedded touch/pinyin IME is not the Cardputer Zero Linux input
  method strategy

The simulator remains a separate development shell under `apps/linux_sim_shell`.

See `docs/targets/cardputerzero-adaptation.md` for the current validation
state and the required real screenshot closure.
