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
| Meshtastic request/reply core | Partially shared | Medium | NodeInfo/Position reply gating, TraceRoute reply gating, TraceRoute payload mutation, and TraceRoute/Position action lifecycle tracking are shared; actual reply send still lives in adapters/UI. |
| Meshtastic duplicated policy ownership | Partially reduced | High | App-data send intent, NodeInfo reannounce gate, NodeInfo/Position reply gates, TraceRoute reply gate, TraceRoute payload mutation, and TraceRoute/Position result lifecycle are shared; PKI resync policy still sits outside the shared core. |
| MeshCore NodeInfo query/reply | Confirmed drift | High | ESP32 implements MeshCore NodeInfo control frames; nRF `requestNodeInfo()` ignores dest/want_response and sends advert only. |
| MeshCore trace | Confirmed drift | Medium | ESP32 parses/forwards MeshCore `PAYLOAD_TYPE_TRACE`; nRF has no equivalent implementation. UI must not expose MC trace. |
| MeshCore app-data ACK/capability | Confirmed drift | Medium | ESP32 claims and tracks app-data ACK; nRF can set a direct app flag but does not declare or track ACK capability. |
| MeshCore direct routing/identity | Confirmed platform gap | High | ESP32 has richer peer routes, identity, secrets, and direct path behavior; nRF path is simplified. |
| MeshCore duplicated/incomplete ownership | Confirmed architecture debt | Critical | MeshCore protocol truth is split between shared helpers, ESP32 adapter, and nRF simplified adapter. |
| Capability granularity | Confirmed design gap | High | `MeshCapabilities` is too coarse to protect UI/actions from protocol-specific drift. |

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

Residual risk:

- NodeInfo reply packet construction and direct send still live in platform adapters, even though
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
- Platform adapters still own `sendPositionTo(...)` and whether a local position is available.

Residual risk:

- Position payload construction and availability policy are not yet shared.

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
- Position availability checks and payload construction still live in platform adapters.

### MT-005 PKI Unknown Resync

Evidence:

- ESP32 and nRF both contain paths that send or request NodeInfo after PKI/routing-key failures.

Current state:

- The working tree now includes the first shared State-pattern implementation,
  `MeshtasticPkiResyncState`, which converts PKI resync causes into protocol effects:
  send NodeInfo, send routing error, and forget stale peer key.
- Platform adapters have not yet been switched to execute those effects.

Next action:

- Replace ESP32/nRF direct `sendNodeInfoTo/buildAndQueueNodeInfo + sendRoutingError +
  forgetNodePublicKey` call sites with shared runtime effects.
- Keep channel/key selection and actual TX in platform executors.

## MeshCore Drift Items

### MC-001 NodeInfo Query/Reply

Evidence:

- ESP32 `MeshCoreAdapter::sendNodeInfoFrame(...)` builds NodeInfo query/info control frames.
- ESP32 `requestNodeInfo(dest, want_response)` distinguishes broadcast query, unicast query, and info send.
- nRF `MeshCoreRadioAdapter::requestNodeInfo(...)` ignores `dest` and `want_response`, then sends advert only.
- nRF `MeshCapabilities` does not claim `supports_node_info`.

Expected behavior:

- If a platform claims MeshCore NodeInfo support, it must implement NodeInfo query/info semantics.
- If it does not support it, UI and use cases must not rely on `requestNodeInfo()`.

Current state:

- Confirmed drift, partly masked by coarse `IMeshAdapter::requestNodeInfo()`.
- ESP32 contains the richer protocol behavior; nRF does not call a shared MeshCore NodeInfo core.

Next action:

- Either implement MeshCore NodeInfo control frames on nRF, or make unsupported behavior explicit and
  remove any UI path that assumes it works.
- Prefer extracting MeshCore NodeInfo control-frame build/parse/reply policy into shared core first.

### MC-002 Trace

Evidence:

- ESP32 MeshCore adapter parses and forwards direct `PAYLOAD_TYPE_TRACE`.
- Official MeshCore source uses `Mesh::createTrace()` and `onTraceRecv()` for this behavior.
- nRF MeshCore adapter has no trace implementation.

Expected behavior:

- MeshCore trace must not be exposed through Meshtastic `TRACEROUTE_APP`.
- UI must hide MeshCore trace until a MeshCore-native request/result interface exists.

Current state:

- Mono node action spec now hides MC trace.
- nRF remains unsupported.

### MC-003 App-data ACK

Evidence:

- ESP32 MeshCore capabilities include `supports_appdata_ack=true`.
- ESP32 direct app-data tracks ACK state.
- nRF MeshCore direct app payload includes a want-ack flag but capabilities do not claim ACK and there is
  no equivalent tracking/result path in the simplified adapter.

Expected behavior:

- `want_ack` must either be fully supported and declared, or treated as unsupported.

Current state:

- Confirmed drift.

Next action:

- Decide if nRF MeshCore supports ACK tracking. If not, ensure callers do not treat `want_ack` success as delivery.

### MC-004 Direct Routing, Identity, And Secrets

Evidence:

- ESP32 MeshCore adapter maintains peer routes, selected preferred channels, identity/public key material,
  derived direct secrets, path forwarding, and discover responses.
- nRF MeshCore adapter builds simpler no-transport flood frames and does not mirror the full direct routing model.

Expected behavior:

- This may be an intentional nRF simplification, but it must be represented as capability differences.

Current state:

- Confirmed platform gap, not fully documented in capabilities.

Next action:

- Split MeshCore capabilities into discovery, native NodeInfo, ACK tracking, direct route, identity/key,
  and trace support.
- Move route/identity policy toward shared MeshCore runtime core before expanding nRF behavior.

## Capability Drift

The current `MeshCapabilities` fields are too coarse:

- `supports_unicast_appdata` does not say whether app-data ACK, app-level response, empty payload,
  direct route, or protocol-native request/reply are supported.
- `supports_node_info` does not distinguish passive parsing, active query, direct reply, broadcast announce,
  or peer reannounce.
- There is no capability for TraceRoute, Position request/reply, or MeshCore native trace.

This capability gap is the structural reason UI actions drift into unsupported adapters.

## Immediate Recommended Fix Order

1. Land the shared Meshtastic app-data send policy and NodeInfo reannounce gate after build review.
2. Add a compact adapter parity test for Meshtastic app-data encoding intent:
   NodeInfo request, Position request, TraceRoute request, broadcast request where valid.
3. Extract shared Meshtastic reply payload construction/availability, TraceRoute, and PKI resync policy,
   replacing platform copies.
4. Split MeshCore NodeInfo capability and stop using generic `requestNodeInfo()` as if all adapters support it.
5. Add MeshCore capability fields for ACK tracking and trace support.
6. Start MeshCore shared runtime extraction from NodeInfo control frames, because the ESP32 behavior already exists
   and nRF lacks it.
7. Audit PKI unknown/resync paths after the above, because they depend on NodeInfo request semantics.

## Guardrail

Every future protocol adapter PR must update either:

- `docs/specification/PROTOCOL_ADAPTER_PARITY_SPEC.md`, if the expected behavior changes;
- this audit, if a known gap is accepted temporarily;
- or tests proving ESP32 and nRF remain aligned for the touched protocol behavior.
