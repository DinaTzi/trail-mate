# Meshtastic Node Payload Parsing Specification

Status: baseline
Updated: 2026-05-10

This document defines where Meshtastic node-fact payload semantics live in
Trail Mate.

## Confusion

`NODEINFO_APP` is not just a UI contact update.

`NODEINFO_APP` is not just a legacy `User` protobuf.

`POSITION_APP` is not a platform radio driver detail.

Linux, ESP Arduino, ESP-IDF, and nRF radio adapters must not each maintain their
own interpretation of NodeInfo, User, Position, device metrics, MQTT origin, or
public-key facts.

## Boundary

Meshtastic node-fact parsing is shared protocol semantics and belongs in
`modules/core_chat`.

The shared parser owns:

- full `meshtastic_NodeInfo`
- legacy `meshtastic_User`
- embedded `meshtastic_Position` inside `NodeInfo`
- standalone `POSITION_APP`
- `via_mqtt`
- device metrics
- ignored and key-verification flags
- public-key presence and key bytes

Platform adapters own only transport context and projection:

- sender node id fallback
- channel index or channel hash
- RSSI and SNR
- hop count
- receive timestamp
- duplicate suppression or retransmit policy
- publishing events or updating `ContactService`
- platform-specific persistence of keys or diagnostics

## Required Entry Points

All raw Meshtastic receive paths must use:

- `chat::meshtastic::decodeNodeInfoPayload(...)`
- `chat::meshtastic::decodePositionPayload(...)`

Current implementation file:

- `modules/core_chat/src/infra/meshtastic/mt_node_payload.cpp`

## Invalid Implementations

The following are invalid in platform receive paths:

- direct `pb_decode(... meshtastic_NodeInfo_fields ...)`
- direct `pb_decode(... meshtastic_User_fields ...)`
- direct `pb_decode(... meshtastic_Position_fields ...)`
- separate per-platform mapping from protobuf fields into contact facts
- parsing only legacy `User` on one platform while another platform parses full
  `NodeInfo`

These operations are valid outside platform receive semantics when they are
constructing local outbound payloads, serving BLE phone API data, or testing the
shared parser.

## Acceptance Check

A platform adapter is aligned with this specification when its receive path
passes `meshtastic_Data` plus transport context into the shared parser and only
projects the decoded result into its local event/store mechanism.

