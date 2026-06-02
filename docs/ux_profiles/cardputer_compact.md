# UX Profile: cardputer_compact

## Screen Class

Compact 320 x 170 landscape keyboard device.

## Input Model

Keyboard-first input. Pointer, touch, and trackball are absent in the current
Cardputer Zero board facts.

On the Linux device route, text composition is owned by the user-session Fcitx5
stack. Trail Mate's compact UX accepts committed text from the normal Linux
input frontend path; the Cardputer Zero IME panel socket is display-only and is
not a Trail Mate text submission API.

## Feature Set

Compact chat/status workflow with contacts, compact map, GPS/team actions,
tracking, and settings.

## Screen Set

Dashboard, Chat, Contacts, Compact Map, GPS, Team, Tracker, Settings.

This screen set must stay aligned with both:

- the executable `CardputerCompactUxPack::buildScreens()` list
- the `cardputer_compact_manifest` route manifest

The main menu visual baseline is the Pager compact family. In shared LVGL UI,
`make_cardputer_zero_profile()` starts from `make_pager_profile()` and then
shrinks geometry for the 320 x 170 Cardputer Zero display. Cardputer Zero page
adaptation must preserve that Pager-derived visual language unless a new UX
profile decision explicitly changes it.

## Map Mode

Compact 320 x 170 landscape map with offline tiles and current-position focus.

## Chat Mode

Quick message chat list and compose optimized for keyboard entry.

## Team Action Mode

Quick location plus compact command/status actions.

## GPS Mode

Compact GPS status with route/tracker surfaces deferred.

## Modal/Picker Strategy

Keyboard-first compact modals and list pickers.

## Renderer Family

Executable UX pack `cardputer_compact`; the shared LVGL menu profile already
has a Cardputer Zero target profile derived from the Pager profile. The current
Linux device-shell build slice validates target identity, UX selection, page
manifest alignment, notification/Fcitx5 boundaries, and the Pager-derived menu
profile. Real framebuffer rendering and page screenshots remain pending until
the Cardputer Zero device renderer is wired and validated.

## Deferred Decisions

Real-device keyboard sampling, framebuffer handoff, per-page 320 x 170
screenshot capture, notification daemon session validation, Fcitx5 session
validation, launch, and packaging remain deferred.

Board describes. Target chooses. UX Pack presents. Renderer draws.
