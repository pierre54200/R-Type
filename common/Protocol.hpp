#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Protocole binaire
enum class PacketType : uint8_t {
    Connect = 1,
    Message = 2,
    Disconnect = 3,
    Action = 4,
    Position = 5,
};

// Action du joueur
enum class Action : uint8_t {
    Up = 0,
    Down = 1,
    Left = 2,
    Right = 3,
    A = 4,
    B = 5,
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
    if (buffer[0] < 1 || buffer[0] > 5)
        return false;
    packet.type = static_cast<PacketType>(buffer[0]);
    packet.data.assign(reinterpret_cast<const char*>(buffer + HEADER_SIZE), size);
    return true;
}

inline Packet makeAction(Action action) {
    return {PacketType::Action, std::string(1, static_cast<char>(action))};
}

inline bool readAction(const Packet& packet, Action& action) {
    if (packet.data.size() != 1 || static_cast<uint8_t>(packet.data[0]) > 5)
        return false;
    action = static_cast<Action>(packet.data[0]);
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
