# Contributing to R-Type

Thank you for your interest in R-Type. This document explains how to set up the project, report bugs, propose changes, and follow the team's conventions.

## Table of Contents

- [Ways to Contribute](#ways-to-contribute)
- [Reporting a Bug](#reporting-a-bug)
- [Suggesting a Feature](#suggesting-a-feature)
- [Development Setup](#development-setup)
- [Project Areas](#project-areas)
- [Branching Model](#branching-model)
- [Commit Convention](#commit-convention)
- [Coding Style](#coding-style)
- [Adding an ECS Component or System](#adding-an-ecs-component-or-system)
- [Network Protocol Changes](#network-protocol-changes)
- [Cross-Platform Requirements](#cross-platform-requirements)
- [Testing](#testing)
- [Pull Requests](#pull-requests)
- [Code Review](#code-review)
- [Releases and Tags](#releases-and-tags)
- [Contact](#contact)

## Ways to Contribute

- Report a bug
- Suggest a new feature or gameplay improvement
- Fix a bug or implement a feature listed in the issues
- Improve the documentation (README, protocol specification, code comments)

Before starting any significant work, open an issue to discuss it with the team.

## Reporting a Bug

Check the [issues](https://github.com/EpitechPGE3-2026/G-ING-500-NCY-5-1-rtype-3/issues) first to make sure the bug has not already been reported, then open a new issue with the `bug` label.

A good bug report includes:

- **Title:** a short and precise description of the problem
- **Affected part:** client, server, or both
- **Steps to reproduce:**
  1. Start the server with `./r-type_server [port]`
  2. Start the client with `./r-type_client [arguments]`
  3. What you did in game
- **Expected behavior:** what should happen
- **Actual behavior:** what actually happens
- **Environment:** OS and version, compiler and version, commit hash
- **Logs or screenshots:** server output, client output, or a capture of the issue

For network issues (desynchronization, lost connection, wrong positions), also specify whether the client and server were on the same machine, on a local network, or over the internet.

## Suggesting a Feature

Open an issue with the `enhancement` label and describe:

- the need or problem it solves
- the proposed solution
- the parts of the project affected (ECS engine, server, protocol)
- the alternatives you considered

## Development Setup

1. Install the requirements listed in the [README](README.md#requirements).
2. Clone the repository and build in debug mode:

   ```bash
   git clone git@github.com:EpitechPGE3-2026/G-ING-500-NCY-5-1-rtype-3.git R-Type
   cd R-Type
   cmake -B build -DCMAKE_BUILD_TYPE=Debug -DRTYPE_BUILD_TESTS=ON
   cmake --build build --parallel
   ```

3. Optionally, share the dependency cache between build directories so SFML and GoogleTest are not downloaded again:

   ```bash
   export CPM_SOURCE_CACHE=$HOME/.cache/CPM
   ```

`CMAKE_EXPORT_COMPILE_COMMANDS` is enabled, so `build/compile_commands.json` can be used by clangd or your IDE for completion and diagnostics.

## Project Areas

| Directory | Content | Maintainer |
|-----------|---------|------------|
| `client/src/` | ECS components and systems, rendering, menus, client game logic | Etan Vazquez |
| `server/src/` | Client management, lobbies, wave state machine, timeouts | Pierre Maciejewski |
| `common/` | Shared code: ECS core (`Registery`, managers), protocol, sockets | Both |
| `tests/` | Unit tests | Both |
| `.github/workflows/` | CI | Pierre Maciejewski |

Changes to `common/` affect both the client and the server, so they must be reviewed by both maintainers.

## Branching Model

| Branch | Purpose |
|--------|---------|
| `main` | Stable, always buildable. Never push directly to it. |
| `Feature-<name>` | New feature (e.g. `Feature-lobby`) |
| `Fix-<name>` | Bug fix (e.g. `Fix-client-timeout`) |
| `<topic>` | Tooling or documentation work (e.g. `cicd`, `unit-test`, `documentation`) |

Always create your branch from an up-to-date `main`:

```bash
git switch main
git pull
git switch -c Feature-<name>
```

Keep branches short-lived and focused on a single topic.

## Commit Convention

```
<Type> <scope>: <short description>
```

The description is written in English, in the imperative mood, without a final period. The scope is the part of the project affected (`server`, `client`, `protocol`, `ecs`, `CMakeLists`, `workflows`...).

| Type | Usage |
|------|-------|
| `Add` | New feature, file or test |
| `Fix` | Bug fix |
| `Remove` | Deletion of code or files |
| `Refactor` | Code restructuring without behavior change |
| `Test` | Adding or updating tests |
| `Docs` | Documentation only |

Examples:

```
Add ecs: collision system
Fix server: disconnect clients that stop sending packets
Add protocol: Ping packet type for client timeout
Remove protocol: unused Action packet type
Docs protocol: document LobbyData payload
Fix CMakeLists: link ws2_32 on Windows
```

One commit should contain one logical change. Do not mix formatting changes with functional changes.

## Coding Style

- **Standard:** C++20.
- **Indentation:** 4 spaces, no tabs.
- **Headers:** `.hpp`, protected with `#pragma once`. Sources: `.cpp`.
- **Naming:**

  | Element | Convention | Example |
  |---------|-----------|---------|
  | Classes, structs, enums | PascalCase | `NetworkClient`, `PacketType` |
  | Functions and methods | camelCase | `readPosition()`, `checkTimeouts()` |
  | Local variables and parameters | camelCase | `fromLength` |
  | Private members | `_` prefix + camelCase | `_clients`, `_nextId` |
  | Constants | UPPER_SNAKE_CASE | `MAX_PACKET_SIZE` |

- **Includes:** project headers first, then standard library headers.
- **Errors:** throw exceptions for fatal errors at startup; `main()` catches them and returns `84`. Never let a malformed packet throw or crash: reject it and continue.
- **Code and comments** in English. Messages displayed to players may be localized.
- **Warnings:** the project builds with `-Wall -Wextra` (`/W4` with MSVC). New code must not add warnings.

## Adding an ECS Component or System

The engine separates data (components, called modules, in `client/src/ECS/Modules/`) from behavior (systems, in `client/src/ECS/System/`).

**Adding a component:**

1. Create a plain struct in `client/src/ECS/Modules/` with only data and default values.
2. Register it in `client/src/main.cpp` with `.registerModule<MyModule>()`.

**Adding a system:**

1. Create a class inheriting from `ECS::System` in `client/src/ECS/System/` and implement `update(ECS::Registery&, float dt)`.
2. Use `dt` (seconds since the last frame) for anything time-based, never the frame count.
3. Register it in `client/src/main.cpp` with the components it requires:

   ```cpp
   reg.registerSystem<MySystem>(reg.signatureOf<Transform, RigidBody>());
   ```

   A system that does not work on entities uses an empty signature: `reg.signatureOf<>()`.

4. Systems run in their registration order, every frame. Place yours accordingly (input, then logic, then network, then rendering).

A system must only touch the components it declares. To communicate with another system, use components, the event queue, or a callback set from `main.cpp`, not a direct reference to the other system.

## Network Protocol Changes

The protocol is specified in [PROTOCOL.md](PROTOCOL.md). Any change to the binary protocol must:

1. Update `PROTOCOL.md` in the same pull request: packet table, payload layout, server behavior, an example, and the changelog.
2. Be implemented on both the client and the server, so neither side is left with an outdated version.
3. Append new packet types at the end of the `PacketType` enum. Never reuse or renumber an existing value.
4. Update the type range check in `decode()`.
5. Keep all multi-byte values in network byte order (`htons` / `ntohs`).
6. Respect the maximum packet size (`MAX_PACKET_SIZE`, 1024 bytes header included).
7. Provide a `make...()` / `read...()` pair in `common/Protocol.hpp`, where `read...()` checks the payload size before reading anything.
8. Add a test in `tests/test_protocol.cpp` covering both a valid packet and a malformed one.

## Cross-Platform Requirements

R-Type must build and run on both **Windows** and **Linux**, and clients on both systems must be able to play together.

- Do not use platform-specific APIs outside of the dedicated abstraction (`common/Socket.hpp`).
- If platform-specific code is unavoidable, isolate it with `#ifdef _WIN32` and provide both implementations.
- Use `std::filesystem` for paths instead of hardcoded `/` or `\` separators.
- Use fixed-size integer types (`uint16_t`...) for anything sent over the network.
- The CI builds both targets on both systems. A pull request with a failing CI job will not be merged.

## Testing

Unit tests use GoogleTest and live in `tests/`.

```bash
cmake -B build -DRTYPE_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Or `npm test`.

When adding a test file, add it to the `r-type_tests` sources in `tests/CMakeLists.txt`.

Before opening a pull request, at minimum check that:

- the project builds without errors or new warnings,
- all unit tests pass,
- the server starts and accepts connections,
- two clients can join the same lobby, start a wave and see each other move,
- killing a client (`kill -9` on Linux, end task on Windows) makes the server remove it after the timeout, and the other client is notified.

For network changes, also test under degraded conditions (packet loss, latency) with `netem` on Linux or `clumsy` on Windows.

## Pull Requests

1. Make sure your branch is up to date with `main`.
2. Make sure the project builds and works as described in [Testing](#testing).
3. Open a pull request to `main` with:
   - a clear title following the commit convention,
   - a description of what changed and why,
   - a link to the related issue (e.g. `Closes #12`),
   - screenshots or a short video if the change is visual.

### Checklist

- [ ] The project builds without errors or new warnings
- [ ] Unit tests pass
- [ ] Tested on Linux
- [ ] Tested on Windows (or CI green on Windows)
- [ ] The code follows the [coding style](#coding-style)
- [ ] `PROTOCOL.md` is updated if the protocol changed
- [ ] `README.md` is updated if usage, controls or build steps changed

## Code Review

- Each pull request must be approved by at least one reviewer other than its author.
- Changes to `common/` or to the protocol must be approved by both maintainers.
- All review comments must be addressed or discussed before merging.
- The CI must be green.
- Merge method: merge commit, so the branch history stays visible.
- Delete the branch after merging.

## Releases and Tags

Important milestones are tagged on `main` with an annotated tag:

```bash
git tag -a v1.0 -m "Part 1 delivery: playable prototype"
git push origin v1.0
```

| Tag | Milestone |
|-----|-----------|
| `v1.x` | Part 1: architecture and playable prototype |
| `v2.x` | Part 2: advanced features |

## Contact

| Name | Area | Contact |
|------|------|---------|
| Etan Vazquez | ECS engine, client | etan.vazquez@epitech.eu |
| Pierre Maciejewski | Server, network, CI | pierre.maciejewski@epitech.eu |