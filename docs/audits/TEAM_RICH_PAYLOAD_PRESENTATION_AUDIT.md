# Team Rich Payload Presentation Audit

## Purpose

Phase 7.8 establishes presentation ownership for Team rich payload display.

Team location and command sending were already moved behind Team action ownership in Phase 7.2. This audit covers the read/display side only:

```text
TeamChatLogEntry / Team payload -> UI-ready Team display row
```

## Current State

| Question | Answer | Problem |
| --- | --- | --- |
| Current `format_team_chat_entry(...)` location | Removed from `ChatUiController`; previous duplicate legacy formatter decoded Team log entries in the controller | Controller formatting mixed UI coordination with Team payload decoding |
| Current Team chat presentation formatter | `TeamChatPresentationSource` builds `MessageRow` structured rich payload fields plus fallback summaries | Correct owner boundary; decode/format is owned by a named projector |
| Current location decode | `TeamRichPayloadProjector` calls `decodeTeamChatLocation(...)` | Decode is now contained in a presentation projector, not the controller |
| Current command decode | `TeamRichPayloadProjector` calls `decodeTeamChatCommand(...)` | Decode is now contained in a presentation projector, not the controller |
| Current display projection | `TeamRichPayloadProjector` fills `TeamRichPayloadDisplay`; `TeamChatPresentationSource` maps it to `MessageRow::team_rich_payload` | UI consumes formatted projection instead of raw payload |
| Current controller behavior | `ChatUiController` consumes `team_chat_model_.snapshot()` rows | Controller does not decode Team location/command display payloads |
| Current projector allocation policy | Fixed buffers, bounded copies, and `std::snprintf` | Avoids projector-local `std::string` / `std::vector` heap churn in shared embedded UI code |

## Boundary Rule

Team rich payload display is presentation projection.

It is not owned by:

- `ChatUiController`
- renderer local state
- `ChatWorkspaceModel`
- Team action send sinks

## Target Owner

Team rich payload display is owned by:

```text
TeamRichPayloadProjector
TeamChatPresentationSource
```

`TeamRichPayloadProjector` translates legacy Team log entries and payloads into `TeamRichPayloadDisplay`.

`TeamChatPresentationSource` consumes that display DTO and fills UI-ready rows.

## Structured Display Fields

Phase 7.8 introduces structured display fields without requiring a rich card UI:

| Field | Status | Notes |
| --- | --- | --- |
| `kind` | active | identifies text, location, command, or unsupported payload |
| `title` | active | short label for future richer rows |
| `summary` | active | fallback text and compact detail |
| `badge` | active | lightweight display label |
| `location` | active | parsed lat/lon/altitude/icon details |
| `command` | active | parsed command kind, coordinate, radius, and priority |

## MessageRow Projection

Team location and command payloads now carry structured row fields in
`MessageRow::team_rich_payload`.

`MessageRow::text` remains populated as fallback text for legacy or compact
renderers. That fallback is a presentation compatibility detail, not a new Team
domain model and not the only render contract.

## Embedded Hardening

Phase 7.8.1 hardens `TeamRichPayloadProjector` for shared embedded targets:

- no projector-local `std::string` formatting temporaries
- no projector-local `std::vector`
- no `iostream`, `sstream`, or `<format>`
- summary text is built with fixed stack buffers and bounded copies

This is a shared-code baseline decision, not a claim that ESP-IDF itself is limited to C++11.

Target-local code may use a higher standard when its toolchain and binary budget allow it.

## Remaining Legacy

| Surface | Reason | Exit condition |
| --- | --- | --- |
| Rich Team card styling | Structured fields exist, but richer visual cards can still be improved | UX pack renders Team rich rows with richer card styling |
| Team position picker renderer | Send-side picker widget is unrelated to display projection | Picker rendering is extracted from `ChatUiController` |
| Other legacy Team decode surfaces | Contacts/GPS/team-page code still has separate legacy display paths | Later phases migrate those screens to shared Team presentation projection |
| `TeamChatPresentationSource` recent log allocation | Legacy `TeamUiStore` returns `std::vector<TeamChatLogEntry>` | `TeamUiStore` exposes a fixed-capacity recent-log visitor / bounded iterator |

## Phase 7.8 Decision

Phase 7.8 burns down Team rich payload display decoding from `ChatUiController`.

It does not claim every Team payload decode in the repository has been removed. The narrowed guarantee is:

```text
The chat controller no longer formats or decodes Team location/command display payloads, and Team chat rows no longer rely on summary-only `MessageRow` data.
```
