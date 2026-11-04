#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "Protocol.hpp"

namespace {

Packet roundTrip(const Packet& in)
{
    std::vector<uint8_t> buffer = encode(in);
    Packet out{};
    EXPECT_TRUE(decode(buffer.data(), buffer.size(), out));
    return out;
}

}

TEST(Protocol, EncodeWritesHeaderInNetworkByteOrder)
{
    std::vector<uint8_t> buffer = encode({PacketType::Message, std::string(300, 'a')});

    ASSERT_EQ(buffer.size(), HEADER_SIZE + 300);
    EXPECT_EQ(buffer[0], static_cast<uint8_t>(PacketType::Message));
    // 300 = 0x012C, big endian on the wire
    EXPECT_EQ(buffer[1], 0x01);
    EXPECT_EQ(buffer[2], 0x2C);
}

TEST(Protocol, EncodeDecodeEmptyPayload)
{
    Packet out = roundTrip({PacketType::Disconnect, ""});

    EXPECT_EQ(out.type, PacketType::Disconnect);
    EXPECT_TRUE(out.data.empty());
}

TEST(Protocol, EncodeDecodeKeepsBinaryPayload)
{
    std::string payload("ab\0cd\xff", 6);
    Packet out = roundTrip({PacketType::Message, payload});

    EXPECT_EQ(out.type, PacketType::Message);
    EXPECT_EQ(out.data, payload);
}

TEST(Protocol, EveryKnownTypeSurvivesRoundTrip)
{
    for (uint8_t t = 1; t <= static_cast<uint8_t>(PacketType::Shoot); ++t) {
        Packet out = roundTrip({static_cast<PacketType>(t), "x"});
        EXPECT_EQ(static_cast<uint8_t>(out.type), t);
    }
}

TEST(Protocol, DecodeRejectsBufferShorterThanHeader)
{
    const uint8_t buffer[] = {1, 0};
    Packet out{};

    EXPECT_FALSE(decode(buffer, 0, out));
    EXPECT_FALSE(decode(buffer, sizeof(buffer), out));
}

TEST(Protocol, DecodeRejectsTruncatedPayload)
{
    std::vector<uint8_t> buffer = encode({PacketType::Message, "hello"});
    Packet out{};

    EXPECT_FALSE(decode(buffer.data(), buffer.size() - 1, out));
}

TEST(Protocol, DecodeRejectsTrailingBytes)
{
    std::vector<uint8_t> buffer = encode({PacketType::Message, "hello"});
    buffer.push_back(0);
    Packet out{};

    EXPECT_FALSE(decode(buffer.data(), buffer.size(), out));
}

TEST(Protocol, DecodeRejectsTypeZeroAndOutOfRange)
{
    for (uint8_t badType : {uint8_t{0}, uint8_t{16}, uint8_t{200}, uint8_t{255}}) {
        const uint8_t buffer[] = {badType, 0, 0};
        Packet out{};
        EXPECT_FALSE(decode(buffer, sizeof(buffer), out)) << "type " << int(badType);
    }
}

TEST(Protocol, DISABLED_DecodeRejectsUndefinedTypes)
{
    for (uint8_t badType = 13; badType <= 15; ++badType) {
        const uint8_t buffer[] = {badType, 0, 0};
        Packet out{};
        EXPECT_FALSE(decode(buffer, sizeof(buffer), out)) << "type " << int(badType);
    }
}

// Robustness requirement of the subject: malformed packets must never crash.
TEST(Protocol, RandomBuffersNeverCrashDecoderOrReaders)
{
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> byte(0, 255);
    std::uniform_int_distribution<int> length(0, 64);

    for (int i = 0; i < 20000; ++i) {
        std::vector<uint8_t> buffer(static_cast<std::size_t>(length(rng)));
        for (auto& b : buffer)
            b = static_cast<uint8_t>(byte(rng));
        // Make some of them structurally valid so the readers get exercised too
        if (buffer.size() >= HEADER_SIZE && i % 2 == 0) {
            uint16_t size = htons(static_cast<uint16_t>(buffer.size() - HEADER_SIZE));
            std::memcpy(&buffer[1], &size, sizeof(size));
            buffer[0] = static_cast<uint8_t>(1 + i % 12);
        }

        Packet packet{};
        if (!decode(buffer.data(), buffer.size(), packet))
            continue;
        PlayerPosition position{};
        Shoot shoot{};
        readPosition(packet, position);
        readEnemySpawn(packet, position);
        readShoot(packet, shoot);
    }
    SUCCEED();
}

/* ---------- Position ---------- */

TEST(Protocol, PositionRoundTrip)
{
    Packet packet = makePosition({7, 123, 456, "Pierre"});
    EXPECT_EQ(packet.type, PacketType::Position);

    PlayerPosition out{};
    ASSERT_TRUE(readPosition(roundTrip(packet), out));
    EXPECT_EQ(out.id, 7);
    EXPECT_EQ(out.x, 123);
    EXPECT_EQ(out.y, 456);
    EXPECT_EQ(out.name, "Pierre");
}

TEST(Protocol, PositionUsesNetworkByteOrder)
{
    Packet packet = makePosition({1, 0x0102, 0x0304, ""});

    ASSERT_EQ(packet.data.size(), 5u);
    EXPECT_EQ(static_cast<uint8_t>(packet.data[1]), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(packet.data[2]), 0x02);
    EXPECT_EQ(static_cast<uint8_t>(packet.data[3]), 0x03);
    EXPECT_EQ(static_cast<uint8_t>(packet.data[4]), 0x04);
}

TEST(Protocol, PositionHandlesExtremeValuesAndEmptyName)
{
    PlayerPosition out{};
    ASSERT_TRUE(readPosition(makePosition({255, 0, 65535, ""}), out));
    EXPECT_EQ(out.id, 255);
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 65535);
    EXPECT_TRUE(out.name.empty());
}

TEST(Protocol, ReadPositionRejectsShortPayload)
{
    PlayerPosition out{};
    EXPECT_FALSE(readPosition({PacketType::Position, std::string(4, '\0')}, out));
}

/* ---------- EnemySpawn ---------- */

TEST(Protocol, EnemySpawnRoundTrip)
{
    Packet packet = makeEnemySpawn({3, 500, 42, "bydo"});
    EXPECT_EQ(packet.type, PacketType::EnemySpawn);

    PlayerPosition out{};
    ASSERT_TRUE(readEnemySpawn(roundTrip(packet), out));
    EXPECT_EQ(out.id, 3);
    EXPECT_EQ(out.x, 500);
    EXPECT_EQ(out.y, 42);
    EXPECT_EQ(out.name, "bydo");
}

TEST(Protocol, ReadEnemySpawnRejectsShortPayload)
{
    PlayerPosition out{};
    EXPECT_FALSE(readEnemySpawn({PacketType::EnemySpawn, "abc"}, out));
}

/* ---------- Shoot ---------- */

TEST(Protocol, ShootRoundTripWithNegativeDirection)
{
    Packet packet = makeShoot({2, 10, 20, -1, -300, "Bob"});
    EXPECT_EQ(packet.type, PacketType::Shoot);

    Shoot out{};
    ASSERT_TRUE(readShoot(roundTrip(packet), out));
    EXPECT_EQ(out.id, 2);
    EXPECT_EQ(out.x, 10);
    EXPECT_EQ(out.y, 20);
    EXPECT_EQ(out.dirx, -1);
    EXPECT_EQ(out.diry, -300);
    EXPECT_EQ(out.name, "Bob");
}

TEST(Protocol, ShootHandlesInt16Limits)
{
    Shoot out{};
    ASSERT_TRUE(readShoot(makeShoot({0, 0, 0, INT16_MIN, INT16_MAX, ""}), out));
    EXPECT_EQ(out.dirx, INT16_MIN);
    EXPECT_EQ(out.diry, INT16_MAX);
}

TEST(Protocol, ReadShootRejectsShortPayload)
{
    Shoot out{};
    EXPECT_FALSE(readShoot({PacketType::Shoot, std::string(8, '\0')}, out));
}

TEST(Protocol, ShootPacketFitsInMaxPacketSize)
{
    std::vector<uint8_t> buffer = encode(makeShoot({0, 0, 0, 0, 0, std::string(32, 'n')}));
    EXPECT_LE(buffer.size(), MAX_PACKET_SIZE);
}
