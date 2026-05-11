# LilyGo T-Deck GPS UART / LoRa TX Noise Known Issue

This document records a known hardware-facing issue observed on the ordinary
LilyGo T-Deck, plus the current software mitigation in this repository.

For general GPS Settings guidance, see
`docs/devices/gps-settings-guide.md`.

Scope:

- Applies to ordinary `tdeck`.
- Does not describe `tdeck_pro`.
- Concerns the external GPS / GPS shield UART path on `GPIO43/GPIO44`.
- The issue was diagnosed from real-device logs and the ordinary T-Deck
  schematic in May 2026.

## Summary

The ordinary T-Deck exposes `UART0_TX/UART0_RX` on external connectors. In this
repository those lines are also used as the GPS UART:

- `GPS_TX = GPIO43`
- `GPS_RX = GPIO44`
- MCU schematic labels: `U0TXD / UART0_TX` and `U0RXD / UART0_RX`

LoRa uses separate pins and does not logically share the GPS UART:

- LoRa SPI / control: `CS=9`, `BUSY=13`, `RST=17`, `DIO1=45`,
  `SCK=40`, `MOSI=41`, `MISO=38`

Observed behavior shows that LoRa transmission can cause real bytes to appear on
the GPS UART RX path. These bytes are not valid NMEA and are not UBX frames. The
current leading explanation is that `GPIO44 / UART0_RX / GPS_RX` is either
floating or weakly driven by the attached GPS/shield path, so LoRa TX RF or power
transients are sampled by the UART peripheral as serial frames.

This is not primarily a TinyGPS memory corruption symptom. Logs with the
additional diagnostics show that the TinyGPS character counter and the UART
read counter rise together.

## Critical Distinctions

Keep these concepts separate during future debugging:

- `GPS ready=1`: the board runtime has opened the UART and marked the transport
  path ready. On ordinary T-Deck this does not prove that a GPS module is powered,
  attached, or speaking.
- `read_bytes` / `read_10s`: bytes actually read from the UART stream.
- `chars_total` / `chars_10s`: bytes accepted by TinyGPS in older diagnostics,
  or NMEA candidate bytes after the software mitigation described below.
- Valid GPS stream: NMEA sentences beginning with `$`, or a recognized binary
  receiver protocol if deliberately supported.
- LoRa TX event: radio transmission on the SX1262 path. It does not write to the
  GPS UART in software, but it can disturb a floating or weakly driven RX line.

## Observed Symptoms

The recurring symptom set:

- GPS service starts and reports ready.
- No fix is acquired.
- No satellites are visible or used.
- Early raw bytes appear on the GPS UART, but they are not NMEA.
- After long idle periods, sending a LoRa message causes GPS UART traffic to
  resume.
- The resumed traffic is random-looking, has no NMEA comma structure, and has no
  UBX sync pattern.
- The induced UART byte count is TX-power dependent in real-device testing:
  `22 dBm` produced hundreds of UART bytes after a LoRa transmit, while `4 dBm`
  produced none during the same monitoring window.

Representative startup log:

```text
[GPS Task] Loop 2: GPS ready=1, valid=0, mutex_ok=1
[GPS][RAW] reason=first64_no_nmea len=64 hex=B1 B9 4E 8E 8E AA B0 56 4E 8E 00 80 B6 AE 4E 48 8E B6 6A 76 A7 03 8E A0 6A F6 76 8E 00 A0 B6 F6 76 49 8A A0 B6 F6 76 5E 00 A0 B6 76 76 5E 8E 2A B6 76 56 5E 8E A0 B6 76 56 49 8E 80 B6 76 56 4E
[GPS][RAW_BURST] n=1 reason=non_nmea_burst64 len=64 bytes=64 gap_ms=0 age_ms=62 printable=23 comma=0 high=33 zero=5 ubx_sync=0 hex=8E 8E 39 2F 56 4E 8E 8E A0 76 56 4E 40 00 A0 76 5E 8E 8E 00 B0 B6 48 87 8E 8E B2 B6 76 2B 03 00 AA B6 F6 76 3A 03 80 B6 B6 76 56 1B AC 8E A0 B4 B6 76 85 B6 76 76 5E 8E 8E B9 76 C8 48 00 00 B4
[GPS Task] GPS loop processed 512 characters this cycle (total: 512, read_bytes=512)
```

Representative idle health log before a LoRa transmit:

```text
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=676 chars_10s=0 read_10s=0 poll_ms=250 collection_ms=60000 loops=439
```

Representative LoRa transmit followed by GPS UART noise:

```text
[MT][TX] queue text id=30FEAC58 dest=FFFFFFFF logical_ch=0 len=4
[MT][TX_ROUTE] id=30FEAC58 dest=FFFFFFFF port=1 logical_ch=0 wire_ch=8 path=CHANNEL payload=20
[GPS][RAW_BURST] n=2 reason=non_nmea_burst64 len=64 bytes=64 gap_ms=112996 age_ms=2 printable=26 comma=0 high=35 zero=2 ubx_sync=0 hex=49 B9 76 56 48 8E 8E B9 2F 5E 4E 48 00 80 35 27 5E 8E 00 80 B6 42 87 48 8E 80 B4 B6 AF 02 8E 80 B4 B6 B9 8E 40 80 B6 B6 76 76 8E A0 B4 F6 76 5E 45 DB B6 B6 76 C8 48 80 6A F6 76 5E 48 B0 B6 76
[GPS Task] GPS loop processed 92 characters this cycle (total: 768, read_bytes=92)
[GPS Task] GPS loop processed 210 characters this cycle (total: 978, read_bytes=210)
[GPS Task] GPS loop processed 128 characters this cycle (total: 1106, read_bytes=128)
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=1106 chars_10s=430 read_10s=430 poll_ms=250 collection_ms=60000 loops=479
```

The important evidence is `chars_10s=430 read_10s=430`. The UART read count and
the GPS character counter rose together. That means the firmware read real bytes
from the UART path.

### TX Power Correlation Test

A later A/B test changed only the LoRa TX power class during the same style of
manual message transmission.

At `22 dBm`, one outgoing LoRa text packet was followed by GPS UART reads:

```text
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=267 chars_10s=0 read_10s=0 poll_ms=250 collection_ms=60000 loops=279
[MT][TX] queue text id=064BE2C3 dest=FFFFFFFF logical_ch=0 len=4
[MT][TX_ROUTE] id=064BE2C3 dest=FFFFFFFF port=1 logical_ch=0 wire_ch=8 path=CHANNEL payload=20
[GPS Task] GPS loop processed 59 characters this cycle (total: 326, read_bytes=59)
[GPS Task] GPS loop processed 212 characters this cycle (total: 538, read_bytes=212)
[GPS Task] GPS loop processed 159 characters this cycle (total: 697, read_bytes=159)
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=697 chars_10s=430 read_10s=430 poll_ms=250 collection_ms=60000 loops=319
```

The per-loop UART reads sum to `430` bytes (`59 + 212 + 159`), and the next
health line confirms `read_10s=430`.

At `4 dBm`, the same style of LoRa transmit did not produce GPS UART reads in
the observed window:

```text
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=697 chars_10s=0 read_10s=0 poll_ms=250 collection_ms=60000 loops=840
[MT][TX] queue text id=064BE2C4 dest=FFFFFFFF logical_ch=0 len=4
[MT][TX_ROUTE] id=064BE2C4 dest=FFFFFFFF port=1 logical_ch=0 wire_ch=8 path=CHANNEL payload=20
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=697 chars_10s=0 read_10s=0 poll_ms=250 collection_ms=60000 loops=880
[MT][RX] from=0C16AAEC to=FFFFFFFF id=064BE2C4 flags=0xE6 ch=8 next=0 relay=228 len=20
[MT][IMPLICIT_ACK] observed self-broadcast id=064BE2C4 relay=000000E4 next=00000000 ch=8
[GPS] health ready=1 powered=1 state=nofix sats=0 view=0 use=0 chars_total=697 chars_10s=0 read_10s=0 poll_ms=250 collection_ms=60000 loops=960
```

`chars_total` stayed at `697`, and both `chars_10s` and `read_10s` stayed at
`0`. This does not prove the exact coupling path by itself, but it strongly
supports the hardware-facing hypothesis: higher LoRa TX energy can disturb the
GPS UART RX input, while a much lower TX power may be below the threshold needed
to create sampled UART frames.

The same boot also showed `chars_10s=267 read_10s=678` during startup noise.
That means the current lightweight NMEA gate can reject some noise, but random
bytes can still occasionally look like an NMEA candidate. For hardware
diagnosis, `read_10s` remains the most direct indicator of whether the MCU
actually received bytes on `UART0_RX / GPIO44`.

## Hardware Analysis

UART RX is a high-impedance input. A healthy attached GPS module should drive it
to a stable idle high level and only pull it low to send start bits. If the GPS
module is not connected, not powered, reset, in high impedance state, wired to
the wrong pin, or otherwise not driving its TX output, `GPIO44` can behave like
a floating input.

During LoRa transmission, several physical effects can create edges on such a
line:

- RF coupling from the SX1262 transmit path, antenna, or nearby conductors.
- 3V3 rail and ground transients caused by the higher TX current.
- Ground reference movement between the radio, MCU, external connector, and GPS
  module.
- Long or poorly referenced jumper/shield wiring acting as an antenna.

The UART peripheral does not know about GPS or NMEA. If it sees a falling edge
that resembles a start bit at the configured baud rate, it samples the following
bit periods and places a byte in the FIFO. Noise can therefore produce byte
patterns such as `49 B9 76 56 48 8E ...`.

The captured noise does not match valid GPS stream structure:

- NMEA should contain `$` starts and comma-separated ASCII fields.
- The burst logs show `comma=0`.
- UBX should contain `B5 62` sync.
- The burst logs show `ubx_sync=0`.
- The byte stream has many high-bit bytes, which is unlike normal NMEA.

## Ruled-Out Or Lower-Priority Explanations

### TinyGPS Counter Memory Corruption

Lower priority for this specific symptom.

The added diagnostics showed `read_10s` and `chars_10s` rising by the same
amount. If memory corruption were the primary cause, a stronger signal would be
`chars_10s > 0` while `read_10s = 0`, or `[GPS][COUNT_CHECK]` entries showing a
TinyGPS delta without matching UART reads.

### LoRa Software Writing Into The GPS UART

No evidence found.

The ordinary T-Deck pin map separates LoRa SPI/control lines from
`GPIO43/GPIO44`. The observed bytes are read from the GPS UART stream after
radio TX, but the software LoRa path does not intentionally write to that stream.

### u-blox-Specific Configuration Failure

Not the leading explanation for the noise bursts.

The ordinary T-Deck must not assume that the GPS receiver is u-blox. Current
runtime policy skips UBX configuration unless the detected protocol or explicit
profile permits it. The observed LoRa-correlated bytes also occur when receiver
config writes are not active.

## Current Software Mitigations

The repository currently applies these mitigations for ordinary T-Deck GPS
bring-up and diagnostics:

1. GPS UART access is serialized.

   GPS initialization/configuration and the collector task must not read/write or
   reopen the same physical UART concurrently. The GPS service uses a recursive
   UART lock around init/config/open/close and parser loop access.

2. Ordinary T-Deck does not blindly send UBX configuration.

   UBX-specific GNSS and NMEA configuration is skipped unless the receiver is
   known or explicitly configured as u-blox.

3. `GPIO44 / GPS_RX` enables an internal pull-up after `Serial1.begin`.

   UART idle is high. The weak pull-up is a safe diagnostic/mitigation for a
   floating RX line, but it may not be strong enough to overcome RF or wiring
   coupling.

4. Diagnostics distinguish UART reads from parser input.

   The logs include:

   - per-loop `read_bytes`
   - health-window `read_10s`
   - `[GPS][COUNT_CHECK]` if UART reads and parser count diverge
   - `[GPS][RAW_BURST]` for non-NMEA bursts after idle periods

5. Non-NMEA noise is filtered before TinyGPS.

   The GPS loop only feeds TinyGPS when the incoming stream looks like an NMEA
   candidate sentence beginning with `$`. Random UART noise is still counted as
   UART traffic through `read_bytes` / `read_10s`, but it should no longer make
   TinyGPS's `charsProcessed()` counter look like valid GPS parser progress.

This software mitigation protects GPS state and diagnostics. It does not solve
the underlying hardware signal-integrity or connection issue.

## Expected Logs After The Mitigation

When LoRa TX still induces noise but the parser filter works, future logs should
look like this pattern:

```text
[GPS][RAW_BURST] ...
[GPS Task] GPS loop processed 0 characters this cycle (total: <unchanged>, read_bytes=<nonzero>)
[GPS] health ... chars_10s=0 read_10s=<nonzero> ...
```

Interpretation:

- `read_10s > 0`: the UART RX line still saw real bytes.
- `chars_10s = 0`: those bytes were rejected before TinyGPS because they were
  not NMEA candidates.

If future logs show this instead:

```text
[GPS][COUNT_CHECK] read_bytes=0 tinygps_delta=<nonzero>
```

then reopen the memory-corruption hypothesis.

## How To Check Your Own T-Deck

Use this procedure when you want to know whether a specific ordinary T-Deck has
the LoRa-induced GPS UART noise issue.

The short version: watch `read_10s`, send LoRa messages, and check whether GPS
UART reads appear only after radio transmission.

### Prerequisites

- Use an ordinary `tdeck` build with GPS diagnostics enabled.
- Open the serial monitor and keep the full log around each transmit event.
- Note the LoRa TX power used for each test.
- Record whether the GPS module/shield is attached, disconnected, or externally
  pulled up.

### Step 1: Establish An Idle Baseline

Boot the device and wait for at least two GPS health intervals without manually
sending LoRa messages.

The useful line is:

```text
[GPS] health ... chars_10s=<n> read_10s=<n> ...
```

Expected idle result after startup settles:

```text
[GPS] health ... chars_10s=0 read_10s=0 ...
```

Some boards may show startup noise before the line settles. That is still useful
evidence, but the LoRa-correlation test should start from a quiet idle window
where `read_10s=0`.

### Step 2: Send A LoRa Message At Normal Or High TX Power

Send one short LoRa message and capture the logs from the transmit line through
the next GPS health line.

Relevant LoRa markers:

```text
[MT][TX] queue text ...
[MT][TX_ROUTE] ...
```

A failing pattern is:

```text
[MT][TX] queue text ...
[MT][TX_ROUTE] ...
[GPS Task] GPS loop processed <nonzero> characters this cycle (... read_bytes=<nonzero>)
[GPS] health ... chars_10s=<nonzero> read_10s=<nonzero> ...
```

If `read_10s` rises immediately after the transmit, the MCU really received bytes
on the GPS UART RX path. If those bytes are accompanied by `[GPS][RAW_BURST]`
with `comma=0`, many high-bit bytes, and `ubx_sync=0`, treat them as noise rather
than real GPS traffic.

### Step 3: Repeat At Lower TX Power

Reduce LoRa TX power and repeat the same message test.

This pattern strongly supports the known issue:

```text
High TX power: [GPS] health ... read_10s=430 ...
Low TX power:  [GPS] health ... read_10s=0 ...
```

The exact byte count does not need to be `430`. The important signal is that
higher TX power produces GPS UART reads and lower TX power reduces or eliminates
them.

### Step 4: Isolate The GPS/Shield Path

If the symptom appears, run the same transmit test under these hardware states:

1. GPS/shield disconnected.

   If `read_10s` still rises after LoRa TX, the board-side
   `UART0_RX / GPIO44` input or connector path is susceptible while floating.

2. GPS/shield connected and powered.

   If the issue appears only with the GPS/shield attached, inspect GPS TX idle
   level, GPS power/reset state, wiring, and shield routing.

3. `UART0_RX / GPIO44` temporarily pulled to `3V3` with `4.7k` to `10k`.

   If the pull-up prevents LoRa-induced `read_10s` increases, the RX input is
   floating or weakly driven.

### Step 5: Decide The Result

Use this table as the quick interpretation guide:

| Observation | Interpretation |
| --- | --- |
| `read_10s=0` before LoRa TX and remains `0` after several normal-power sends | This board/test setup does not show the issue in the observed conditions. |
| `read_10s` rises immediately after LoRa TX, with random `[RAW_BURST]` bytes | Likely LoRa-induced GPS UART noise. |
| High TX power produces `read_10s>0`, low TX power produces `read_10s=0` | Strong evidence of RF or supply-transient coupling into the GPS UART RX path. |
| GPS/shield disconnected and LoRa TX still produces `read_10s>0` | Strong evidence that the board-side RX path is vulnerable when floating. |
| External `4.7k` to `10k` pull-up makes the symptom disappear | Strong evidence that `UART0_RX / GPIO44` was floating or weakly driven. |
| `chars_10s>0` while `read_10s=0`, or `[GPS][COUNT_CHECK]` appears | Reopen the software counter or memory-corruption investigation. |
| Valid `$GNRMC`, `$GNGGA`, `$GNGSA`, or `$GNGSV` sentences appear with commas and checksums | This is GPS data, not the random UART-noise signature described here. |

When reporting the result, include the `MT][TX]` lines, the health line before
the transmit, the first health line after the transmit, any `[GPS][RAW_BURST]`
lines, the TX power, and the GPS/shield wiring state.

## Hardware Verification Checklist

Use these tests to identify the physical cause:

1. Test with no GPS/shield attached.

   Send a LoRa message and watch `read_10s`. If it rises, bare `GPIO44/UART0_RX`
   is susceptible to LoRa TX when the line is floating.

2. Add a stronger external pull-up on `UART0_RX / GPIO44`.

   A temporary `4.7k` to `10k` pull-up to `3V3` is a useful diagnostic. If LoRa
   TX no longer produces UART bytes, the problem is a floating or weakly driven
   RX line.

3. Measure GPS TX idle voltage.

   With GPS attached and powered, the GPS module TX pin should idle high near
   `3.3V`. If it is low, drifting, or high impedance, the MCU RX input is not
   being driven correctly.

4. Verify UART crossing.

   GPS module `TX` must connect to MCU `UART0_RX / GPIO44`; GPS module `RX` must
   connect to MCU `UART0_TX / GPIO43`.

5. Verify GPS module power, reset, and mode pins.

   If the GPS shield exposes `RESET`, `BL_CTRL`, or similar control pins, confirm
   that they leave the module powered and its TX output active.

6. Reduce LoRa TX power temporarily.

   If noise byte counts fall with lower TX power, RF or supply coupling is part
   of the mechanism.

7. Compare against known-good passive NMEA.

   A healthy GPS stream should produce logs like:

   ```text
   [GPS][NMEA] sample1=$GNTXT,...
   [GPS][NMEA] sample2=$GNRMC,...
   [GPS][NMEA] sample3=$GNGGA,...
   ```

   It should not produce only high-bit raw bursts with `comma=0`.

## Direct Hardware Monitoring Method

The most direct verification method is to watch the GPS UART diagnostics while
manually changing only one hardware condition at a time.

Serial log fields to watch:

- `read_10s`: bytes actually read from `UART0_RX / GPIO44`.
- `chars_10s`: bytes accepted by TinyGPS/NMEA parsing after software filtering.
- `[GPS][RAW_BURST]`: a non-NMEA burst captured after an idle period.
- `[GPS][COUNT_CHECK]`: mismatch between UART reads and parser count. This should
  be absent in the normal UART-noise case.

Expected interpretation:

- `read_10s > 0` after LoRa TX means the MCU really received UART bytes.
- `read_10s > 0` with `chars_10s = 0` means the UART line is still noisy, but the
  parser filter is protecting TinyGPS.
- `read_10s = 0` after LoRa TX means no UART bytes were induced during that
  monitoring window.
- `[RAW_BURST]` with `comma=0`, many high-bit bytes, and `ubx_sync=0` should be
  treated as noise, not GPS data.

Direct test procedure:

1. Disconnect the GPS/shield and leave only the board running.

   Send a LoRa message. If `read_10s` still rises, the board-side
   `UART0_RX / GPIO44` line or connector is susceptible to LoRa TX while the RX
   input is floating.

2. Temporarily pull `UART0_RX / GPIO44` up to `3V3` with a stronger resistor.

   Use a `4.7k` to `10k` pull-up for the test. If LoRa TX no longer increases
   `read_10s`, the issue is effectively confirmed as a floating or weakly driven
   RX line. The ESP32-S3 internal pull-up is weak and may not overcome RF or
   supply coupling by itself.

3. With the GPS module attached, measure the GPS TX idle level.

   The GPS TX pin should idle high near `3.3V`. If the idle level drifts, stays
   low, or appears high impedance, then the MCU `UART0_RX / GPIO44` input is not
   being stably driven by the GPS module.

4. Verify that GPS TX/RX are crossed correctly.

   UART wiring must be crossed:

   ```text
   GPS module TX -> MCU UART0_RX / GPIO44
   GPS module RX -> MCU UART0_TX / GPIO43
   ```

   If TX/RX are reversed, MCU RX will not have a valid GPS TX driver and will be
   more likely to sample noise during LoRa TX.

5. Repeat the test with lower LoRa TX power.

   If the GPS noise byte count drops noticeably with lower TX power, RF coupling
   or supply transients are contributing to the issue. This does not, by itself,
   prove that the GPS line is floating, but it does prove a physical correlation
   between LoRa TX intensity and UART noise.

## Maintenance Notes

- Do not treat ordinary T-Deck GPS readiness as equivalent to receiver presence.
  `ready=1` can mean only that the UART transport is open.
- Do not regress to sending u-blox commands by default on ordinary T-Deck. The
  attached receiver may be CASIC/L76K, generic NMEA, u-blox, or absent.
- Keep `read_bytes/read_10s` available while this hardware issue is unresolved.
  They are the main distinction between UART-line noise and parser/counter
  corruption.
- Software can filter noise and keep state clean, but the final fix for
  LoRa-correlated UART bytes is hardware-side: stable GPS TX drive, correct
  wiring, proper power/reset state, stronger pull-up where appropriate, and
  better shielding/routing/grounding if needed.
