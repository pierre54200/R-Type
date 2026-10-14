#include "Server.hpp"

#include <algorithm>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port) {
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0)
        throw std::runtime_error("socket() a echoue");

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(_socket);
        throw std::runtime_error("bind() a echoue sur le port " + std::to_string(port));
    }
    std::cout << "Serveur en ecoute sur le port " << port << std::endl;
}

Server::~Server() {
    close(_socket);
}

void Server::run() {
    uint8_t buffer[MAX_PACKET_SIZE];
    pollfd fds[1] = {{_socket, POLLIN, 0}};

    while (true) {
        if (poll(fds, 1, 100) <= 0 || !(fds[0].revents & POLLIN))
            continue;

        sockaddr_in from{};
        socklen_t fromLength = sizeof(from);
        ssize_t received = recvfrom(_socket, buffer, sizeof(buffer), 0,
                                    reinterpret_cast<sockaddr*>(&from), &fromLength);
        if (received < 0)
            continue;

        Packet packet;
        if (decode(buffer, static_cast<std::size_t>(received), packet))
            handle(packet, from);
    }
}

void Server::handle(const Packet& packet, const sockaddr_in& from) {
    Client* client = find(from);

    switch (packet.type) {
    case PacketType::Connect:
        if (!client) {
            _clients.push_back({from, packet.data});
            std::cout << packet.data << " s'est connecte (" << _clients.size()
                      << " joueur(s))" << std::endl;
        }
        break;

    case PacketType::Message:
        if (!client)
            return;
        std::cout << client->name << " : " << packet.data << std::endl;
        for (const Client& other : _clients) {
            if (&other != client)
                send({PacketType::Message, client->name + " : " + packet.data}, other.address);
        }
        break;

    case PacketType::Disconnect:
        if (!client)
            return;
        std::cout << client->name << " s'est deconnecte" << std::endl;
        _clients.erase(_clients.begin() + (client - _clients.data()));
        break;
    }
}

void Server::send(const Packet& packet, const sockaddr_in& to) {
    std::vector<uint8_t> buffer = encode(packet);
    sendto(_socket, buffer.data(), buffer.size(), 0,
           reinterpret_cast<const sockaddr*>(&to), sizeof(to));
}

Server::Client* Server::find(const sockaddr_in& address) {
    for (Client& client : _clients) {
        if (client.address.sin_addr.s_addr == address.sin_addr.s_addr &&
            client.address.sin_port == address.sin_port)
            return &client;
    }
    return nullptr;
}
