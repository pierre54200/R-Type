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
    };

    void handle(const Packet& packet, const sockaddr_in& from);
    void send(const Packet& packet, const sockaddr_in& to);
    Client* find(const sockaddr_in& address);

    int _socket;
    std::vector<Client> _clients;
};
