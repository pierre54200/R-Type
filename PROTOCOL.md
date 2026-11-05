# R-Type Network Protocol

## Table of Contents

- [Purpose](#purpose)
- [Overview](#overview)
- [Transport](#transport)
- [Conventions](#conventions)
- [Packet Format](#packet-format)
- [Packet Types](#packet-types)
- [Packet Details](#packet-details)
  - [Connection](#connection)
  - [Lobbies](#lobbies)
  - [Game Flow](#game-flow)
  - [In-Game Events](#in-game-events)
  - [Keep-Alive](#keep-alive)
- [Validation on Reception](#validation-on-reception)
- [Communication Flow](#communication-flow)
- [Examples](#examples)
- [Writing a New Client](#writing-a-new-client)
- [Known Limitations](#known-limitations)
- [Changelog](#changelog)

## Purpose

This document specifies the binary protocol used between the R-Type client and the R-Type server.

It is meant to be complete enough for anyone to write a new client (or a new server) without reading the source code. Every packet type, its payload layout, the rules the server applies when receiving it and the responses it produces are described below.

The reference implementation lives in `common/Protocol.hpp`.

## Overview

```
+----------+                         +----------+
|  Client  |  <---- UDP packets ---> |          |
+----------+                         |          |
                                     |  Server  |
+----------+                         |          |
|  Client  |  <---- UDP packets ---> |          |
+----------+                         +----------+
```

Clients connect to the server, browse and join lobbies, mark themselves ready, then play waves together. The server keeps track of connected clients and lobbies, drives the lobby state machine (waiting, playing, intermission), and forwards game events to the other members of the lobby.

## Transport

| Parameter | Value |
|-----------|-------|
| Transport protocol | UDP |
| Default port | 4242 |
| Maximum packet size | 1024 bytes (header included) |
| Header size | 3 bytes |
| Maximum payload size | 1021 bytes |
| Packets per datagram | Exactly 1 |
| Keep-alive interval | 1 second (client to server) |
| Client timeout | 5 seconds without any packet |

UDP guarantees neither delivery, ordering nor uniqueness of datagrams. The protocol does not currently add any reliability layer (see [Known Limitations](#known-limitations)).

A client is identified by the server **by its source address and port**. A client must therefore send all its packets from the same UDP socket for the whole session.

## Conventions

- All multi-byte integers are encoded in **big-endian** (network byte order).
- Signed integers use two's complement.
- Strings are **not** null-terminated. Unless stated otherwise, a string fills the rest of the payload and its length is deduced from the payload size.
- Some packets carry a single ASCII character as a boolean: `'1'` (0x31) means true, `'0'` (0x30) means false.
- Positions are expressed in world pixels. The game world is 4000 x 4000 pixels, with the origin at the top-left corner.
- Human-readable strings sent by the server (`Info`, error messages) are currently written in French.
- Offsets in this document start at 0.
- "Lobby members" means all clients currently in the same lobby as the sender.

## Packet Format

Each packet consists of a 3-byte header followed by the payload.

```
 0        1        2        3                     3 + size
+--------+--------+--------+--------  ...  --------+
|  type  |      size       |        payload        |
+--------+--------+--------+--------  ...  --------+
  1 byte     2 bytes             `size` bytes
```

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 1 byte | `uint8_t` | `type` | Packet type (see [Packet Types](#packet-types)) |
| 1 | 2 bytes | `uint16_t` | `size` | Number of payload bytes following the header |
| 3 | `size` bytes | raw bytes | `payload` | Content specific to the packet type |

Matching structure in the code:

```cpp
struct Packet {
    PacketType type;
    std::string data;   // payload
};
```

Packets are serialized with `encode()` and parsed with `decode()`.

## Packet Types

| Value | Name | Direction | Payload | Summary |
|-------|------|-----------|---------|---------|
| 1 | `Connect` | C -> S | string | Register with a username |
| 2 | `Message` | C -> S, S -> C | string | Chat message inside a lobby |
| 3 | `Disconnect` | C -> S | empty | Leave the server |
| 4 | `Action` | none | none | Reserved, not used |
| 5 | `Position` | C -> S, S -> C | 5 bytes + string | Player position |
| 6 | `LobbyList` | C -> S | empty | Ask for the list of lobbies |
| 7 | `LobbyCreate` | C -> S | string | Create a lobby and enter it |
| 8 | `LobbyJoin` | C -> S | string | Join an existing lobby |
| 9 | `LobbyLeave` | C -> S | empty | Leave the current lobby |
| 10 | `Info` | S -> C | string | Human-readable server notification |
| 11 | `EnemySpawn` | C -> S, S -> C | 5 bytes + string | An enemy appears |
| 12 | `Shoot` | C -> S, S -> C | 10 bytes + string | A player fires a missile |
| 13 | `Ready` | C -> S | 1 char | Ready / not ready |
| 14 | `WaveStart` | S -> C | 1 byte | A wave starts |
| 15 | `WaveClear` | C -> S | empty | The current wave is over |
| 16 | `Intermission` | S -> C | 1 byte | Pause between two waves |
| 17 | `Death` | C -> S | 2 bytes | The sender died |
| 18 | `PlayerDied` | S -> C | 7 bytes | Another player died |
| 19 | `LobbyData` | S -> C | structured list | List of lobbies |
| 20 | `LobbyJoined` | S -> C | string | Confirms the lobby the client is now in |
| 21 | `Host` | S -> C | 1 char | Tells the client whether it is the lobby host |
| 22 | `Ping` | C -> S | empty | Keep-alive |

Values are never reused or renumbered. New types are appended at the end.

## Packet Details

Each section gives the payload layout, the conditions under which the server accepts the packet, and what it sends in response. A packet that does not meet its conditions is silently ignored unless an `Info` response is mentioned.

Any packet other than `Connect` coming from an address that is not registered is ignored.

### Connection

#### Connect (1)

**Direction:** Client to Server

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Username, must not be empty |

**Server behavior:**

- If the address is already registered, or the name is empty: ignored.
- If the name is already used by another client: `Info` with an error message, the client is not registered.
- Otherwise the client is registered, receives a server-assigned player ID, then gets:
  1. `Info` with a welcome message,
  2. `LobbyData` with the current lobby list.

The player ID is never sent on its own. The client learns it from the `Position` packets it receives (see [Position](#position-5)).

#### Disconnect (3)

**Direction:** Client to Server. Empty payload.

**Server behavior:** the client leaves its lobby (same effects as [LobbyLeave](#lobbyleave-9)) and is unregistered.

#### Message (2)

**Direction:** Client to Server, then Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `text` | Message content |

**Server behavior:**

- If the sender is not in a lobby: `Info` with an error message.
- Otherwise every other lobby member receives a `Message` whose payload is `"<sender name> : <text>"`.

#### Info (10)

**Direction:** Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `text` | Notification meant to be displayed to the player |

Used for welcome messages, errors (name taken, lobby not found...), players joining or leaving, ready counters, and crash notifications. Clients should display it but must not parse it: its wording may change.

### Lobbies

A lobby is created by a player and exists as long as at least one player is in it. Each lobby has a state:

| Value | State | Meaning |
|-------|-------|---------|
| 0 | `Waiting` | Players are gathering, no wave played yet |
| 1 | `Playing` | A wave is in progress |
| 2 | `Intermission` | Between two waves, players pick upgrades |

```
             all members Ready             WaveClear
 Waiting  ----------------------> Playing ----------> Intermission
                                     ^                     |
                                     |  all members Ready  |
                                     +---------------------+
```

#### LobbyList (6)

**Direction:** Client to Server. Empty payload.

**Server behavior:** replies with `LobbyData`.

The reference client sends this packet every 2 seconds while the lobby browser is displayed.

#### LobbyData (19)

**Direction:** Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | `uint8_t` | `count` | Number of entries that follow (at most 20) |
| variable | entry[`count`] | `entries` | Lobby entries, sorted by name |

Each entry:

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | `uint8_t` | `nameLength` | Length of the lobby name |
| `nameLength` bytes | string | `name` | Lobby name |
| 1 byte | `uint8_t` | `players` | Number of players in the lobby |
| 1 byte | `uint8_t` | `state` | Lobby state (see table above) |

A client must check that each entry fits in the remaining payload before reading it. `readLobbyData()` rejects the whole packet otherwise.

#### LobbyCreate (7)

**Direction:** Client to Server

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Name of the lobby to create, 1 to 32 bytes |

**Server behavior:**

- Empty name, name longer than 32 bytes, or lobby already existing: `Info` with an error message.
- Otherwise the client leaves its current lobby if any, the lobby is created in the `Waiting` state, and the client receives:
  1. `Info` confirming the creation,
  2. `LobbyJoined` with the lobby name,
  3. `Position` with its own ID and spawn position.

#### LobbyJoin (8)

**Direction:** Client to Server

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Name of the lobby to join |

**Server behavior:**

- Empty name, already in this lobby, lobby not found, or lobby in the `Playing` state: `Info` with an error message.
- Otherwise the client leaves its current lobby if any, then:
  - the joining client receives `Info`, `LobbyJoined`, and one `Position` per lobby member (itself included),
  - every other member receives a `Position` for the newcomer and an `Info` announcing it.

#### LobbyJoined (20)

**Direction:** Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Name of the lobby the client is now in |

Sent after a successful `LobbyCreate` or `LobbyJoin`. The reference client switches to the lobby room screen when receiving it.

#### LobbyLeave (9)

**Direction:** Client to Server. Empty payload.

**Server behavior:** if the client is in a lobby:

1. it receives an `Info` confirming it left,
2. the remaining members receive an `Info` announcing it,
3. if the lobby is now empty, it is deleted,
4. otherwise, if the client was the host, a new host is elected (`Host` sent to every member), and if the lobby is not `Playing`, the ready check runs again (see [Ready](#ready-13)).

### Game Flow

#### Ready (13)

**Direction:** Client to Server

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | char | `ready` | `'1'` ready, `'0'` not ready |

**Server behavior:**

- Ignored if the client is not in a lobby or if the lobby is `Playing`.
- Otherwise the flag is stored and every member receives an `Info` with the counter (`ready/total`).
- When every member is ready:
  1. the lobby switches to `Playing` and its wave number is incremented,
  2. all ready flags are reset,
  3. a host is elected and every member receives `Host`,
  4. every member receives `WaveStart`.

The reference client sends `'1'` when the player clicks the ready button in the lobby, and after picking an upgrade during an intermission.

#### Host (21)

**Direction:** Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | char | `isHost` | `'1'` if the receiver is the lobby host, `'0'` otherwise |

The host is the only client allowed to send `EnemySpawn`. The previous host keeps the role if it is still in the lobby; otherwise the first member found is chosen.

#### WaveStart (14)

**Direction:** Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | `uint8_t` | `wave` | Number of the wave starting (1 for the first one) |

The reference client switches to the game screen on the first wave, then spawns `5 + 3 x wave` enemies during the wave if it is the host.

#### WaveClear (15)

**Direction:** Client to Server. Empty payload.

**Server behavior:** ignored unless the lobby is `Playing`. Otherwise the lobby switches to `Intermission`, every member is marked not ready and alive again, and every member receives `Intermission`.

#### Intermission (16)

**Direction:** Server to Client

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | `uint8_t` | `wave` | Number of the wave that just ended |

The reference client shows the upgrade selection screen, then sends `Ready` once an upgrade is picked. The next `WaveStart` is sent when everyone is ready.

### In-Game Events

#### Position (5)

**Direction:** Client to Server, then Server to Client

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 1 byte | `uint8_t` | `id` | Player ID |
| 1 | 2 bytes | `uint16_t` | `x` | Horizontal position |
| 3 | 2 bytes | `uint16_t` | `y` | Vertical position |
| 5 | rest | string | `name` | Player name |

Minimum payload size: 5 bytes.

**Server behavior (client to server):**

- Ignored if the sender is not in a lobby, is dead, or the payload is shorter than 5 bytes.
- The `id` and `name` sent by the client are ignored and replaced by the server's values.
- `x` and `y` are clamped to the world size (4000).
- The position is forwarded to every other lobby member.

**Server to client:** also sent when a client creates or joins a lobby (see above). A client recognizes its own entry by comparing `name` with its username, which is how it learns its ID. The first `Position` received for an unknown ID means a new player must be displayed.

The reference client sends its position at a fixed rate, and only when it changed.

#### Shoot (12)

**Direction:** Client to Server, then Server to Client

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 1 byte | `uint8_t` | `id` | Shooter ID |
| 1 | 2 bytes | `uint16_t` | `x` | Missile spawn position, horizontal |
| 3 | 2 bytes | `uint16_t` | `y` | Missile spawn position, vertical |
| 5 | 2 bytes | `int16_t` | `vx` | Missile velocity, horizontal (pixels per second) |
| 7 | 2 bytes | `int16_t` | `vy` | Missile velocity, vertical (pixels per second) |
| 9 | 1 byte | `uint8_t` | `damage` | Damage dealt on hit |
| 10 | rest | string | `name` | Shooter name |

Minimum payload size: 10 bytes.

**Server behavior:** ignored if the sender is not in a lobby, is dead, or the payload is too short. Otherwise `id` and `name` are replaced by the sender's values and the packet is forwarded to every other lobby member.

#### EnemySpawn (11)

**Direction:** Client (host) to Server, then Server to Client

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 1 byte | `uint8_t` | `id` | Not used, set to 0 |
| 1 | 2 bytes | `uint16_t` | `x` | Spawn position, horizontal |
| 3 | 2 bytes | `uint16_t` | `y` | Spawn position, vertical |
| 5 | rest | string | `name` | Not used, may be empty |

The layout is the same as `Position`.

**Server behavior:** accepted only if the lobby is `Playing` and the sender is the lobby host. The packet is then forwarded unchanged to every other lobby member, which spawn the enemy locally.

#### Death (17)

**Direction:** Client to Server

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 2 bytes | `uint16_t` | `maxHp` | Maximum health of the player who died |

The payload must be exactly 2 bytes.

**Server behavior:** ignored unless the lobby is `Playing` and the sender is alive. Otherwise the sender is marked dead (its `Position` and `Shoot` packets are ignored until the next intermission), and every other lobby member receives `PlayerDied` with an explosion damage of `max(1, maxHp / 5)`.

#### PlayerDied (18)

**Direction:** Server to Client

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 1 byte | `uint8_t` | `id` | ID of the player who died |
| 1 | 2 bytes | `uint16_t` | `x` | Last known position, horizontal |
| 3 | 2 bytes | `uint16_t` | `y` | Last known position, vertical |
| 5 | 2 bytes | `uint16_t` | `damage` | Explosion damage applied to the receiving player |

The payload must be exactly 7 bytes. The reference client hides the dead player and applies `damage` to the local player.

#### Action (4)

Reserved. Not sent by the reference client and ignored by the server.

### Keep-Alive

#### Ping (22)

**Direction:** Client to Server. Empty payload.

**Server behavior:** no response.

UDP has no notion of connection, so the server cannot detect on its own that a client crashed or lost its network. To handle this:

- **Every** packet received from a registered client (not only `Ping`) refreshes its last activity time.
- A client must send a `Ping` at least every second, whatever screen it is on.
- If the server receives nothing from a client for 5 seconds, it considers the client lost:
  1. the other lobby members receive an `Info` saying the player crashed or lost its connection,
  2. the client leaves its lobby (same effects as [LobbyLeave](#lobbyleave-9)),
  3. the client is unregistered.

With a 1 second interval and a 5 second timeout, about five consecutive pings must be lost before a client is wrongly disconnected.

## Validation on Reception

`decode()` rejects a datagram (returns `false`) when:

1. it is shorter than 3 bytes (incomplete header),
2. its total length is not exactly `3 + size` (truncated packet, or extra bytes after the payload),
3. its type is outside the valid range (1 to 22).

Rejected datagrams are dropped without any response.

Then each payload reader checks its own constraints before reading:

| Function | Rule |
|----------|------|
| `readPosition()`, `readEnemySpawn()` | payload of at least 5 bytes |
| `readShoot()` | payload of at least 10 bytes |
| `readDeath()` | payload of exactly 2 bytes |
| `readPlayerDied()` | payload of exactly 7 bytes |
| `readLobbyData()` | each entry must fit in the remaining payload |

Receive buffers are fixed-size (`MAX_PACKET_SIZE`), so an oversized datagram is truncated by the socket and then rejected by rule 2. On the client side, datagrams that do not come from the server address are ignored.

## Communication Flow

### Connection and lobby

```
Client                                    Server
  |  ------- Connect "Pierre" ------------>  |
  |  <------ Info "Bienvenue ..." ---------  |
  |  <------ LobbyData --------------------  |
  |  ------- LobbyJoin "Salon1" ---------->  |
  |  <------ Info -------------------------  |
  |  <------ LobbyJoined "Salon1" ---------  |
  |  <------ Position (one per member) ----  |
  |                                          |
  |  ------- Ping (every second) --------->  |
```

### Starting a wave

```
Client A (host)          Server               Client B
  |  --- Ready '1' --->     |                     |
  |  <-- Info 1/2 -----     | --- Info 1/2 ---->  |
  |                         |  <--- Ready '1' --  |
  |  <-- Info 2/2 -----     | --- Info 2/2 ---->  |
  |  <-- Host '1' -----     | --- Host '0' ---->  |
  |  <-- WaveStart 1 --     | --- WaveStart 1 ->  |
```

### In game

```
Client A (host)          Server               Client B
  |  --- Position --->      |                     |
  |                         | --- Position ---->  |
  |  --- EnemySpawn ->      |                     |
  |                         | --- EnemySpawn -->  |
  |                         |  <--- Shoot ------  |
  |  <-- Shoot -------      |                     |
  |                         |  <--- Death ------  |
  |  <-- PlayerDied --      |                     |
  |  --- WaveClear -->      |                     |
  |  <-- Intermission       | --- Intermission->  |
```

### Leaving and crashing

```
Client A                 Server               Client B
  |  --- LobbyLeave -->     |                     |
  |  <-- Info ---------     | --- Info -------->  |
  |  --- Disconnect -->     |                     |

  (Client A crashes, no packet for 5 seconds)
                            | --- Info -------->  |
                            |  (A removed)        |
```

## Examples

All bytes are in hexadecimal.

**Connect with the name "Bob"**

```
01 00 03 42 6F 62
|  |     +------- "Bob"
|  +------------- size = 3
+---------------- type = Connect
```

**Ping**

```
16 00 00
|  +---- size = 0
+------- type = Ping (22)
```

**Position of player 2 at (300, 150), name "Bob"**

```
05 00 08 02 01 2C 00 96 42 6F 62
|  |     |  |     |     +------- name = "Bob"
|  |     |  |     +------------- y = 0x0096 = 150
|  |     |  +------------------- x = 0x012C = 300
|  |     +---------------------- id = 2
|  +---------------------------- size = 8
+------------------------------- type = Position
```

**Shoot from player 1 at (100, 200), velocity (+600, -300), 1 damage, name "A"**

```
0C 00 0B 01 00 64 00 C8 02 58 FE D4 01 41
|  |     |  |     |     |     |     |  +- name = "A"
|  |     |  |     |     |     |     +---- damage = 1
|  |     |  |     |     |     +---------- vy = 0xFED4 = -300
|  |     |  |     |     +---------------- vx = 0x0258 = 600
|  |     |  |     +---------------------- y = 200
|  |     |  +---------------------------- x = 100
|  |     +------------------------------- id = 1
|  +------------------------------------- size = 11
+---------------------------------------- type = Shoot
```

**LobbyData with one lobby "Salon1", 2 players, Waiting**

```
13 00 0A 01 06 53 61 6C 6F 6E 31 02 00
|  |     |  |  |                 |  +- state = Waiting
|  |     |  |  |                 +---- players = 2
|  |     |  |  +---------------------- name = "Salon1"
|  |     |  +------------------------- nameLength = 6
|  |     +---------------------------- count = 1
|  +---------------------------------- size = 10
+------------------------------------- type = LobbyData (19)
```

## Writing a New Client

A minimal client must:

1. open a UDP socket and keep using it for the whole session,
2. send `Connect` with a unique name,
3. send `Ping` every second, from the start and until it quits,
4. handle `Info` and `LobbyData`, then send `LobbyCreate` or `LobbyJoin`,
5. learn its ID from the `Position` packet carrying its own name,
6. send `Ready` to start, then `Position` and `Shoot` during waves,
7. render the other players from `Position`, `Shoot`, `EnemySpawn` and `PlayerDied`,
8. send `Disconnect` when quitting.

It must also ignore any packet type it does not know, so that new types can be added without breaking it.

## Known Limitations

- **No reliability layer.** A lost `Ready`, `WaveStart`, `WaveClear` or `Intermission` can block a lobby. Sequence numbers and acknowledgements are planned.
- **Client-side authority.** Enemy spawns, deaths and wave completion are currently decided by clients and trusted by the server. Moving this logic to the server is in progress; the related packets will change accordingly.
- **No authentication.** A client is identified only by its address and port.
- **Player IDs are 8-bit.** IDs wrap around after 255 connections.
- **Server messages are in French** and are meant for display only.

## Changelog

| Version | Changes |
|---------|---------|
| 0.3 | Added `Ping` (22) and the client timeout. Full rewrite of this document to cover every packet type. |
| 0.2 | Added `EnemySpawn`, `Shoot`, `Ready`, `WaveStart`, `WaveClear`, `Intermission`, `Death`, `PlayerDied`, `LobbyData`, `LobbyJoined`, `Host`. `Action` is no longer used. |
| 0.1 | Initial protocol: connection, chat, positions, lobbies. |