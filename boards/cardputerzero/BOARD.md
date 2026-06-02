# Cardputer Zero Board Facts

Sources:

- `docs/LINUX_ADAPTATION_GUIDE.md`
- `docs/targets/linux_targets.md`
- `platform/linux/common/src/core/display_profile.h`
- Cardputer Zero SX1262 pinout notes from the hardware package

This record describes current repo evidence for the Linux Cardputer Zero route.

## Identity

- board id: `cardputerzero`
- current build entry evidence: `builds/linux_cmake`
- current app shell evidence: `apps/linux_cardputer_zero`
- historical implementation evidence: `removed root linux_rpi`
- current shared Linux code: `platform/linux/common`

## Confirmed Facts

- logical display size used by current common Linux shell: 320 x 170
- keyboard input is required but real device key mapping still needs sampling
- LoRa chip: `sx1262`
- LoRa SPI endpoint: `/dev/spidev0.1`
- LoRa SPI speed: 500000 Hz
- LoRa GPIO control lines: Reset=26, IRQ/DIO1=23, Busy=22
- LoRa DIO2 RF switch and DIO3 TCXO voltage control are required
- Cardputer Zero user-session notifications are provided through the standard
  `org.freedesktop.Notifications` D-Bus interface, not through a private app API
- Cardputer Zero user-session input method display is provided by a Fcitx5 UI
  addon plus Wayland panel; text submission stays on the normal Linux input
  method path
- display/input ownership for the real Pi OS path is not yet closed
- current dedicated app shell baseline is build-owned by
  `apps/linux_cardputer_zero`

## Pending Hardware Evidence

- framebuffer/display handoff on real Cardputer Zero hardware
- evdev keyboard mapping sampled from the real device
- Trail Mate Linux SX1262 runtime validation against `/dev/spidev0.1` and the
  documented GPIO lines
- notification daemon and Fcitx5 user-session integration on real device image
- launch/package route for the portable Linux device

The Linux simulator remains a separate target under `apps/linux_sim_shell`.
