---
on:
  schedule: daily

permissions:
  contents: read

safe-outputs:
  create-issue:
    title-prefix: "[security] "
    labels: [security, automated]

tools:
  github:
  bash: true
---

# Security Scan

Scan recent code changes in the Trail Mate repository for suspicious or dangerous patterns and create an issue if anything is found.

Trail Mate is a C++ embedded systems firmware project. Focus the scan on code merged in the last 7 days.

**Check for:**

1. **Hardcoded credentials** — API keys, tokens, passwords, or private keys embedded as string literals in source files (not in `.gitignore`d files)
2. **Unexpected network calls** — new HTTP/HTTPS/TCP calls to external services that were not present before, especially in non-network modules
3. **Obfuscated code** — heavily encoded strings, unusual base64 blobs, or code that obscures its intent without explanation
4. **Dependency tampering** — changes to `dependencies.lock` or third-party library files in `third_party/` that modify pinned versions without a corresponding PR description explaining why
5. **Dangerous build scripts** — new scripts in `scripts/` or CI workflow changes that execute arbitrary remote content (e.g., `curl | bash`)
6. **Unsafe memory patterns** — newly introduced `strcpy`, `sprintf`, or unbounded buffer operations in recently changed files

**Output rules:**
- Only create an issue if you find at least one genuine concern
- For each finding, list: the file path, line number, a brief description of the concern, and a severity (Low/Medium/High)
- Do NOT report false positives from test files or example code that is clearly marked as such
- If no issues are found, do not create an issue
