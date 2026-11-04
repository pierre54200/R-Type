#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "Protocol.hpp"
#include "Server/Server.hpp"
#include "Socket.hpp"

namespace {

using namespace std::chrono_literals;

constexpr int REPLY_TIMEOUT_MS = 200;

int freePort()
{
    ensureNetworkInit();
    socket_t s = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    socket_len_t len = sizeof(addr);
    getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len);
    closeSocket(s);
    return ntohs(addr.sin_port);
}

int startServer()
{
    int port = freePort();
    auto* server = new Server(port);
    std::thread([server] { server->run(); }).detach();
    return port;
}

class TestClient
{
public:
    explicit TestClient(int port)
    {
        ensureNetworkInit();
        _socket = socket(AF_INET, SOCK_DGRAM, 0);
        _server.sin_family = AF_INET;
        _server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        _server.sin_port = htons(static_cast<uint16_t>(port));
    }

    ~TestClient() { closeSocket(_socket); }

    TestClient(const TestClient&) = delete;
    TestClient& operator=(const TestClient&) = delete;

    void send(const Packet& packet) { sendRaw(encode(packet)); }

    void sendRaw(const std::vector<uint8_t>& bytes)
    {
        sendto(_socket, reinterpret_cast<const char*>(bytes.data()),
               static_cast<int>(bytes.size()), 0,
               reinterpret_cast<const sockaddr*>(&_server), sizeof(_server));
    }

    // Every packet received until the server stays silent for timeoutMs
    std::vector<Packet> receiveAll(int timeoutMs = REPLY_TIMEOUT_MS)
    {
        std::vector<Packet> packets;
        while (waitReadable(_socket, timeoutMs)) {
            uint8_t buffer[MAX_PACKET_SIZE];
            auto received = recvfrom(_socket, reinterpret_cast<char*>(buffer),
                                     static_cast<int>(sizeof(buffer)), 0, nullptr, nullptr);
            Packet packet;
            if (received > 0 && decode(buffer, static_cast<std::size_t>(received), packet))
                packets.push_back(packet);
        }
        return packets;
    }

    void drain() { receiveAll(80); }

    void connect(const std::string& name)
    {
        send({PacketType::Connect, name});
        drain();
    }

    void enter(const std::string& name, const std::string& lobby, bool create)
    {
        connect(name);
        send({create ? PacketType::LobbyCreate : PacketType::LobbyJoin, lobby});
        drain();
    }

private:
    socket_t _socket;
    sockaddr_in _server{};
};

bool hasInfoContaining(const std::vector<Packet>& packets, const std::string& text)
{
    for (const Packet& p : packets)
        if (p.type == PacketType::Info && p.data.find(text) != std::string::npos)
            return true;
    return false;
}

std::vector<PlayerPosition> positionsIn(const std::vector<Packet>& packets)
{
    std::vector<PlayerPosition> result;
    for (const Packet& p : packets) {
        PlayerPosition pos{};
        if (p.type == PacketType::Position && readPosition(p, pos))
            result.push_back(pos);
    }
    return result;
}

std::optional<PlayerPosition> positionOf(const std::vector<Packet>& packets, const std::string& name)
{
    for (const PlayerPosition& pos : positionsIn(packets))
        if (pos.name == name)
            return pos;
    return std::nullopt;
}

std::size_t countType(const std::vector<Packet>& packets, PacketType type)
{
    std::size_t n = 0;
    for (const Packet& p : packets)
        n += p.type == type;
    return n;
}

class ServerTest : public ::testing::Test
{
protected:
    void SetUp() override { port = startServer(); }
    int port = 0;
};

}

TEST_F(ServerTest, ConnectGetsWelcomeAndLobbyList)
{
    TestClient alice(port);
    alice.send({PacketType::Connect, "Alice"});
    auto replies = alice.receiveAll();

    EXPECT_TRUE(hasInfoContaining(replies, "Bienvenue Alice"));
    EXPECT_TRUE(hasInfoContaining(replies, "Aucun lobby"));
}

TEST_F(ServerTest, ConnectWithEmptyNameIsIgnored)
{
    TestClient anon(port);
    anon.send({PacketType::Connect, ""});
    EXPECT_TRUE(anon.receiveAll().empty());
}

TEST_F(ServerTest, DuplicateNameIsRefused)
{
    TestClient first(port);
    first.connect("Alice");

    TestClient second(port);
    second.send({PacketType::Connect, "Alice"});
    auto replies = second.receiveAll();

    EXPECT_TRUE(hasInfoContaining(replies, "deja pris"));
    EXPECT_FALSE(hasInfoContaining(replies, "Bienvenue"));
}

TEST_F(ServerTest, ConnectingTwiceFromSameAddressIsIgnored)
{
    TestClient alice(port);
    alice.connect("Alice");

    alice.send({PacketType::Connect, "Alice2"});
    EXPECT_TRUE(alice.receiveAll().empty());
}

TEST_F(ServerTest, PacketsFromUnknownClientsAreIgnored)
{
    TestClient stranger(port);
    stranger.send({PacketType::LobbyList, ""});
    stranger.send({PacketType::LobbyCreate, "x"});
    stranger.send({PacketType::Message, "hello"});
    stranger.send(makePosition({1, 10, 10, "ghost"}));

    EXPECT_TRUE(stranger.receiveAll().empty());
}

TEST_F(ServerTest, MalformedPacketsDoNotBreakTheServer)
{
    TestClient attacker(port);
    attacker.sendRaw({});                         // empty datagram
    attacker.sendRaw({1});                        // shorter than the header
    attacker.sendRaw({1, 0, 50, 'a'});            // announced size too big
    attacker.sendRaw({0, 0, 0});                  // type 0
    attacker.sendRaw({255, 0, 0});                // unknown type
    attacker.sendRaw(std::vector<uint8_t>(2000, 0xAB)); // bigger than MAX_PACKET_SIZE
    attacker.send({PacketType::Position, "x"});   // valid header, short payload
    attacker.send({PacketType::Shoot, "x"});
    attacker.drain();

    TestClient alice(port);
    alice.send({PacketType::Connect, "Alice"});
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "Bienvenue Alice"));
}

TEST_F(ServerTest, ShortPayloadsInLobbyAreIgnored)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.enter("Bob", "room", false);
    alice.drain();

    bob.send({PacketType::Position, "abcd"});
    bob.send({PacketType::Shoot, "abcdefgh"});

    EXPECT_TRUE(alice.receiveAll().empty());
}

TEST_F(ServerTest, CreateLobbyConfirmsAndSendsOwnPosition)
{
    TestClient alice(port);
    alice.connect("Alice");

    alice.send({PacketType::LobbyCreate, "room"});
    auto replies = alice.receiveAll();

    EXPECT_TRUE(hasInfoContaining(replies, "Lobby room cree"));
    auto pos = positionOf(replies, "Alice");
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->x, 100);
    EXPECT_EQ(pos->y, 100);
}

TEST_F(ServerTest, CreateLobbyWithEmptyNameShowsUsage)
{
    TestClient alice(port);
    alice.connect("Alice");

    alice.send({PacketType::LobbyCreate, ""});
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "Usage : /create"));
}

TEST_F(ServerTest, CreatingAnExistingLobbyIsRefused)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);

    TestClient bob(port);
    bob.connect("Bob");
    bob.send({PacketType::LobbyCreate, "room"});

    EXPECT_TRUE(hasInfoContaining(bob.receiveAll(), "existe deja"));
}

TEST_F(ServerTest, JoiningMissingLobbyIsRefused)
{
    TestClient alice(port);
    alice.connect("Alice");

    alice.send({PacketType::LobbyJoin, "nowhere"});
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "n'existe pas"));
}

TEST_F(ServerTest, JoiningOwnLobbyAgainIsRefused)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);

    alice.send({PacketType::LobbyJoin, "room"});
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "deja dans room"));
}

TEST_F(ServerTest, JoinExchangesPositionsBothWays)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);

    TestClient bob(port);
    bob.connect("Bob");
    bob.send({PacketType::LobbyJoin, "room"});
    auto bobReplies = bob.receiveAll();
    auto aliceReplies = alice.receiveAll();

    EXPECT_TRUE(hasInfoContaining(bobReplies, "Tu as rejoint room"));
    EXPECT_TRUE(positionOf(bobReplies, "Alice").has_value());
    EXPECT_TRUE(positionOf(aliceReplies, "Bob").has_value());
    EXPECT_TRUE(hasInfoContaining(aliceReplies, "Bob a rejoint le lobby"));
}

TEST_F(ServerTest, PlayersGetDistinctIds)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.connect("Bob");
    bob.send({PacketType::LobbyJoin, "room"});
    auto replies = bob.receiveAll();

    auto a = positionOf(replies, "Alice");
    auto b = positionOf(replies, "Bob");
    ASSERT_TRUE(a && b);
    EXPECT_NE(a->id, b->id);
}

TEST_F(ServerTest, LobbyListShowsLobbiesAndPlayerCount)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.enter("Bob", "room", false);
    TestClient carol(port);
    carol.connect("Carol");

    carol.send({PacketType::LobbyList, ""});
    EXPECT_TRUE(hasInfoContaining(carol.receiveAll(), "room (2 joueur(s))"));
}

TEST_F(ServerTest, LeavingNotifiesTheOthers)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.enter("Bob", "room", false);
    alice.drain();

    bob.send({PacketType::LobbyLeave, ""});

    EXPECT_TRUE(hasInfoContaining(bob.receiveAll(), "Tu as quitte room"));
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "Bob a quitte le lobby"));
}

TEST_F(ServerTest, LastPlayerLeavingDeletesTheLobby)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    alice.send({PacketType::LobbyLeave, ""});
    alice.drain();

    alice.send({PacketType::LobbyList, ""});
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "Aucun lobby"));
}

TEST_F(ServerTest, PositionIsRelayedToOthersButNotToSender)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.enter("Bob", "room", false);
    alice.drain();

    bob.send(makePosition({0, 250, 300, "Bob"}));

    auto pos = positionOf(alice.receiveAll(), "Bob");
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->x, 250);
    EXPECT_EQ(pos->y, 300);
    EXPECT_TRUE(bob.receiveAll().empty());
}

TEST_F(ServerTest, PositionIsClampedToTheMap)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.enter("Bob", "room", false);
    alice.drain();

    bob.send(makePosition({0, 60000, 60000, "Bob"}));

    auto pos = positionOf(alice.receiveAll(), "Bob");
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->x, 600);
    EXPECT_EQ(pos->y, 800);
}

TEST_F(ServerTest, ServerOverridesSpoofedIdAndName)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.connect("Bob");
    bob.send({PacketType::LobbyJoin, "room"});
    auto realBob = positionOf(alice.receiveAll(), "Bob");
    ASSERT_TRUE(realBob.has_value());

    bob.send(makePosition({99, 10, 20, "Alice"}));
    auto relayed = positionsIn(alice.receiveAll());

    ASSERT_EQ(relayed.size(), 1u);
    EXPECT_EQ(relayed[0].name, "Bob");
    EXPECT_EQ(relayed[0].id, realBob->id);
}

TEST_F(ServerTest, LobbiesAreIsolated)
{
    TestClient alice(port);
    alice.enter("Alice", "red", true);
    TestClient bob(port);
    bob.enter("Bob", "blue", true);
    alice.drain();

    bob.send(makePosition({0, 10, 10, "Bob"}));
    bob.send({PacketType::Message, "hello"});
    bob.send(makeShoot({0, 10, 10, 1, 0, "Bob"}));

    EXPECT_TRUE(alice.receiveAll().empty());
}

TEST_F(ServerTest, PositionOutsideLobbyIsIgnored)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.connect("Bob");
    alice.drain();

    bob.send(makePosition({0, 10, 10, "Bob"}));

    EXPECT_TRUE(alice.receiveAll().empty());
}

TEST_F(ServerTest, ChatMessageIsRelayedWithSenderName)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.enter("Bob", "room", false);
    alice.drain();

    bob.send({PacketType::Message, "salut"});
    auto replies = alice.receiveAll();

    ASSERT_EQ(countType(replies, PacketType::Message), 1u);
    for (const Packet& p : replies) {
        if (p.type == PacketType::Message) {
            EXPECT_EQ(p.data, "Bob : salut");
        }
    }
    EXPECT_TRUE(bob.receiveAll().empty());
}

TEST_F(ServerTest, ChatOutsideLobbyIsRefused)
{
    TestClient alice(port);
    alice.connect("Alice");

    alice.send({PacketType::Message, "hello"});
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "dans un lobby"));
}

TEST_F(ServerTest, ShootIsRelayedWithServerSideIdentity)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    TestClient bob(port);
    bob.connect("Bob");
    bob.send({PacketType::LobbyJoin, "room"});
    auto realBob = positionOf(alice.receiveAll(), "Bob");
    ASSERT_TRUE(realBob.has_value());
    bob.drain();

    bob.send(makeShoot({77, 40, 50, -3, 4, "Alice"}));
    auto replies = alice.receiveAll();

    ASSERT_EQ(countType(replies, PacketType::Shoot), 1u);
    Shoot shot{};
    for (const Packet& p : replies) {
        if (p.type == PacketType::Shoot) {
            ASSERT_TRUE(readShoot(p, shot));
        }
    }
    EXPECT_EQ(shot.id, realBob->id);
    EXPECT_EQ(shot.name, "Bob");
    EXPECT_EQ(shot.x, 40);
    EXPECT_EQ(shot.y, 50);
    EXPECT_EQ(shot.dirx, -3);
    EXPECT_EQ(shot.diry, 4);
    EXPECT_TRUE(bob.receiveAll().empty());
}

TEST_F(ServerTest, DisconnectNotifiesLobbyAndFreesTheName)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    {
        TestClient bob(port);
        bob.enter("Bob", "room", false);
        alice.drain();
        bob.send({PacketType::Disconnect, ""});
        bob.drain();
    }
    EXPECT_TRUE(hasInfoContaining(alice.receiveAll(), "Bob a quitte le lobby"));

    TestClient newBob(port);
    newBob.send({PacketType::Connect, "Bob"});
    EXPECT_TRUE(hasInfoContaining(newBob.receiveAll(), "Bienvenue Bob"));
}

TEST_F(ServerTest, DisconnectedClientIsForgotten)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    alice.send({PacketType::Disconnect, ""});
    alice.drain();

    alice.send({PacketType::LobbyList, ""});
    EXPECT_TRUE(alice.receiveAll().empty());
}

TEST_F(ServerTest, ServerKeepsRunningWhenAClientVanishes)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    {
        TestClient ghost(port);
        ghost.enter("Ghost", "room", false);
    }
    alice.drain();

    TestClient bob(port);
    bob.connect("Bob");
    bob.send({PacketType::LobbyJoin, "room"});
    EXPECT_TRUE(hasInfoContaining(bob.receiveAll(), "Tu as rejoint room"));
}

TEST_F(ServerTest, DISABLED_CrashedClientIsDetectedAndOthersAreNotified)
{
    TestClient alice(port);
    alice.enter("Alice", "room", true);
    {
        TestClient ghost(port);
        ghost.enter("Ghost", "room", false);
    }
    alice.drain();

    auto replies = alice.receiveAll(5000);
    EXPECT_TRUE(hasInfoContaining(replies, "Ghost"));
}

TEST_F(ServerTest, DISABLED_EnemySpawnOutsideLobbyIsIgnored)
{
    TestClient alice(port);
    alice.connect("Alice");
    TestClient bob(port);
    bob.connect("Bob");

    bob.send(makeEnemySpawn({1, 10, 10, "bydo"}));
    EXPECT_TRUE(alice.receiveAll().empty());
}
