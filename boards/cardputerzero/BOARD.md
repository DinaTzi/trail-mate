# Cardputer Zero Board Facts

Sources:

- `docs/LINUX_ADAPTATION_GUIDE.md`
- `docs/targets/linux_targets.md`
- `platform/linux/common/src/core/display_profile.h`
- M5Stack Cap LoRa-1262 module documentation
- Cardputer Zero SX1262/GNSS pinout notes from the hardware package
- Cardputer Zero external wiring note from hardware bring-up:
  `G15=GPS_RX`, `G14=GPS_TX`

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
- external LoRa/GNSS module: M5Stack Cap LoRa-1262
- LoRa chip: `sx1262`
- LoRa SPI endpoint: `/dev/spidev0.1`
- LoRa SPI speed: 500000 Hz
- LoRa GPIO control lines: Reset=26, IRQ/DIO1=23, Busy=22
- LoRa DIO2 RF switch and DIO3 TCXO voltage control are required
- GPS/GNSS chip on the external module: `ATGM336H-6N@AT6668`
- GPS/GNSS protocol: `NMEA 0183 4.1`
- GPS/GNSS transport: UART, default 115200 bps
- Cardputer Zero GPS pin mapping: `G15` is GPS_RX and `G14` is GPS_TX
- GPIO14/GPIO15 must remain under the Linux UART pinmux (`TXD1`/`RXD1`)
  when used for GPS. Trail Mate must not reconfigure them as ordinary GPIO
  input/output pins from the application runtime.
- the external Cap LoRa-1262 completes both LoRa and GPS capability for this
  target

The M5Stack module documentation establishes the Cap LoRa-1262 module-level
LoRa/GNSS facts. The `G15`/`G14` mapping is Cardputer Zero external wiring
evidence and must not be inferred from Cardputer-Adv or other M5Stack
host-device pin tables.
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
- Trail Mate Linux GPS runtime validation against the final Linux serial device
  for the documented `G15`/`G14` Cap LoRa-1262 UART wiring
- notification daemon and Fcitx5 user-session integration on real device image
- launch/package route for the portable Linux device

The Linux simulator remains a separate target under `apps/linux_sim_shell`.
