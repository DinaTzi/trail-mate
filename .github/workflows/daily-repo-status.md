---
on:
  schedule: daily

permissions:
  contents: read
  issues: read
  pull-requests: read

safe-outputs:
  create-issue:
    title-prefix: "[repo status] "
    labels: [report, daily-status]
    close-older-issues: true

tools:
  github:
---

# Daily Repo Status Report

Create a daily status report for the Trail Mate repository maintainers.

Trail Mate is a C++ embedded systems project for a low-power, offline-first handheld GPS navigation and LoRa communication device running on ESP32-class hardware. It uses PlatformIO/CMake as the build system and LVGL for the UI.

Include in the report:
- Recent repository activity (new issues opened, issues closed, merged PRs, open PRs awaiting review)
- Summary of recent commits and code changes (mention affected modules such as GPS, LoRa, UI, build system)
- Open blockers or stale issues (issues open more than 14 days without activity)
- Failed or flaky CI runs from the past 24 hours
- Actionable next steps for maintainers
- Links to the relevant issues and PRs

Keep it concise. Use markdown formatting with headers and bullet lists. If there is no meaningful activity in the last 24 hours, still create the report but note it was a quiet day.
