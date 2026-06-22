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

Trail Mate is a C++ embedded systems project. The project uses CMake with CTest for execution, with Catch2-style C++ test files in module-local test folders.

Test directory structure to use:
- `modules/*/tests/` — primary unit and component tests (main target)
- `apps/linux_sim_shell/tests/` — Linux simulator-oriented tests when present
- `builds/linux_cmake/` — build and execution entrypoint for Linux-hosted tests (do not add generated artifacts)

**Analysis steps:**

1. Look at files in `modules/` and `src/` that have no corresponding tests under `modules/*/tests/` or have very low coverage by behavior.
2. Prioritize testing:
   - Pure logic functions (coordinate math, message parsing, data formatting)
   - Functions with multiple branches or error paths
   - Any function that was changed in a recent merged PR
3. Do NOT write tests that require physical hardware (GPS module, LoRa radio, display). Focus on logic that runs on the Linux simulator.

**Test quality requirements:**
- Each test must have a clear name describing what it verifies
- Tests should cover both the happy path and at least one edge/error case
- Use the existing test framework and patterns already in the project
- Keep PR scope small: at most 3 source files under test and at most 6 new test cases
- Tests must compile and pass on Linux-hosted CMake/CTest targets

Open a single pull request. PR description should list the functions tested and the cases covered.
