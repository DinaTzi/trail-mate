---
on:
  schedule: weekly

permissions:
  contents: read
  issues: read
  pull-requests: read

safe-outputs:
  create-pull-request:
    title-prefix: "[simplify] "
    labels: [code-quality, automated]
    allowed-base-branches: [main]

tools:
  github:
  bash: true

---

# Code Simplifier

Analyze recently modified C++ source files in the Trail Mate repository and open a pull request with targeted, safe simplifications.

Trail Mate is a C++ embedded systems project. Source code lives in:
- `src/` — core application logic (main entry point)
- `apps/` — platform-specific application modules (esp32_lvgl, linux_sim_shell, linux_uconsole_gtk, linux_cardputer_zero, nrf52_node)
- `modules/` — reusable modules

**Focus on files changed in the last 14 days.** For each file, look for:
1. **Dead code** — functions, variables, or `#ifdef` blocks that are defined but never used
2. **Excessive nesting** — `if/else` chains deeper than 3 levels that can be flattened with early returns
3. **Duplicated logic** — the same code block appearing 2+ times that could be extracted into a shared helper
4. **Magic numbers** — raw integer or float literals that should be named constants

**Constraints:**
- Do NOT change any hardware register access patterns, timing-sensitive code, or interrupt handlers
- Do NOT remove `#ifdef PLATFORM_*` guards — they are intentional for multi-platform support
- Do NOT modify third-party code in `third_party/` or `dependencies.lock`
- Make small, focused changes — prefer 3-5 targeted simplifications over a sweeping rewrite
- Each change must be independently correct; do not introduce regressions

Open a single pull request with all changes. The PR description should list each simplification with a one-line explanation.
