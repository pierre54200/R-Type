# R-Type Network Protocol

## Table of Contents

- [Purpose](#purpose)
- [Overview](#overview)
- [Transport](#transport)
- [Conventions](#conventions)
- [Packet Format](#packet-format)
- [Packet Types](#packet-types)
- [Packet Details](#packet-details)
- [Validation on Reception](#validation-on-reception)
- [Communication Flow](#communication-flow)
- [Examples](#examples)
- [Known Limitations](#known-limitations)
- [Changelog](#changelog)

## Purpose

This document describes the binary protocol used between the R-Type client (a C++ ECS engine handling the game's physics, rendering and features) and the R-Type server (a C++ UDP server handling connections, lobbies and player synchronization).

It allows anyone to understand or reimplement the network communication without reading the source code.

## Overview

```
+----------+                         +----------+
|  Client  |  <---- UDP packets ---> |  Server  |
+----------+                         +----------+
                                          ^
+----------+                              |
|  Client  |  <---- UDP packets ----------+
+----------+
```

The server receives packets from each client, manages lobbies and forwards the relevant information (positions, messages) to the other players.

## Transport

| Parameter | Value |
|-----------|-------|
| Transport protocol | UDP |
| Default port | 4242 |
| Maximum packet size | 1024 bytes (header included) |
| Header size | 3 bytes |
| Maximum payload size | 1021 bytes |
| Packets per datagram | 1 (each UDP datagram contains exactly one packet) |

UDP guarantees neither delivery, ordering nor uniqueness of datagrams. The protocol currently does not handle any of these (see [Known Limitations](#known-limitations)).

## Conventions

- All multi-byte numeric values are encoded in **big-endian** (network byte order), using `htons` when sending and `ntohs` when receiving.
- Strings are **not** null-terminated. Their length is deduced from the payload size.
- Offsets in this document start at 0.

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
| 1 | 2 bytes | `uint16_t` (big-endian) | `size` | Number of payload bytes following the header |
| 3 | `size` bytes | raw bytes | `payload` | Content specific to the packet type |

Matching structure in the code (`common/`):

```cpp
struct Packet {
    PacketType type;
    std::string data;
};
```

Packets are serialized with `encode()` and parsed with `decode()`.

## Packet Types

| Value | Name | Direction | Description |
|-------|------|-----------|-------------|
| 1 | `Connect` | Client -> Server | Connection request |
| 2 | `Message` | Client -> Server, Server to Client | Text message |
| 3 | `Disconnect` | Client -> Server | Client disconnection |
| 4 | `Position` | Client -> Server, Server -> Client | Player position |
| 5 | `LobbyList` | Client -> Server, Server -> Client | Request and send the list of lobbies |
| 6 | `LobbyCreate` | Client -> Server | Create a lobby |
| 7 | `LobbyJoin` | Client -> Server | Join a lobby |
| 8 | `LobbyLeave` | Client -> Server | Leave a lobby |
| 9 | `Info` | Server -> Client | Information sent by the server |

## Packet Details

### Connect (1)

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Username of the player |

**Expected response :** Welcome message for the player who connected and for anyone else an user connection message

### Message (2)

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `text` | Message content |

**Expected response :** Message is send to other player in the lobby

### Disconnect (3)
No data send for disconnect only the header

**Expected response :** Deconnexion message is send to other player in the lobby

### Position (4)

| Size | Type | Field | Description |
|------|------|-------|-------------|
| 1 byte | `uint8_t` | `id` | Player ID (0 to 255) |
| 2 bytes | `uint16_t` (big-endian) | `x` | Horizontal position (0 to 65535) |
| 2 bytes | `uint16_t` (big-endian) | `y` | Vertical position (0 to 65535) |
| rest of payload | string | `name` | Player name |

**Expected response :** Send to all player in the lobby player position

Matching structure in the code:

```cpp
struct PlayerPosition {
    uint8_t id;
    uint16_t x;
    uint16_t y;
    std::string name;
};
```

Related functions: `makePosition()` builds the packet, `readPosition()` parses it. `readPosition()` rejects any packet whose not the good size

### LobbyList (5)

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | List of all the lobbies avaible |

**Expected response :** Send to the player the list of the avaiable lobbies and how to join them

### LobbyCreate (6)

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Lobby name |

**Expected response :** Send to all player that a new lobby was created

### LobbyJoin (7)

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `name` | Lobby name |

**Expected response :** Send to all player in the lobby that someone join

### LobbyLeave (8)

No data send for lobbyleave only the header

**Expected response :** Send to all player in the lobby that someone leave

### Info (9)

**Direction:** [Server to Client]
**Description:** [To be completed]

| Size | Type | Field | Description |
|------|------|-------|-------------|
| `size` bytes | string | `message` | Message to send to players |

**Expected response :** Server Only so no response

## Validation on Reception

`decode()` rejects a packet (returns `false`) in the following cases:

1. The datagram is shorter than 3 bytes (incomplete header).
2. The total received length does not exactly match `3 + size`. A truncated packet, or one followed by extra bytes, is therefore discarded.
3. The type is outside the valid range (1 to 9).

## Communication Flow

### Connection and joining a game

```
Client                                Server
  |  ------- Connect ------------------>  |
  |  <------ Info ----------------------  |
  |  ------- LobbyList ---------------->  |
  |  <------ LobbyList -----------------  |
  |  ------- LobbyJoin ---------------->  |
  |  <------ Info ----------------------  |
```

### In game

```
Client A              Server               Client B
  |  --- Position --->  |                     |
  |                     |  --- Position --->  |
  |                     |  <--- Position ---  |
  |  <--- Position ---  |                     |
```

### Leaving

```
Client                                Server
  |  ------- LobbyLeave --------------->  |
  |  ------- Disconnect --------------->  |
```