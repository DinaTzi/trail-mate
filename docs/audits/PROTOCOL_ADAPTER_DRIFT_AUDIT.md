# Protocol Adapter Drift Audit

Status date: 2026-06-13

This audit records known or suspected behavior drift between Trail Mate protocol adapters.
It focuses on protocol semantics, not UI rendering or board hardware differences.

## Summary

Primary root cause:

- ESP32 and nRF platform adapters currently own too much protocol business logic.
- Several behaviors are duplicated manually instead of delegated to one shared protocol core.
- Capabilities are too coarse to tell UI and use cases which protocol behaviors are truly available.

| Area | Status | Risk | Notes |
| --- | --- | --- | --- |
| Meshtastic NodeInfo peer reannounce | Shared policy in working tree | Medium | ESP32 and nRF now call the same reannounce gate; platform adapters still own the actual queue/send IO. |
| Meshtastic broadcast `want_response` | Shared policy in working tree | High | ESP32 and nRF now share the app-data destination/ACK/response decision; broadcast air ACK is suppressed while request response intent is preserved. |
| Meshtastic request/reply core | Mostly shared | Medium | NodeInfo/Position reply gating, NodeInfo/Position payload construction, TraceRoute reply gating, TraceRoute payload mutation, and TraceRoute/Position action lifecycle tracking are shared; reply radio send still lives in adapters/UI. |
| Meshtastic duplicated policy ownership | Mostly shared | Medium | App-data send intent, NodeInfo reannounce gate, NodeInfo/Position reply gates, NodeInfo self-announcement packet construction, Position payload construction, TraceRoute reply gate, TraceRoute payload mutation, TraceRoute/Position result lifecycle, and PKI/NO_CHANNEL resync decisions now live in shared runtime/policy. Radio IO and local data sources still live in adapters. |
| MeshCore NodeInfo query/reply | Shared runtime/effects | Medium | ESP32 and nRF now route `requestNodeInfo()` through `MeshCoreRuntime` and shared control payload codecs; platform adapters still own packet IO/projection. |
| MeshCore trace | Shared lifecycle, platform-limited routing | Medium | ESP32 and nRF use native `PAYLOAD_TYPE_TRACE` and shared completion/timeout policy; nRF still uses a minimal one-hop hash route. |
| MeshCore app-data ACK/capability | Shared lifecycle | Medium | ESP32 and nRF now declare ACK tracking only when runtime pending/completion handling is wired. ACK frame scheduling remains adapter IO. |
| MeshCore direct routing/identity | Capability-split platform gap | High | ESP32 advertises direct route table, identity keys, peer secret derivation, and rich trace projection; nRF advertises identity keys and peer secret derivation only. |
| MeshCore duplicated/incomplete ownership | Confirmed architecture debt | Critical | MeshCore protocol truth is split between shared helpers, ESP32 adapter, and nRF simplified adapter. |
| Capability granularity | Fine-grained flags added | Medium | Coarse legacy fields remain for compatibility; new flags describe NodeInfo, Position, TraceRoute, app response, and ACK tracking separately. |

## Architecture Finding

The dangerous part is not only that ESP32 and nRF differ. The dangerous part is that they are close
because two platform adapters independently implement protocol rules. That means correctness depends
on manual synchronization.

Target architecture:

- Shared Meshtastic runtime core owns NodeInfo, Position, TraceRoute, ACK/request-response, and PKI resync policy.
- Shared MeshCore runtime core owns NodeInfo control frames, trace, ACK tracking, route/identity policy, and
  app-data semantics.
- `PROTOCOL_RUNTIME_DESIGN_SPEC.md` now defines the required Strategy / Command / State / Bridge /
  Adapter architecture for this migration.
- ESP32 and nRF adapters own physical IO, SDK lifecycle, platform storage, timers, buffers, and queues only.
- Capability objects describe whether a shared core feature is enabled on a platform; they do not define new
  platform-specific protocol rules.

Short-term copies are allowed only as stopgaps. Every copied behavior must have an extraction target.

## Meshtastic Drift Items

### MT-001 NodeInfo Peer Reannounce

Evidence:

- ESP32: `platform/esp/arduino_common/src/chat/infra/meshtastic/mt_adapter.cpp`
  calls `maybeBroadcastNodeInfoAfterPeerAnnouncement(...)` after decoding peer NodeInfo.
- nRF before the current working change only updated node state and reply suppression maps.

Expected behavior:

- After decoding a valid non-MQTT, non-self peer NodeInfo, adapter broadcasts own NodeInfo once,
  subject to 60s suppression.

Current state:

- The reannounce decision has been extracted into
  `chat/runtime/meshtastic_protocol_policy.h`.
- ESP32 `MtAdapter::maybeBroadcastNodeInfoAfterPeerAnnouncement(...)` and nRF
  `MeshtasticRadioAdapter::maybeBroadcastNodeInfoAfterPeerAnnouncement(...)` now call the same
  policy.
- NodeInfo reply suppression gates now use shared
  `resolveMeshtasticNodeInfoReplyPolicy(...)`.
- nRF observed-NodeInfo timestamps are no longer reused as NodeInfo-reply suppression timestamps;
  `nodeinfo_reply_ms_` now tracks reply suppression separately.
- Platform adapters still own the IO side: building, queueing, logging, and channel choice.

Current state:

- NodeInfo reply packet construction now routes through
  `chat::runtime::MeshtasticSelfAnnouncementCore` on nRF52, ESP32 Arduino, and the ESP-IDF radio shim.
- The shared request carries platform-provided user-id override and air ACK intent so ESP32 APRS callsign
  override and unicast NodeInfo ACK behavior remain explicit inputs instead of hidden adapter branches.
- Platform adapters still own radio IO, channel availability, and when to transmit the resulting wire packet.

Residual risk:

- Position local availability and direct send still live in platform adapters, even though
  the reply gate and nRF state separation now match the intended concepts.

### MT-002 Broadcast `want_response`

Evidence:

- ESP32 `MtAdapter::sendAppData()` computes `effective_want_response = want_response || want_ack`
  and does not clear it only because the destination is broadcast.
- nRF `MeshtasticRadioAdapter::sendAppData()` currently clears both `want_ack` and `want_response`
  when `dest == 0 || dest == broadcast`.

Expected behavior:

- Broadcast air ACK must be disabled.
- Broadcast `want_response` must be governed by Meshtastic semantics and preserved for supported
  request-style broadcasts.

Risk:

- Broadcast request/reply behaviors can silently work on ESP32 and fail on nRF.
- Official Meshtastic Position broadcast can use request-replies semantics.

Current state:

- Shared `resolveMeshtasticAppDataSendPolicy(...)` now makes both adapters preserve
  `want_response` for broadcast while disabling impossible broadcast air ACK tracking.
- `modules/core_chat/tests/test_meshtastic_protocol_policy.cpp` covers the shared policy.

Residual risk:

- The test covers policy intent, not full wire encoding on both adapters.
- Request/reply result handling is still split across platform adapters.

Next action:

- Confirm every current caller that sends broadcast `want_response`.
- Add a small encode-level parity test once adapter-side packet builders are easier to invoke without
  hardware.

### MT-003 Empty Payload Request Ports

Evidence:

- `POSITION_APP` requests and protobuf-empty `TRACEROUTE_APP` are valid request shapes.
- nRF was previously fixed to allow empty app payloads for Meshtastic request-style packets.

Expected behavior:

- Both ESP32 and nRF allow `len == 0` with null payload when the app port semantics allow it.

Current state:

- Appears aligned for Meshtastic after the nRF fix.

Residual risk:

- No shared test asserts this across both adapter implementations.

### MT-003a Position Reply Gate

Evidence:

- ESP32 and nRF both send own position for incoming Meshtastic `POSITION_APP` with
  `want_response` addressed to us or broadcast, subject to a 3m suppression window.

Current state:

- Shared `resolveMeshtasticPositionReplyPolicy(...)` now owns the `want_response`,
  address, and suppression-window decision.
- ESP32 and nRF Position replies now preserve the original request packet id in
  `Data.request_id`, allowing shared action lifecycle tracking to complete only on matching
  Position responses.
- `MeshtasticPositionCore` now owns `meshtastic_Position` protobuf payload construction, including
  lat/lon scaling, altitude/speed rounding, course clamping, satellites, and timestamp validity.
- Platform adapters still own `sendPositionTo(...)`, radio/channel mechanics, and whether a local
  position is available.

Residual risk:

- Position availability policy is not yet shared; adapters still decide whether their local GPS source is
  sufficient for a reply.

### MT-004 TraceRoute Request Semantics

Evidence:

- Official Meshtastic `TraceRouteModule` sends `TRACEROUTE_APP`, `want_response=true`,
  and unicast `want_ack=true`.
- Mono UI was corrected to send unicast TraceRoute with ACK.

Expected behavior:

- UI and adapters must not report local enqueue as TraceRoute success.
- Final result requires TraceRoute response, routing error, or timeout.

Current state:

- Shared `resolveMeshtasticTraceRouteReplyPolicy(...)` now owns whether an incoming
  `TRACEROUTE_APP` packet should produce a reply.
- Shared `updateTraceRoutePayload(...)` now owns decode -> insert unknown hops -> append local
  node/SNR -> re-encode mutation for incoming TraceRoute payloads.
- Shared `MeshtasticAppActionRuntime` now owns outgoing TraceRoute and Position Exchange
  lifecycle interpretation: pending, routing delivered where available, routing error,
  matching app response completed, and timeout.
- Mono UI observes `ChatService::IncomingDataObserver` instead of polling the adapter queue
  directly, so TraceRoute result state does not steal app-data from BLE/other consumers.
- Adapter send mechanics still exist on ESP32 and nRF.

Residual risk:

- Route display details are not yet rendered; the UI currently reports lifecycle status only.
- Position availability checks still live in platform adapters.

### MT-005 PKI Unknown Resync

Evidence:

- ESP32 and nRF both contain paths that send or request NodeInfo after PKI/routing-key failures.

Current state:

- `MeshtasticPkiResyncState` converts PKI and local `NO_CHANNEL` resync causes into protocol
  effects: send NodeInfo, send routing error, and forget stale peer key.
- ESP32 and nRF now call the shared runtime for local PKI-not-ready, missing peer key, stale peer key,
  peer-reported PKI/NO_CHANNEL routing errors, and local channel-mismatch routing replies.
- Platform adapters execute those effects only: choose the channel key/hash, build the NodeInfo or
  Routing packet, forget the stored key, and queue/transmit.

Residual risk:

- PKI resync now uses shared NodeInfo packet construction when it emits `SendNodeInfoEffect`; Position
  availability remains platform-owned.

## MeshCore Drift Items

### MC-001 NodeInfo Query/Reply

Evidence:

- ESP32 and nRF both expose `requestNodeInfo(dest, want_response)` through
  `RequestNodeInfoIntent -> MeshCoreRuntime -> SendNodeInfoEffect`.
- Shared MeshCore codecs build and parse NodeInfo query/info control frames.
- ESP32 and nRF both declare the fine-grained NodeInfo capability fields they now execute.

Expected behavior:

- If a platform claims MeshCore NodeInfo support, it must implement NodeInfo query/info semantics.
- If it does not support it, UI and use cases must not rely on `requestNodeInfo()`.

Current state:

- NodeInfo query/info decision mapping is shared by `MeshCoreRuntime`.
- NodeInfo control payload layout is shared by `meshcore_payload_helpers`.
- ESP32 and nRF differ only in how the resulting `SendNodeInfoEffect` is transmitted and projected.

Residual risk:

- nRF currently has limited projection for received `PublishNodeInfoEffect`; the shared runtime can emit the
  effect, but platform-specific contact persistence remains thinner than ESP32.

### MC-002 Trace

Evidence:

- ESP32 MeshCore adapter parses and forwards direct `PAYLOAD_TYPE_TRACE`.
- Official MeshCore source uses `Mesh::createTrace()` and `onTraceRecv()` for this behavior.
- nRF MeshCore adapter uses the shared trace runtime and sends a minimal one-hop native trace when no full route
  table exists.

Expected behavior:

- MeshCore trace must not be exposed through Meshtastic `TRACEROUTE_APP`.
- UI must hide MeshCore trace until a MeshCore-native request/result interface exists.

Current state:

- Trace base payload build/decode and lifecycle policy are shared.
- ESP32 still owns richer route scheduling and BLE `TraceData` projection.
- nRF declares `supports_trace_route_request` for its minimal native MeshCore trace path, but not richer route
  projection.

### MC-003 App-data ACK

Evidence:

- ESP32 and nRF direct app-data send paths register pending ACK signatures in `MeshCoreRuntime`.
- ESP32 and nRF incoming ACK frames call shared `handleAppAck(...)`.
- Both adapters declare `supports_protocol_ack_tracking` when this runtime path is wired.

Expected behavior:

- `want_ack` must either be fully supported and declared, or treated as unsupported.

Current state:

- ACK pending/completed/timeout lifecycle is shared.

Residual risk:

- ACK frame response scheduling for received want-ack app-data is still platform IO; ESP32 has a fuller peer ACK
  path than nRF.

### MC-004 Direct Routing, Identity, And Secrets

Evidence:

- ESP32 MeshCore adapter maintains peer routes, selected preferred channels, identity/public key material,
  derived direct secrets, path forwarding, and discover responses.
- nRF MeshCore adapter builds simpler no-transport flood frames and does not mirror the full direct routing model.

Expected behavior:

- This may be an intentional nRF simplification, but it must be represented as capability differences.

Current state:

- Confirmed platform gap, now documented in fine-grained capabilities.
- ESP32 direct text/app-data sends now use `MeshCoreDirectRoutePolicy` for the shared decision table:
  missing peer pubkey -> discover/fail, selected route -> direct route path, no selected route -> flood,
  and preferred-channel secret derivation may fall back to the requested channel.

Residual risk:

- Peer route storage, pubkey persistence, direct secret derivation, and frame transmission remain platform-owned.
- nRF still derives identity/peer secrets for request payloads but does not implement the ESP32 direct route table.

## Capability Drift

Current state:

- `MeshCapabilities` now keeps the legacy coarse flags and adds fine-grained protocol flags for NodeInfo,
  Position, TraceRoute, protocol app responses, and ACK tracking.
- ESP32/nRF Meshtastic and MeshCore adapters declare the fine-grained flags they execute.
- Linux loopback/raw-lora adapters and the legacy ESP radio shim were updated to avoid silently omitting the
  new fields.

Residual risk:

- MeshCore direct route tables, identity keys, peer secret derivation, and rich trace route projection now have
  separate capability fields. The remaining debt is policy ownership, not capability visibility.

## Immediate Recommended Fix Order

1. Add parity tests that assert ESP32 and nRF adapters advertise the fine-grained capabilities they actually
   execute.
2. Extract shared Meshtastic Position availability policy.
3. Move MeshCore peer route storage / pubkey persistence effects toward shared runtime state.
4. Move MeshCore direct secret derivation request/response effects behind protocol runtime effects before exposing
   richer MeshCore direct actions through UI.

## Guardrail

Every future protocol adapter PR must update either:

- `docs/specification/PROTOCOL_ADAPTER_PARITY_SPEC.md`, if the expected behavior changes;
- this audit, if a known gap is accepted temporarily;
- or tests proving ESP32 and nRF remain aligned for the touched protocol behavior.
