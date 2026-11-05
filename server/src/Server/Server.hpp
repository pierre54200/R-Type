#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

#include "Protocol.hpp"

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run();

private:
    struct Client {
        sockaddr_in address;
        std::string name;
        uint8_t id;
        uint16_t x;
        uint16_t y;
        std::string lobby; // vide = pas dans un lobby
        bool ready = false;
        bool dead = false;
    };

    enum class Phase
    {
        Waiting,
        Playing,
        Intermission
    };
    struct LobbyState {
        Phase phase = Phase::Waiting;
        int wave = 0;
        uint8_t host = 0;
        bool hasHost = false;
    };

    static constexpr uint16_t MAP_WIDTH = 4000;
    static constexpr uint16_t MAP_HEIGHT = 4000;
    static constexpr std::size_t MAX_LOBBY_NAME = 32;
    static constexpr std::size_t MAX_LISTED_LOBBIES = 20;

    void handle(const Packet& packet, const sockaddr_in& from);
    void send(const Packet& packet, const sockaddr_in& to);
    void info(const Client& client, const std::string& text);
    void sendToLobby(const std::string& lobby, const Packet& packet, const Client* except = nullptr);
    Client* find(const sockaddr_in& address);
    bool nameTaken(const std::string& name) const;

    // Lobbies
    bool lobbyExists(const std::string& lobby) const;
    void listLobbies(const Client& client);
    void createLobby(Client& client, const std::string& lobby);
    void joinLobby(Client& client, const std::string& lobby);
    void leaveLobby(Client& client);
    void electHost(const std::string& lobby);

    // Waves
    void resetReady(const std::string& lobby);
    void checkReady(const std::string& lobby);

    int _socket;
    std::vector<Client> _clients;
    std::map<std::string, LobbyState> _lobbies;
    uint8_t _nextId = 1;
};
