# Contributing to Trail Mate

Thank you for your interest in contributing to Trail Mate!

## Getting Started

1. Fork the repository and clone your fork.
2. Follow the build instructions in `README.md` to set up the Linux simulator environment.
3. Make your changes on a feature branch (not `main`).
4. Open a pull request with a clear description of what changed and why.

## Code Style

- C++ code uses **snake_case** for variables and functions, **PascalCase** for classes and structs.
- Keep files focused — one module per file.
- Avoid magic numbers; use named constants.
- Do not add `#include` for headers that are not needed.

## Pull Request Guidelines

- Keep PRs small and focused. One logical change per PR.
- PRs that mix unrelated changes will be asked to be split.
- Include a description explaining what changed and why (not just what the diff shows).
- Make sure CI passes before requesting review.
- Changes to `CMakeLists.txt`, `platformio.ini`, or `boards/` must include an explanation.

## Hardware Targets

Trail Mate supports multiple hardware variants. If your change affects hardware-specific code:
- Test on the Linux simulator at minimum.
- Note which hardware targets were tested in the PR description.
- Do not break existing hardware targets when adding a new one.

## Security

- Do not hardcode credentials, API keys, or tokens.
- Do not add network calls without discussion in an issue first.
- Do not modify pinned dependency versions in `dependencies.lock` without explanation.

## Issues

- Use the issue tracker for bugs, feature requests, and questions.
- Search for existing issues before opening a new one.
- Include your hardware target, firmware version, and steps to reproduce for bug reports.
