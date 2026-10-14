#include "Client.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Client::Client(const std::string& ip, int port, const std::string& name) {
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0)
        throw std::runtime_error("socket() a echoue");

    _server.sin_family = AF_INET;
    _server.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &_server.sin_addr) != 1) {
        close(_socket);
        throw std::runtime_error("adresse IP invalide : " + ip);
    }

    send({PacketType::Connect, name});
    std::cout << "Connecte a " << ip << ":" << port
              << " (tape un message puis Entree, Ctrl+D pour quitter)" << std::endl;
}

Client::~Client() {
    close(_socket);
}

void Client::run() {
    pollfd fds[2] = {
        {STDIN_FILENO, POLLIN, 0},
        {_socket, POLLIN, 0},
    };

    while (true) {
        if (poll(fds, 2, 100) < 0)
            continue;
        if (fds[1].revents & POLLIN)
            readSocket();
        if (fds[0].revents & (POLLIN | POLLHUP) && !readKeyboard())
            break;
    }
    send({PacketType::Disconnect, ""});
}

// False si le joueur s'est déconnecté
bool Client::readKeyboard() {
    char buffer[256];
    ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (n <= 0)
        return false;
    _input.append(buffer, static_cast<std::size_t>(n));

    std::size_t end;
    while ((end = _input.find('\n')) != std::string::npos) {
        std::string line = _input.substr(0, end);
        _input.erase(0, end + 1);
        if (!line.empty())
            send({PacketType::Message, line});
    }
    return true;
}

void Client::readSocket() {
    uint8_t buffer[MAX_PACKET_SIZE];
    ssize_t received = recvfrom(_socket, buffer, sizeof(buffer), 0, nullptr, nullptr);
    if (received < 0)
        return;

    Packet packet;
    if (decode(buffer, static_cast<std::size_t>(received), packet) &&
        packet.type == PacketType::Message)
        std::cout << packet.data << std::endl;
}

void Client::send(const Packet& packet) {
    std::vector<uint8_t> buffer = encode(packet);
    sendto(_socket, buffer.data(), buffer.size(), 0,
           reinterpret_cast<const sockaddr*>(&_server), sizeof(_server));
}
