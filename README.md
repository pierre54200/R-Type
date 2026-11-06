# R-Type

A networked, cooperative 2D shoot'em up inspired by the classic R-Type, built in C++ on top of a custom Entity Component System engine. Up to four players join a lobby and fight waves of Bydos together, with a central server keeping everyone in sync.

Developed as a third-year project at Epitech.

## Table of Contents

- [Features](#features)
- [Supported Platforms](#supported-platforms)
- [Requirements](#requirements)
- [Dependencies](#dependencies)
- [Build](#build)
- [Usage](#usage)
- [How to Play](#how-to-play)
- [Tests](#tests)
- [Project Structure](#project-structure)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)
- [Authors](#authors)

## Features

- Client and server communicating over UDP with a custom binary protocol
- Lobby system: create, browse and join games from the client
- Wave-based gameplay with an upgrade choice between waves
- Each player is identified by its own color
- Automatic detection of crashed or disconnected players
- Custom ECS engine with decoupled systems for rendering, input, physics and networking

## Supported Platforms

| Platform | Client | Server |
|----------|--------|--------|
| Linux | Yes | Yes |
| Windows | Yes | Yes |

Both platforms are built on every pull request by the CI (GitHub Actions).

## Requirements

- A C++20 compiler: GCC 11+, Clang 14+ or MSVC (Visual Studio 2022)
- CMake 3.14 or newer
- Git
- An internet connection for the first build (dependencies are downloaded automatically)

On Linux, SFML needs a few system development packages (windowing and OpenGL). On Debian or Ubuntu:

```bash
sudo apt-get install libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
    libxinerama-dev libudev-dev libgl1-mesa-dev libegl1-mesa-dev libfreetype-dev
```

Nothing else needs to be installed on Windows.

## Dependencies

Third-party libraries are fetched and built automatically by [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) at configure time. No library source code is stored in this repository.

| Library | Version | Used by | Purpose |
|---------|---------|---------|---------|
| [SFML](https://www.sfml-dev.org/) | 2.6.1 | Client | Window, rendering, input |
| [GoogleTest](https://github.com/google/googletest) | 1.15.2 | Tests only | Unit tests |

Networking uses the operating system sockets (BSD sockets on Linux, Winsock on Windows), wrapped in `common/Socket.hpp`.

To avoid downloading dependencies again for every build directory, you can set a shared cache:

```bash
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
```

## Build

The same commands work on Linux and Windows:

```bash
git clone git@github.com:EpitechPGE3-2026/G-ING-500-NCY-5-1-rtype-3.git R-Type
cd R-Type
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The binaries `r-type_server` and `r-type_client` (`.exe` on Windows) are placed at the root of the repository.

If you have `npm` installed, shortcuts are available:

```bash
npm run build     # configure and build
npm run server    # start a server on port 4242
npm run client    # start a client named Player1
npm run client2   # start a second client named Player2
npm run clean     # remove the build directory and binaries
```

## Usage

### Start the server

```bash
./r-type_server [port]
```

| Argument | Description | Default |
|----------|-------------|---------|
| `port` | UDP port to listen on | `4242` |

### Start a client

```bash
./r-type_client [hostname] [port] [username] [lobby]
```

| Argument | Description | Default |
|----------|-------------|---------|
| `hostname` | Server IPv4 address | `127.0.0.1` |
| `port` | Server port | `4242` |
| `username` | Player name, must be unique on the server | random `PlayerXXXX` |
| `lobby` | Lobby to create or join on startup | none (opens the lobby browser) |

### Example: local game with two players

```bash
# Terminal 1
./r-type_server 4242

# Terminal 2
./r-type_client 127.0.0.1 4242 Pierre Room1

# Terminal 3
./r-type_client 127.0.0.1 4242 Etan Room1
```

To play over a local network, start the server on one machine and use its IP address as `hostname` on the others. Make sure the UDP port is open in the firewall.

## How to Play

1. Create a lobby or join one from the lobby browser.
2. Click **Ready**. The first wave starts when every player in the lobby is ready.
3. Move your ship and survive. Your ship fires automatically at the nearest enemy.
4. When a wave is cleared, pick one upgrade among the cards shown. The next wave starts once everyone has chosen.
5. When a player dies, its ship explodes and damages the other players.

### Controls

| Input | Action |
|-------|--------|
| `W` `A` `S` `D` | Move |
| Mouse left click | Use menus, buttons and upgrade cards |

## Tests

Unit tests use GoogleTest and are disabled by default.

```bash
cmake -B build -DRTYPE_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Or simply `npm test`.

## Project Structure

```
R-Type/
├── client/src/
│   ├── ECS/
│   │   ├── Modules/     # Components (Transform, Health, HitBox, FrontEnd...)
│   │   ├── System/      # Systems (Movement, Collision, Display, Network...)
│   │   └── Utils/       # Entity factories, menus, waves, upgrades
│   ├── Network/         # NetworkClient: UDP connection to the server
│   └── main.cpp         # Registers systems and scenes, then runs the loop
├── server/src/
│   ├── Server/          # Clients, lobbies, wave state machine, timeouts
│   └── main.cpp
├── common/              # Shared code: ECS core, binary protocol, sockets
├── tests/               # Unit tests (GoogleTest)
├── assets/              # Fonts and other resources
├── .github/workflows/   # CI: Linux and Windows builds
├── CMakeLists.txt
├── PROTOCOL.md          # Network protocol specification
├── CONTRIBUTING.md      # Contribution guidelines and conventions
└── README.md
```

## Documentation

- [PROTOCOL.md](PROTOCOL.md): full specification of the binary network protocol, detailed enough to write a new client.
- [CONTRIBUTING.md](CONTRIBUTING.md): development setup, branching model, commit convention, coding style and review process.

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening an issue or a pull request.

## License

No open-source license has been chosen yet. Until then, all rights are reserved by the authors.

## Authors

| Name | Role | Contact |
|------|------|---------|
| Etan Vazquez | ECS engine, client | etan.vazquez@epitech.eu |
| Pierre Maciejewski | Server, network, CI | pierre.maciejewski@epitech.eu |