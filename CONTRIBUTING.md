# Contributing to R-Type

Thank you for your interest in R-Type. This document explains how to set up the project, report bugs, propose changes and follow the project's conventions.

## Table of Contents

- [Ways to Contribute](#ways-to-contribute)
- [Reporting a Bug](#reporting-a-bug)
- [Suggesting a Feature](#suggesting-a-feature)
- [Development Setup](#development-setup)
- [Project Areas](#project-areas)
- [Branching Model](#branching-model)
- [Commit Convention](#commit-convention)
- [Coding Style](#coding-style)
- [Network Protocol Changes](#network-protocol-changes)
- [Cross-Platform Requirements](#cross-platform-requirements)
- [Testing](#testing)
- [Pull Requests](#pull-requests)
- [Code Review](#code-review)
- [Contact](#contact)

## Ways to Contribute

- Report a bug
- Suggest a new feature or gameplay improvement
- Fix a bug or implement a feature listed in the issues
- Improve the documentation (README, protocol specification, code comments)

Before starting any significant work, open an issue to discuss it with the team.

## Reporting a Bug

Check the [issues](https://github.com/EpitechPGE3-2026/G-ING-500-NCY-5-1-rtype-3/issues) first to make sure the bug has not already been reported.

A good bug report includes:

- **Title:** a short and precise description of the problem
- **Affected part:** client, server, or both
- **Steps to reproduce:**
  1. Start the server with `./r-type_server [port]`
  2. Start the client with `./r-type_client [arguments]`
  3. [What you did in game]
- **Expected behavior:** what should happen
- **Actual behavior:** what actually happens
- **Environment:** OS (Windows or Linux and version), compiler and version, commit hash
- **Logs or screenshots:** server output, client output, or a capture of the issue

For network issues (desynchronization, lost connection, wrong positions), please specify whether the client and server were on the same machine, on a local network, or over the internet.

## Suggesting a Feature

Open an issue with the `enhancement` label and describe:

- the need or problem it solves
- the proposed solution
- the parts of the project affected (ECS engine, server, protocol)
- alternatives you considered

# Project Areas

| Directory | Content | Maintainer |
|-----------|---------|------------|
| `client/src/` | ECS engine, rendering, game logic | Etan Vazquez |
| `server/src/` | Networking, lobbies, synchronization | Pierre Maciejewski |
| `common/` | Code shared by client and server (protocol, sockets) | Both |

Changes to `common/` affect both the client and the server, so they must be reviewed by both maintainers.

## Branching Model

| Branch | Purpose |
|--------|---------|
| `main` | Stable version, never push directly to it |
| `server/<name>` | Server changes |
| `client/<name>` | Client changes |
| `documentation` | Documentation changes |

Always create your branch from `[main]`:

## Commit Convention

```
<type> <scope>: <short description>
```

**Types:**

| Type | Usage |
|------|-------|
| `add` | New feature |
| `fix` | Bug fix |
| `refactor` | Code restructuring without behavior change |
| `test` | Adding or updating tests |

Examples:

```
feat(ecs): add collision system
fix(server): handle client timeout without Disconnect packet
feat(protocol): add Shoot packet type
docs: update protocol.md with LobbyJoin payload
build: add Windows support in CMakeLists.txt
```

## Network Protocol Changes

The protocol is documented in [PROTOCOL.md](PROTOCOL.md). Any change to the binary protocol must:

1. Update `PROTOCOL.md` in the same pull request (packet type, payload layout, example).
2. Be implemented on both the client and the server, so that neither side is left with an outdated version.
3. Add new packet types at the end of the `PacketType` enum, never reuse or renumber an existing value.
4. Update the type range check in `decode()`.
5. Keep all multi-byte values in network byte order (`htons` / `ntohs`).
6. Respect the maximum packet size (`MAX_PACKET_SIZE`, 1024 bytes header included).

## Cross-Platform Requirements

R-Type must build and run on both **Windows** and **Linux**.

- Do not use platform-specific APIs outside of the dedicated abstraction (e.g. `Socket.hpp`).
- If platform-specific code is unavoidable, isolate it with `#ifdef _WIN32` and provide both implementations.
- Use `std::filesystem` for paths instead of hardcoded `/` or `\` separators.
- Test your changes on both platforms when possible, or mention in the pull request which platform was tested.

Before opening a pull request, at minimum:

- the project builds without errors or warnings
- the server starts and accepts connections
- two clients can connect to the same lobby and see each other move

## Pull Requests

1. Make sure your branch is up to date with `[dev]`.
2. Make sure the project builds and works as described in [Testing](#testing).
3. Open a pull request to `[dev]` with:
   - a clear title following the commit convention
   - a description of what changed and why
   - a link to the related issue (e.g. `Closes #12`)
   - screenshots or a short video if the change is visual

### Checklist

- [ ] The project builds without errors or warnings
- [ ] Tested on Linux
- [ ] Tested on Windows
- [ ] The code follows the project's coding style
- [ ] `protocol.md` is updated if the protocol changed
- [ ] The README is updated if usage or build steps changed

## Code Review

- Each pull request must be approved by at least [1] reviewer.
- Changes to `common/` or to the protocol must be approved by both maintainers.
- All review comments must be addressed or discussed before merging.
- Merge method: [Squash and merge].

## Contact

- **Etan Vazquez** (ECS engine): [email or GitHub profile]
- **Pierre Maciejewski** (Server): [email or GitHub profile]