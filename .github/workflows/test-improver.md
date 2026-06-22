---
on:
  schedule: weekly

permissions:
  contents: read
  issues: read
  pull-requests: read

safe-outputs:
  create-pull-request:
    title-prefix: "[tests] "
    labels: [testing, automated]
    allowed-base-branches: [main]

tools:
  github:
  bash: true

---

# Test Improver

Identify under-tested areas of the Trail Mate codebase and open a pull request adding meaningful test cases.

Trail Mate is a C++ embedded systems project. The project uses CMake/CTest for testing. Tests are located in the `builds/` directory and in per-platform app directories. The Linux simulator (`apps/linux_sim_shell/`) is the primary platform for running tests without hardware.

**Analysis steps:**

1. Look at files in `src/` and `apps/linux_sim_shell/` that have no corresponding test file or have very few test cases.
2. Prioritize testing:
   - Pure logic functions (coordinate math, message parsing, data formatting)
   - Functions with multiple branches or error paths
   - Any function that was changed in a recent merged PR
3. Do NOT write tests that require physical hardware (GPS module, LoRa radio, display). Focus on logic that runs on the Linux simulator.

**Test quality requirements:**
- Each test must have a clear name describing what it verifies
- Tests should cover both the happy path and at least one edge/error case
- Use the existing test framework and patterns already in the project
- Tests must compile and pass on the Linux simulator target

Open a single pull request. PR description should list the functions tested and the cases covered.
