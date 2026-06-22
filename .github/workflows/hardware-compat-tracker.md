---
on:
  schedule: weekly

permissions:
  contents: read
  issues: read
  pull-requests: read
  copilot-requests: write

safe-outputs:
  create-issue:
    title-prefix: "[hardware compat] "
    labels: [report, hardware, automated]
    close-older-issues: true

tools:
  github:
  bash: true
---

# Hardware Compatibility Tracker

Trail Mate supports multiple hardware variants (ESP32 Cardputer, uConsole, CardputerZero, nRF52 node). This workflow checks the state of hardware support across the codebase and creates a weekly compatibility report.

**Steps:**

1. Read `boards/`, `variants/`, `platformio.ini`, and `CMakeLists.txt` to enumerate all supported hardware targets.
2. For each hardware target, check:
   - Does it have a corresponding build workflow in `.github/workflows/`?
   - Are there open issues specifically mentioning this hardware variant?
   - Were there any recent commits or merged PRs that touched this target's files?
   - Does the `README.md` hardware support table accurately reflect what is in the codebase?
3. Look for any recently added hardware support that is missing from documentation.
4. Look for any hardware targets that appear to be broken (failing CI, open P0/P1 issues, no recent build success).

**Report format:**

Create a weekly issue with a table:
| Hardware Target | Build Status | Open Issues | Last Updated | README Listed |
|---|---|---|---|---|

Include a summary of any gaps or concerns. Keep the report concise and factual.
