---
on:
  schedule: weekly

permissions:
  contents: read
  issues: read
  pull-requests: read

safe-outputs:
  create-issue:
    title-prefix: "[contribution check] "
    labels: [report, contribution-review]
    close-older-issues: true

tools:
  github:
---

# Contribution Check

Review all open pull requests in the Trail Mate repository and check them against the project's contribution guidelines.

Trail Mate is a C++ embedded systems project. Key contribution standards to check:

1. **Scope** — PRs should be focused. A PR that mixes unrelated changes (e.g., adds a feature AND refactors unrelated code) needs to be split.
2. **Build system** — Changes to `CMakeLists.txt`, `platformio.ini`, or the `boards/` directory should include a note explaining why.
3. **Naming conventions** — C++ code should use snake_case for variables/functions and PascalCase for classes, consistent with the existing codebase in `src/` and `apps/`.
4. **No hardcoded credentials or API keys** — Any PR adding network calls or external service config should be flagged.
5. **CI passing** — Note any PRs where CI is failing or has not run.
6. **Description quality** — PRs should have a description explaining what changed and why. Flag PRs with empty or one-word descriptions.

For each open PR, list:
- PR number and title
- Which contribution guidelines it meets or violates
- A suggested action (approve, request changes, split, add description)

Create a report issue summarizing findings. If all open PRs are in good shape, note that in the report.
