---
on:
  schedule: daily

permissions:
  contents: read
  issues: read
  pull-requests: read
  copilot-requests: write

safe-outputs:
  create-pull-request:
    title-prefix: "[docs] "
    labels: [documentation, automated]
    allowed-base-branches: [main]

tools:
  github:
  bash: true
---

# Documentation Updater

Check for recent code changes in merged pull requests and open a pull request updating relevant documentation to reflect those changes.

Trail Mate is a C++ embedded systems project. Documentation files are:
- `README.md` and `README_CN.md` — main project overview, features list, hardware support table
- `docs/` — additional documentation files
- `CHANGELOG.md` — version history
- Inline code comments in `src/` and `apps/` (these are documentation too)

**Steps:**

1. Look at pull requests merged in the last 7 days.
2. For each merged PR, identify if it:
   - Added or changed a user-visible feature (update README features section)
   - Added support for new hardware (update hardware support table in README)
   - Changed build instructions or dependencies (update setup/build docs)
   - Fixed a notable bug (consider adding to CHANGELOG.md if not already there)
   - Changed an API or public interface (update any relevant doc comments)
3. Update only the documentation that is genuinely out of date. Do not rewrite sections that are still accurate.
4. Do NOT translate changes to `README_CN.md` — leave a TODO comment for human translators instead.

Open a single pull request with all documentation updates. The PR description should list each merged PR that prompted a documentation change and what was updated.

If no documentation updates are needed (all recent changes are already reflected), do not open a PR.
