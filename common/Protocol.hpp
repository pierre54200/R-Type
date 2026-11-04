#pragma once

#include "Socket.hpp"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Protocole binaire
enum class PacketType : uint8_t {
    Connect = 1,
    Message = 2,
    Disconnect = 3,
    Position = 4,
    LobbyList = 5,
    LobbyCreate = 6,
    LobbyJoin = 7,
    LobbyLeave = 8,
    Info = 9,
    EnemySpawn = 10,
    Shoot   =   11,
    //  Upgrade,
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
    if (buffer[0] < 1 || buffer[0] > 15)
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
};

inline Packet makeShoot(const Shoot& s) {
    uint16_t x = htons(s.x), y = htons(s.y);
    uint16_t dx = htons(static_cast<uint16_t>(s.dirx));
    uint16_t dy = htons(static_cast<uint16_t>(s.diry));
    std::string data(9, '\0');
    data[0] = static_cast<char>(s.id);
    std::memcpy(&data[1], &x,  2);
    std::memcpy(&data[3], &y,  2);
    std::memcpy(&data[5], &dx, 2);
    std::memcpy(&data[7], &dy, 2);
    return {PacketType::Shoot, data + s.name};
}

inline bool readShoot(const Packet& packet, Shoot&    shoot)
{
    if (packet.data.size() < 9)
        return false;
    shoot.id = static_cast<uint8_t>(packet.data[0]);
    std::memcpy(&shoot.x, &packet.data[1], sizeof(shoot.x));
    std::memcpy(&shoot.y, &packet.data[3], sizeof(shoot.y));
    std::memcpy(&shoot.dirx, &packet.data[5], sizeof(shoot.dirx));
    std::memcpy(&shoot.diry, &packet.data[7], sizeof(shoot.diry));
    shoot.x = ntohs(shoot.x);
    shoot.y = ntohs(shoot.y);
    shoot.dirx = ntohs(shoot.dirx);
    shoot.diry = ntohs(shoot.diry);
    shoot.name = packet.data.substr(9);
    return true;
}
