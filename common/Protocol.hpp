#pragma once

#include "Socket.hpp"
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <vector>

// Protocole binaire
enum class PacketType : uint8_t {
    Connect = 1,
    Message = 2,
    Disconnect = 3,
    Action = 4,
    Position = 5,
    LobbyList = 6,
    LobbyCreate = 7,
    LobbyJoin = 8,
    LobbyLeave = 9,
    Info = 10,
    EnemySpawn = 11,
    Shoot = 12,
    Ready = 13,         // client -> serveur : "1" / "0"
    WaveStart = 14,     // serveur -> clients : 1 octet, numero de la vague
    WaveClear = 15,     // client -> serveur : ma vague est terminee
    Intermission = 16,  // serveur -> clients : 1 octet, vague qui vient de finir
    Death = 17,         // client -> serveur : uint16 PV max
    PlayerDied = 18,    // serveur -> autres : id(1) x(2) y(2) degats(2)
    LobbyData = 19,     // serveur -> client : liste structuree des lobbies
    LobbyJoined = 20,   // serveur -> client : nom du lobby rejoint / cree
    Host = 21,
};

// Donnée envoyé et recu
struct Packet {
    PacketType type;
    std::string data;
};

constexpr std::size_t HEADER_SIZE = 3;
constexpr std::size_t MAX_PACKET_SIZE = 1024;

inline std::vector<uint8_t> encode(const Packet& packet) {
    uint16_t size = htons(static_cast<uint16_t>(packet.data.size()));
    std::vector<uint8_t> buffer(HEADER_SIZE + packet.data.size());
    buffer[0] = static_cast<uint8_t>(packet.type);
    std::memcpy(&buffer[1], &size, sizeof(size));
    std::memcpy(buffer.data() + HEADER_SIZE, packet.data.data(), packet.data.size());
    return buffer;
}

inline bool decode(const uint8_t* buffer, std::size_t length, Packet& packet) {
    if (length < HEADER_SIZE)
        return false;
    uint16_t size;
    std::memcpy(&size, &buffer[1], sizeof(size));
    size = ntohs(size);
    if (length != HEADER_SIZE + size)
        return false;
    if (buffer[0] < 1 || buffer[0] > static_cast<uint8_t>(PacketType::Host))
        return false;
    packet.type = static_cast<PacketType>(buffer[0]);
    packet.data.assign(reinterpret_cast<const char*>(buffer + HEADER_SIZE), size);
    return true;
}

struct PlayerPosition {
    uint8_t id;
    uint16_t x;
    uint16_t y;
    std::string name;
};

inline Packet makePosition(const PlayerPosition& pos) {
    uint16_t x = htons(pos.x);
    uint16_t y = htons(pos.y);
    std::string data(5, '\0');
    data[0] = static_cast<char>(pos.id);
    std::memcpy(&data[1], &x, sizeof(x));
    std::memcpy(&data[3], &y, sizeof(y));
    return {PacketType::Position, data + pos.name};
}

inline bool readPosition(const Packet& packet, PlayerPosition& pos) {
    if (packet.data.size() < 5)
        return false;
    pos.id = static_cast<uint8_t>(packet.data[0]);
    std::memcpy(&pos.x, &packet.data[1], sizeof(pos.x));
    std::memcpy(&pos.y, &packet.data[3], sizeof(pos.y));
    pos.x = ntohs(pos.x);
    pos.y = ntohs(pos.y);
    pos.name = packet.data.substr(5);
    return true;
}

inline Packet makeEnemySpawn(const PlayerPosition& pos) {
    uint16_t x = htons(pos.x);
    uint16_t y = htons(pos.y);
    std::string data(5, '\0');
    data[0] = static_cast<char>(pos.id);
    std::memcpy(&data[1], &x, sizeof(x));
    std::memcpy(&data[3], &y, sizeof(y));
    return {PacketType::EnemySpawn, data + pos.name};
}

inline bool readEnemySpawn(const Packet& packet, PlayerPosition& pos) {
    if (packet.data.size() < 5)
        return false;
    pos.id = static_cast<uint8_t>(packet.data[0]);
    std::memcpy(&pos.x, &packet.data[1], sizeof(pos.x));
    std::memcpy(&pos.y, &packet.data[3], sizeof(pos.y));
    pos.x = ntohs(pos.x);
    pos.y = ntohs(pos.y);
    pos.name = packet.data.substr(5);
    return true;
}

struct Shoot
{
    uint8_t id;
    uint16_t x;
    uint16_t y;
    int16_t dirx;
    int16_t diry;
    std::string name;
    uint8_t damage = 1;
};

inline Packet makeShoot(const Shoot& shoot)
{
    uint16_t x = htons(shoot.x);
    uint16_t y = htons(shoot.y);
    uint16_t dx = htons(static_cast<uint16_t>(shoot.dirx));
    uint16_t dy = htons(static_cast<uint16_t>(shoot.diry));

    std::string data(10, '\0');
    data[0] = static_cast<char>(shoot.id);
    std::memcpy(&data[1], &x, sizeof(x));
    std::memcpy(&data[3], &y, sizeof(y));
    std::memcpy(&data[5], &dx, sizeof(dx));
    std::memcpy(&data[7], &dy, sizeof(dy));
    data[9] = static_cast<char>(shoot.damage);

    return {PacketType::Shoot, data + shoot.name};
}

inline bool readShoot(const Packet& packet, Shoot& shoot)
{
    if (packet.data.size() < 10)
        return false;
    uint16_t x, y, dx, dy;
    shoot.id = static_cast<uint8_t>(packet.data[0]);
    std::memcpy(&x, &packet.data[1], sizeof(x));
    std::memcpy(&y, &packet.data[3], sizeof(y));
    std::memcpy(&dx, &packet.data[5], sizeof(dx));
    std::memcpy(&dy, &packet.data[7], sizeof(dy));
    shoot.x = ntohs(x);
    shoot.y = ntohs(y);
    shoot.dirx = static_cast<int16_t>(ntohs(dx));
    shoot.diry = static_cast<int16_t>(ntohs(dy));
    shoot.damage = static_cast<uint8_t>(packet.data[9]);
    shoot.name = packet.data.substr(10);
    return true;
}

struct DeathEvent
{
    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint16_t damage;
};

inline Packet makeDeath(uint16_t maxHp)
{
    uint16_t v = htons(maxHp);
    std::string data(2, '\0');
    std::memcpy(&data[0], &v, sizeof(v));
    return {PacketType::Death, data};
}

inline bool readDeath(const Packet& packet, uint16_t& maxHp)
{
    if (packet.data.size() != 2)
        return false;
    std::memcpy(&maxHp, packet.data.data(), sizeof(maxHp));
    maxHp = ntohs(maxHp);
    return true;
}

inline Packet makePlayerDied(const DeathEvent& d)
{
    uint16_t x = htons(d.x), y = htons(d.y), dmg = htons(d.damage);
    std::string data(7, '\0');
    data[0] = static_cast<char>(d.id);
    std::memcpy(&data[1], &x, sizeof(x));
    std::memcpy(&data[3], &y, sizeof(y));
    std::memcpy(&data[5], &dmg, sizeof(dmg));
    return {PacketType::PlayerDied, data};
}

inline bool readPlayerDied(const Packet& packet, DeathEvent& d)
{
    if (packet.data.size() != 7)
        return false;
    d.id = static_cast<uint8_t>(packet.data[0]);
    std::memcpy(&d.x, &packet.data[1], sizeof(d.x));
    std::memcpy(&d.y, &packet.data[3], sizeof(d.y));
    std::memcpy(&d.damage, &packet.data[5], sizeof(d.damage));
    d.x = ntohs(d.x);
    d.y = ntohs(d.y);
    d.damage = ntohs(d.damage);
    return true;
}

struct LobbyEntry
{
    std::string name;
    uint8_t players;
    uint8_t state;
};

inline Packet makeLobbyData(const std::vector<LobbyEntry>& lobbies)
{
    std::string data(1, static_cast<char>(lobbies.size()));
    for (const auto& l : lobbies)
    {
        data += static_cast<char>(l.name.size());
        data += l.name;
        data += static_cast<char>(l.players);
        data += static_cast<char>(l.state);
    }
    return {PacketType::LobbyData, data};
}

inline bool readLobbyData(const Packet& packet, std::vector<LobbyEntry>& out)
{
    out.clear();
    if (packet.data.empty())
        return false;
    size_t count = static_cast<uint8_t>(packet.data[0]);
    size_t pos = 1;
    for (size_t i = 0; i < count; i++)
    {
        if (pos >= packet.data.size())
            return false;
        size_t len = static_cast<uint8_t>(packet.data[pos++]);
        if (pos + len + 2 > packet.data.size())
            return false;
        LobbyEntry e;
        e.name    = packet.data.substr(pos, len);
        pos      += len;
        e.players = static_cast<uint8_t>(packet.data[pos++]);
        e.state   = static_cast<uint8_t>(packet.data[pos++]);
        out.push_back(std::move(e));
    }
    return true;
}
