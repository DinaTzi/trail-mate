# GPS Settings Guide

This guide explains the GPS settings shown in the device Settings app. It is a
user-facing companion to `docs/specs/gps.md`: the spec defines runtime semantics,
while this guide explains which settings to change and which settings to leave
alone.

## Quick Recommendations

For most users, start with this profile:

| Setting | Recommended value | Why |
| --- | --- | --- |
| GPS Enabled | `ON` | Allows the runtime to power and poll GPS. |
| Receiver Baud | `Auto` | Lets the board use its default compatible baud. |
| Probe Window | `900 ms` | Balanced startup detection window. |
| Receiver Profile | `Auto` | Avoids assuming a receiver family. |
| RXM Init | `Auto` | Lets the runtime decide whether receiver power-mode commands are safe. |
| GNSS Init | `Auto` | Lets the runtime decide whether constellation commands are safe. |
| NMEA Init | `Auto` | Lets the runtime decide whether internal NMEA setup is safe. |
| Location Mode | `High Accuracy` | Best default when power is available. |
| Satellite Systems | `GPS+BDS+GAL` | Good default for multi-GNSS receivers. |
| Position Strategy | `Continuous` | Keeps GPS available for map, tracker, and team features. |
| Update Interval | `1s` or `5s` | Use `1s` for live tracking; use `5s` for lower update volume. |
| Altitude Reference | `Sea Level` | Most user-facing altitude displays expect MSL-style altitude. |
| Coordinate Format | `DD` | Decimal degrees are the safest default for maps and sharing. |
| NMEA Export | `OFF` | Leave off unless an external tool needs NMEA output. |
| NMEA Sentences | `GGA+RMC+GSA+GSV` | Full diagnostic set if export is enabled. |

For ordinary T-Deck with an external GPS or GPS shield, prefer conservative
receiver settings until the module is proven:

| Setting | Safer value |
| --- | --- |
| Receiver Baud | `9600` if Auto does not find the module |
| Receiver Profile | `NMEA Passive` for generic NMEA/CASIC/L76K-style modules |
| RXM Init | `Skip` unless the receiver is known to support the command |
| GNSS Init | `Skip` unless the receiver is known to support the command |
| NMEA Init | `Skip` unless the receiver is known to support the command |

## Mental Model

The GPS settings fall into three separate groups. Do not treat every setting as
an accuracy knob.

1. Transport and receiver compatibility.

   These settings decide how the firmware opens and initializes the receiver:
   baud rate, probe timing, receiver family profile, and whether receiver-specific
   setup commands may be sent.

2. Runtime behavior.

   These settings decide how the GPS runtime behaves once the receiver is usable:
   enabled state, collection interval, power strategy, location mode, and
   constellation selection.

3. Presentation and external export.

   These settings decide how position data is displayed or exported. They do not
   make the internal GPS parser more accurate.

The key distinction:

- `GPS Enabled` means user intent.
- `Ready` in diagnostics means the transport path is open.
- A valid fix means the receiver has produced a usable position.
- NMEA export is for external consumers and is separate from the internal GPS
  stream used by the firmware.

## Setting Reference

### GPS Enabled

Controls whether the GPS runtime is allowed to power and poll the receiver.

Use `ON` when you want maps, tracker, team position, time sync, or sky plot data.
Use `OFF` to stop live GPS collection and power down the receiver when the board
supports doing so.

This is not the same as fix state. `GPS Enabled = ON` does not guarantee that the
receiver is connected, speaking, or fixed.

### Receiver Baud

Controls the UART baud rate used to talk to the GPS receiver.

Options:

- `Auto`
- `9600`
- `38400`
- `115200`
- `57600`
- `19200`
- `4800`

Use `Auto` first. Use a fixed baud when the module documentation states a known
rate or Auto opens the transport but the receiver never produces valid NMEA.

For ordinary T-Deck GPS shield or L76K/CASIC-style modules, `9600` is the safest
manual starting point.

Changing this setting may require restarting GPS or rebooting before the physical
UART is reopened with the new rate.

### Probe Window

Controls the short startup window used for receiver detection or compatibility
probing.

Options:

- `250 ms`
- `500 ms`
- `900 ms`
- `1600 ms`

Use `900 ms` as the default. Shorter windows start faster but can miss slow or
cold receivers. Longer windows help slow modules but increase startup time.

If the receiver occasionally appears after boot but not consistently, try
`1600 ms`.

### Receiver Profile

Tells the runtime how conservative it should be with receiver-specific behavior.

Options:

- `Auto`
- `NMEA Passive`
- `u-blox Legacy`
- `u-blox Modern`

Use `Auto` for normal operation. Use `NMEA Passive` for generic NMEA modules,
CASIC/L76K-style modules, or unknown GPS modules where the safest behavior is to
listen for NMEA and avoid vendor-specific commands.

Use `u-blox Legacy` or `u-blox Modern` only when the installed receiver is known
to be u-blox and you want the firmware to allow UBX configuration commands.
Selecting a u-blox profile for a non-u-blox receiver can prevent useful GPS
traffic or create confusing diagnostics.

### RXM Init

Controls whether the firmware may send receiver power-mode configuration.

Options:

- `Auto`
- `Skip`
- `Send`

Use `Auto` normally. Use `Skip` for unknown, generic NMEA, CASIC/L76K-style, or
problematic external modules. Use `Send` only when the receiver is known to
support the command path selected by the active receiver profile.

On ordinary T-Deck, `Auto` is intentionally conservative and skips UBX
configuration unless the receiver/protocol profile allows it.

### GNSS Init

Controls whether the firmware may send constellation configuration to the
receiver.

Options:

- `Auto`
- `Skip`
- `Send`

Use `Auto` normally. Use `Skip` if the receiver is generic NMEA, unknown, or if
changing Satellite Systems causes GPS traffic to stop. Use `Send` only for known
receivers that support the configured command family.

This setting gates whether `Satellite Systems` can be pushed into the receiver.
If it is skipped, the UI preference may be saved but the receiver can continue
using its own internal constellation configuration.

### NMEA Init

Controls whether the firmware may send internal NMEA message-rate configuration
to the receiver.

Options:

- `Auto`
- `Skip`
- `Send`

Use `Auto` normally. Use `Skip` for unknown or NMEA-passive receivers. Use `Send`
only for receivers that support the selected command family.

This setting is about receiver configuration, not the external NMEA export
feature.

### Location Mode

Selects the desired receiver behavior profile.

Options:

- `High Accuracy`
- `Power Save`
- `Fix Only`

Use `High Accuracy` when you care about live maps, tracking, or stable team
position updates.

Use `Power Save` when battery life matters more than update responsiveness. Some
receiver/configuration combinations may ignore this setting.

Use `Fix Only` when the product goal is to obtain a position occasionally rather
than keep a continuously warm receiver.

On u-blox-style configuration paths, power-save behavior may be disabled when
GLONASS is selected because that combination is not supported by the current
receiver command logic.

### Satellite Systems

Selects the GNSS constellations the firmware should request from compatible
receivers.

Options:

- `GPS+BDS+GAL`
- `GPS`
- `GPS+BDS`
- `GPS+GAL`
- `GPS+BDS+GAL+GLO`

Use `GPS+BDS+GAL` as the default. It gives broad sky coverage without enabling
every possible constellation.

Use `GPS` for older or simpler receivers, or when you want the most conservative
configuration.

Use `GPS+BDS+GAL+GLO` only when the receiver supports it and you do not need
power-save receiver mode. GLONASS can conflict with some power-save paths.

This setting only changes the receiver when GNSS initialization is allowed and
supported. It does not create satellites on a receiver that lacks that GNSS
capability.

### Position Strategy

Controls when GPS should stay powered.

Options:

- `Continuous`
- `Motion Wake`
- `Low Power Off`

Use `Continuous` for normal navigation, map following, track recording, and team
location sharing.

Use `Motion Wake` when the board has a supported motion sensor and you want GPS
to stay active while moving, then power down after the motion idle timeout.

Use `Low Power Off` when you want to keep GPS off for battery saving. In this
mode, diagnostics may show `GPSD_POWER_OFF` even though `GPS Enabled` is still
on.

Team mode can force GPS on while team features need live position.

### Update Interval

Controls the collection or publish interval for GPS observations.

Options:

- `1s`
- `2s`
- `5s`
- `10s`

Use `1s` for live navigation and detailed tracks. Use `5s` or `10s` when battery
life and smaller logs matter more.

This is not the UART poll interval. The firmware may still read the receiver
more frequently internally. This setting controls how often GPS observations are
published or sampled by runtime policy.

Low battery power tiers may force a longer effective interval than the UI value.

### Altitude Reference

Controls how altitude should be interpreted or displayed.

Options:

- `Sea Level`
- `Ellipsoid`

Use `Sea Level` for most user-facing altitude displays. Use `Ellipsoid` only when
you know your workflow expects raw ellipsoid height rather than mean sea level
style altitude.

### Coordinate Format

Controls how coordinates are displayed in UI surfaces that honor this setting.

Options:

- `DD`
- `DMS`
- `UTM`

Use `DD` for decimal degrees. It is the most compatible format for maps, links,
and sharing.

Use `DMS` when you need degrees-minutes-seconds notation. Use `UTM` for grid
navigation workflows.

This setting does not change the GPS receiver or the internal coordinate system.

### NMEA Export

Controls user-visible NMEA output for external consumers.

Options:

- `OFF`
- `1Hz`
- `5Hz`

Leave this `OFF` unless another tool needs NMEA output. Turning it on is not
required for the internal GPS runtime, map, tracker, or sky plot.

NMEA export is separate from the internal GPS stream. Do not use it as a privacy
control and do not assume disabling export disables internal GPS parsing.

### NMEA Sentences

Selects which sentence group to export when NMEA Export is enabled.

Options:

- `GGA+RMC+GSA+GSV`
- `RMC+GSA+GSV`
- `GGA+RMC`

Use `GGA+RMC+GSA+GSV` when debugging or feeding tools that need satellite
diagnostics. Use `GGA+RMC` for compact position-only consumers. Use
`RMC+GSA+GSV` when altitude/quality from GGA is not needed.

This setting does not decide which internal receiver sentences the firmware
needs for its own GPS features.

### Diagnostics

Opens a snapshot of GPS health.

Important fields:

- `Code`: high-level diagnostic result such as `GPSD_OK`, `GPSD_NO_FIX`,
  `GPSD_NO_UART_TRAFFIC`, or `GPSD_POWER_OFF`.
- `Supported`: whether the board build supports GPS.
- `Enabled`: user intent from GPS Enabled.
- `Powered`: whether the runtime currently has GPS powered.
- `Ready`: whether the transport path is open.
- `Fix`: whether a valid position is available.
- `Sats`, `View`, `Use`: satellite counts when known.
- `Chars` and `Recent`: receiver/parser traffic counters.
- `Last RX`: age of the last receiver byte.
- `Poll`: internal receiver poll interval.
- `Publish`: effective GPS collection interval.

Do not read `Ready=1` as "GPS module is healthy." On UART-backed boards it can
mean only that the UART transport is open.

## Recipes

### Ordinary T-Deck With LilyGo GPS Shield Or Generic NMEA Module

Start with:

- GPS Enabled: `ON`
- Receiver Baud: `9600`
- Probe Window: `900 ms`
- Receiver Profile: `NMEA Passive`
- RXM Init: `Skip`
- GNSS Init: `Skip`
- NMEA Init: `Skip`
- Location Mode: `High Accuracy`
- Satellite Systems: `GPS+BDS+GAL`
- Position Strategy: `Continuous`
- Update Interval: `1s` or `5s`

This profile listens for receiver output without assuming the module is u-blox.
It is the safest first configuration for user-replaceable GPS modules.

If valid NMEA appears and the module documentation confirms u-blox support, you
can try a u-blox profile and `Auto`/`Send` init policies later. Change one
setting at a time.

### Known u-blox Receiver

Start with:

- Receiver Profile: `u-blox Modern` for modern u-blox modules, or
  `u-blox Legacy` for older modules.
- RXM Init: `Auto`
- GNSS Init: `Auto`
- NMEA Init: `Auto`
- Satellite Systems: choose the constellations supported by the module.

Use `Send` only when Auto is too conservative and you are sure the receiver
supports the command. If GPS traffic stops after changing these settings, return
the relevant init policy to `Skip` and reboot.

### Battery-Saving Tracker

Start with:

- Position Strategy: `Motion Wake` if the board has a supported motion sensor.
- Update Interval: `5s` or `10s`.
- Location Mode: `Power Save`.
- NMEA Export: `OFF`.

If the board has no supported motion sensor, Motion Wake may behave like
Continuous or may not provide the expected savings.

### Debugging No Fix

Use this sequence:

1. Set GPS Enabled to `ON`.
2. Set Position Strategy to `Continuous`.
3. Set Receiver Baud to `Auto`; if no traffic appears, try the documented module
   baud, commonly `9600`.
4. Set Receiver Profile to `NMEA Passive`.
5. Set RXM Init, GNSS Init, and NMEA Init to `Skip`.
6. Open Diagnostics.
7. Wait outdoors or near a window with antenna sky view.

Interpret the result:

| Diagnostic pattern | Meaning |
| --- | --- |
| `Powered=0` | GPS is off due to strategy, board support, or runtime power policy. |
| `Ready=0` | Transport is not open or the board has not prepared GPS. |
| `Ready=1`, `Chars=0`, `Last RX=never` | UART is open but no receiver traffic has been seen. Check baud, wiring, power, and reset. |
| `Recent` rises but no fix | Receiver is speaking but has not fixed yet, or the stream is noise. Check raw logs and sky view. |
| Valid NMEA appears but no fix | Receiver is alive; wait longer, move outdoors, or check antenna. |
| Random `[GPS][RAW_BURST]` appears after LoRa TX | See the T-Deck GPS UART / LoRa TX noise known issue document. |

## T-Deck UART Noise Note

On ordinary T-Deck, LoRa TX can induce bytes on the external GPS UART RX path if
the GPS TX line is floating or weakly driven. GPS settings can reduce parser
confusion, but they cannot remove the physical noise from `GPIO44 / UART0_RX`.

For this issue:

- Keep Receiver Profile at `NMEA Passive` unless the receiver is proven u-blox.
- Keep RXM Init, GNSS Init, and NMEA Init at `Skip` while debugging unknown
  modules.
- Use Diagnostics and serial logs to distinguish real GPS data from UART noise.
- If changing LoRa TX power changes GPS UART byte counts, treat it as a hardware
  coupling symptom, not as a GPS accuracy setting.

See `docs/devices/lilygo-tdeck-gps-uart-lora-noise.md` for the hardware
verification procedure.

## What Not To Do

- Do not enable u-blox profiles for unknown GPS modules just to "try harder."
- Do not set all init policies to `Send` unless the receiver supports them.
- Do not treat GPS Enabled as a guarantee of a physical module or a fix.
- Do not treat NMEA Export as required for internal GPS operation.
- Do not use Update Interval to diagnose UART noise.
- Do not change several compatibility settings at once. Change one setting,
  reboot or restart GPS if needed, then check Diagnostics.

