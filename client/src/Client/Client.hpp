#pragma once

#include <netinet/in.h>
#include <string>

#include "Protocol.hpp"

class Client {
public:
    Client(const std::string& ip, int port, const std::string& name);
    ~Client();

    void run();

private:
    bool readKeyboard();
    void readSocket();
    void send(const Packet& packet);

    int _socket;
    sockaddr_in _server{};
    std::string _input;
};
