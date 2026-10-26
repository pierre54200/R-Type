#pragma once

#include <optional>
#include <string>

#include "Protocol.hpp"
#include "Socket.hpp"

class NetworkClient
{
    public:
        NetworkClient(const std::string& ip, int port, const std::string& name);
        ~NetworkClient();
        NetworkClient(const NetworkClient&) = delete;
        NetworkClient&  operator=(const NetworkClient&) = delete;

        void    send(const Packet& packet);

        std::optional<Packet>   receive();

    private:
        socket_t    _socket = INVALID_SOCK;
        sockaddr_in _server{};
};