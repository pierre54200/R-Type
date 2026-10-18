#pragma once

#include <netinet/in.h>
#include <optional>
#include <string>

#include "Protocol.hpp"

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
        int         _socket;
        sockaddr_in _server{};
};