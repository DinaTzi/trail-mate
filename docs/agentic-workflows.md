# Agentic Workflows

This document catalogs all GitHub Agentic Workflows in this repository. These workflows run as GitHub Actions powered by GitHub Copilot and automate routine repository maintenance tasks.

## Overview

| Workflow | File | Trigger | Output |
|----------|------|---------|--------|
| Daily Repo Status | `daily-repo-status.md` | Schedule (daily) | Issue |
| Issue Triage | `issue-triage.md` | Issue opened | Labels + comment |
| Contribution Check | `contribution-check.md` | Schedule (weekly) | Issue |
| Code Simplifier | `code-simplifier.md` | Schedule (weekly) | Pull Request |
| Test Improver | `test-improver.md` | Schedule (weekly) | Pull Request |
| Documentation Updater | `doc-updater.md` | Schedule (daily) | Pull Request |
| CI Doctor | `ci-doctor.md` | Workflow run failed | PR/issue comment |
| ChatOps: /plan | `chatops-plan.md` | Comment `/plan` | Issue comment |
| ChatOps: /ask | `chatops-ask.md` | Comment `/ask` | Issue comment |
| Security Scan | `security-scan.md` | Schedule (daily) | Issue (if findings) |
| Hardware Compat Tracker | `hardware-compat-tracker.md` | Schedule (weekly) | Issue |

---

## Workflow Details

### Daily Repo Status

**Trigger:** Daily schedule  
**Permissions:** `contents: read`, `issues: read`, `pull-requests: read`  
**Output:** Creates one issue per day with title prefix `[repo status]`, labeled `report, daily-status`. Closes previous status issues.  
**What it does:** Summarizes recent issues, merged PRs, open blockers, failed CI runs, and suggests action items for maintainers.  
**To trigger manually:** Go to Actions → Daily Repo Status → Run workflow.

---

### Issue Triage

**Trigger:** When a new issue is opened  
**Permissions:** `contents: read`, `issues: read`  
**Output:** Adds one type label (`bug`, `feature`, `question`, `documentation`) and one priority label (`P0`–`P3`); posts one comment with triage rationale.  
**What it does:** Automatically classifies new issues so maintainers can prioritize without manual review.  
**To trigger manually:** Open a new test issue.

---

### Contribution Check

**Trigger:** Weekly schedule  
**Permissions:** `contents: read`, `issues: read`, `pull-requests: read`  
**Output:** Creates one issue per week with title prefix `[contribution check]`, labeled `report, contribution-review`. Closes previous reports.  
**What it does:** Reviews all open PRs against contribution guidelines (scope, naming, CI status, description quality).

---

### Code Simplifier

**Trigger:** Weekly schedule  
**Permissions:** `contents: read`, `pull-requests: read`  
**Output:** Opens at most one PR per week with title prefix `[simplify]`, labeled `code-quality, automated`.  
**What it does:** Analyzes recently modified C++ source files in `src/` and `apps/` for dead code, excessive nesting, duplicated logic, and magic numbers. Produces targeted, safe simplifications.  
**To trigger manually:** Go to Actions → Code Simplifier → Run workflow.

---

### Test Improver

**Trigger:** Weekly schedule  
**Permissions:** `contents: read`, `pull-requests: read`  
**Output:** Opens at most one PR per week with title prefix `[tests]`, labeled `testing, automated`.  
**What it does:** Identifies under-tested functions in the Linux simulator target and adds meaningful test cases (pure logic, no hardware dependencies).

---

### Documentation Updater

**Trigger:** Daily schedule  
**Permissions:** `contents: read`, `pull-requests: read`  
**Output:** Opens at most one PR per day with title prefix `[docs]`, labeled `documentation, automated`. Does not open a PR if no updates are needed.  
**What it does:** Checks merged PRs from the last 7 days and updates `README.md`, `docs/`, and `CHANGELOG.md` to reflect code changes.

---

### CI Doctor

**Trigger:** When a CI workflow run completes with a failure  
**Permissions:** `contents: read`, `actions: read`, `issues: read`, `pull-requests: read`  
**Output:** Posts one comment on the associated PR or issue.  
**What it does:** Reads failure logs, identifies root cause (compile error, test failure, dependency issue, flaky test), and suggests a concrete fix.

---

### ChatOps: /plan

**Trigger:** Issue comment containing `/plan`  
**Permissions:** `contents: read`, `issues: read`  
**Output:** Posts one comment on the issue.  
**Usage:** Comment `/plan` on any issue to get a breakdown into 3–7 actionable sub-tasks with file references.  
**Who can use it:** Anyone with comment access (typically maintainers and contributors).

---

### ChatOps: /ask

**Trigger:** Issue comment containing `/ask <question>`  
**Permissions:** `contents: read`, `issues: read`, `pull-requests: read`  
**Output:** Posts one comment on the issue.  
**Usage:** Comment `/ask How does the GPS map rendering work?` to get a repository-grounded answer.  
**Who can use it:** Anyone with comment access.

---

### Security Scan

**Trigger:** Daily schedule  
**Permissions:** `contents: read`  
**Output:** Creates at most one issue per run with title prefix `[security]`, labeled `security, automated`. Only creates an issue if findings exist.  
**What it does:** Scans code merged in the last 7 days for hardcoded credentials, unexpected network calls, obfuscated code, dependency tampering, dangerous build scripts, and unsafe memory patterns.

---

### Hardware Compat Tracker

**Trigger:** Weekly schedule  
**Permissions:** `contents: read`, `issues: read`  
**Output:** Creates one issue per week with title prefix `[hardware compat]`, labeled `report, hardware, automated`. Closes previous reports.  
**What it does:** Checks all hardware targets (Cardputer, uConsole, CardputerZero, nRF52) for build status, open issues, documentation coverage, and recent activity.

---

## Security Model

All agentic workflows follow these constraints:

- **Read-only permissions by default** — no workflow has `write` access to contents or issues directly
- **Safe outputs only** — all write operations (create issue, open PR, post comment) go through the safe-output layer with defined constraints
- **Scoped outputs** — every safe output has title prefixes, label constraints, and maximum counts
- **No secrets in agent scope** — any tokens required for write operations exist only in the post-agent job
- **Prompt injection resistance** — workflows that read user-supplied content (issue bodies, PR descriptions, comments) include instructions to ignore embedded directives

## Required Labels

Before workflows that assign labels will work correctly, ensure these labels exist in the repository:

```
bug, feature, question, documentation
P0, P1, P2, P3
report, daily-status, contribution-review
code-quality, testing, documentation, automated
security, hardware
```

Create them via: Settings → Labels, or use `gh label create <name>`.
