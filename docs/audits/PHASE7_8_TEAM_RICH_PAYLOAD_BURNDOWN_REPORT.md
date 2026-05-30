# Phase 7.8 Team Rich Payload Burn-down Report

## Decision

Phase 7.8 moved Team location/command display decoding out of `ChatUiController` and into Team presentation projection.

The row remains text-compatible for compact renderers, but `MessageRow` now also
carries structured Team rich payload fields:

```text
TeamChatLogEntry -> TeamRichPayloadProjector -> TeamChatPresentationSource -> MessageRow::team_rich_payload
```

## Burned Down

| Surface | Result |
| --- | --- |
| `ChatUiController` direct Team rich payload formatter | removed |
| `format_team_chat_entry(...)` in chat controller | forbidden |
| `decodeTeamChatLocation(...)` in chat controller | forbidden |
| `decodeTeamChatCommand(...)` in chat controller | forbidden |
| `TeamChatLocation` / `TeamChatCommand` display structs in chat controller | forbidden |
| Team chat subtitle/message rich display summary | projected by `TeamRichPayloadProjector` through `TeamChatPresentationSource` |
| `TeamRichPayloadProjector` heap formatting temporaries | removed; summary construction uses fixed buffers and bounded copies |
| `MessageRow` summary-only Team rich limitation | removed; rows now carry `has_team_rich_payload` and `team_rich_payload` |

## Still Contained

| Surface | Reason | Exit condition |
| --- | --- | --- |
| `LegacyTeamActionBridge` | Closed by later Team burn-down; active bridge file/class/test removed | See `LEGACY_BURNDOWN_REGISTER.md` for current owner |
| Team position picker renderer | Picker widget lifecycle is separate from display projection | Resolved by Phase 7.9 with `TeamPositionPickerRenderer` |
| Rich Team card styling | Structured row fields exist; visuals can still become richer | UX pack renders Team rich rows with richer card styling |
| Other Team rich decode surfaces | Contacts/GPS/team screens have separate legacy display paths | Later phases migrate those screens to shared Team projection |
| `TeamChatPresentationSource` vector boundary | Legacy Team store API returns recent logs through `std::vector` | Add a fixed-capacity visitor / bounded iterator API to `TeamUiStore` |

## Checker Changes

- Added Team rich payload presentation audit/spec requirements.
- Required `TeamRichPayloadDisplay` and `TeamRichPayloadProjector`.
- Required projector smoke test.
- Required `TeamChatPresentationSource` to consume `TeamRichPayloadProjector`.
- Required `MessageRow` to carry structured Team rich payload fields.
- Forbid Team rich payload decode/format tokens in `ChatUiController`.
- Forbid send/action/runtime tokens in `TeamRichPayloadProjector`.
- Forbid `std::string`, `std::vector`, `iostream`, `sstream`, and `<format>` in `TeamRichPayloadProjector`.

## Remaining Work

| Work | Direction |
| --- | --- |
| Rich Team card UI | Improve styling using existing structured row fields |
| Team position picker renderer | Resolved by Phase 7.9 without changing Team send ownership |
| Contacts/GPS Team payload formatting | Move those legacy display paths to shared Team presentation projection |
| Team action bridge burn-down | Closed by `TeamActionRuntimeSink` / `TeamActionRequest` |
| Team store recent-log API | Replace vector-return API with fixed-capacity visitor / bounded iterator for embedded targets |

## Later Team Closeout

Subsequent Team burn-down passes removed the active `LegacyTeamActionBridge`,
added structured `MessageRow::team_rich_payload` fields, and moved Team
map/GPS/dashboard position consumption behind `TeamMapOverlaySource`. This
report records the original Phase 7.8 transition; the current Team legacy status
is governed by `LEGACY_BURNDOWN_REGISTER.md` and the Phase 7 runtime ownership
checker.
