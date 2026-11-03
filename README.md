# R-Type

A multiplayer 2D shoot'em up running on Windows and Linux, developed as a third-year project at Epitech.

## Table of Contents

- [About](#about)
- [Technologies](#technologies)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Build](#build)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Network Protocol](#network-protocol)
- [Contributing](#contributing)
- [Authors](#authors)

## About

The project is split into two parts:

- **Client:** an ECS (Entity Component System) engine in C++ that handles the game's physics, rendering and features.
- **Server:** a UDP server in C++ that manages client connections and lobbies, and keeps players synchronized in-game.

## Technologies

- **C++** for the client, the engine and the server
- **CMake** as the build system
- **SFML** for rendering on the client side
- **UDP sockets** for networking

## Prerequisites

- A C++ compiler (GCC or Clang)
- CMake
- Git

## Installation

Clone the repository:

```bash
git clone git@github.com:EpitechPGE3-2026/G-ING-500-NCY-5-1-rtype-3.git R-Type
cd R-Type
```

## Build

### Linux

```bash
cmake -B build
cmake --build build
```

### Windows

```powershell
cmake -B build
cmake --build build
```

The binaries `r-type_server` and `r-type_client` are generated at the root of the repository.

## Usage

### Start the server

```bash
./r-type_server [port]
```

### Start a client

```bash
./r-type_client [hostname] [port] [username] [lobby]
```

On Windows, use `r-type_server.exe` and `r-type_client.exe`.

All arguments are optional. Default values:

| Argument | Description | Default |
|----------|-------------|---------|
| `hostname` | Server address | `127.0.0.1` |
| `port` | Server port | `4242` |
| `username` | Player name displayed in game | `player` |
| `lobby` | Lobby to join | `default` |

### Example: local game with two players

```bash
# Terminal 1
./r-type_server 4242

# Terminal 2
./r-type_client 127.0.0.1 4242 Pierre Salon1

# Terminal 3
./r-type_client 127.0.0.1 4242 Etan Salon1
```

### Controls

| Key | Action |
|-----|--------|
| [Arrow keys] | [Move] |

## Project Structure

```
R-Type/
├── client/src/       # Client source files (ECS engine, rendering, game logic)
├── server/src/       # Server source files (networking, lobbies, synchronization)
├── common/           # Code shared by client and server (network protocol, sockets)
├── CMakeLists.txt    # Build configuration for client and server
├── protocol.md       # Network protocol specification
├── CONTRIBUTING.md   # Contribution guidelines
└── README.md
```

## Network Protocol

Client and server communicate over UDP with a custom binary protocol. The full specification (packet format, packet types, examples) is available in [protocol.md](protocol.md).

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening an issue or a pull request.

## Authors

- **Etan Vazquez**: ECS engine
- **Pierre Maciejewski**: Server