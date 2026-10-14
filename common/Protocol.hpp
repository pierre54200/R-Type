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
};

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
    if (buffer[0] < 1 || buffer[0] > 3)
        return false;
    packet.type = static_cast<PacketType>(buffer[0]);
    packet.data.assign(reinterpret_cast<const char*>(buffer + HEADER_SIZE), size);
    return true;
}
