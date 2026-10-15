#pragma once

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
    };

    static constexpr uint16_t MAP_WIDTH = 800;
    static constexpr uint16_t MAP_HEIGHT = 600;
    static constexpr uint16_t STEP = 10;

    void handle(const Packet& packet, const sockaddr_in& from);
    void send(const Packet& packet, const sockaddr_in& to);
    void info(const Client& client, const std::string& text);
    void sendToLobby(const std::string& lobby, const Packet& packet, const Client* except = nullptr);
    void move(Client& client, Action action);
    Client* find(const sockaddr_in& address);

    // Lobbies
    bool lobbyExists(const std::string& lobby) const;
    void listLobbies(const Client& client);
    void createLobby(Client& client, const std::string& lobby);
    void joinLobby(Client& client, const std::string& lobby);
    void leaveLobby(Client& client);

    int _socket;
    std::vector<Client> _clients;
    uint8_t _nextId = 1;
};
