#include "Server.hpp"

#include <algorithm>
#include <iostream>
#include <map>
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

    // Avant tout, il faut s'etre connecte avec un pseudo
    if (packet.type == PacketType::Connect) {
        if (client || packet.data.empty())
            return;
        _clients.push_back({from, packet.data, _nextId++, 100, 100, ""});
        Client& added = _clients.back();
        std::cout << added.name << " s'est connecte (" << _clients.size() << " joueur(s))"
                  << std::endl;
        info(added, "Bienvenue " + added.name + " !");
        listLobbies(added);
        return;
    }
    if (!client)
        return;

    switch (packet.type) {
    case PacketType::LobbyList:
        listLobbies(*client);
        break;

    case PacketType::LobbyCreate:
        createLobby(*client, packet.data);
        break;

    case PacketType::LobbyJoin:
        joinLobby(*client, packet.data);
        break;

    case PacketType::LobbyLeave:
        leaveLobby(*client);
        break;

    case PacketType::Action: {
        Action action;
        if (client->lobby.empty() || !readAction(packet, action))
            return;
        move(*client, action);
        sendToLobby(client->lobby, makePosition({client->id, client->x, client->y, client->name}));
        break;
    }

    case PacketType::Message:
        if (client->lobby.empty()) {
            info(*client, "Tu dois etre dans un lobby pour envoyer des messages");
            return;
        }
        std::cout << "[" << client->lobby << "] " << client->name << " : " << packet.data
                  << std::endl;
        sendToLobby(client->lobby, {PacketType::Message, client->name + " : " + packet.data},
                    client);
        break;

    case PacketType::Disconnect:
        leaveLobby(*client);
        std::cout << client->name << " s'est deconnecte" << std::endl;
        _clients.erase(_clients.begin() + (client - _clients.data()));
        break;

    default:
        // Position et Info ne sont envoyes que par le serveur
        break;
    }
}

void Server::send(const Packet& packet, const sockaddr_in& to) {
    std::vector<uint8_t> buffer = encode(packet);
    sendto(_socket, buffer.data(), buffer.size(), 0,
           reinterpret_cast<const sockaddr*>(&to), sizeof(to));
}

void Server::info(const Client& client, const std::string& text) {
    send({PacketType::Info, text}, client.address);
}

void Server::sendToLobby(const std::string& lobby, const Packet& packet, const Client* except) {
    for (const Client& other : _clients) {
        if (other.lobby == lobby && &other != except)
            send(packet, other.address);
    }
}

bool Server::lobbyExists(const std::string& lobby) const {
    for (const Client& other : _clients) {
        if (other.lobby == lobby)
            return true;
    }
    return false;
}

void Server::listLobbies(const Client& client) {
    // Un lobby existe tant qu'il reste au moins un joueur dedans
    std::map<std::string, int> lobbies;
    for (const Client& other : _clients) {
        if (!other.lobby.empty())
            lobbies[other.lobby]++;
    }
    if (lobbies.empty()) {
        info(client, "Aucun lobby. Cree-en un avec /create <nom>");
        return;
    }
    std::string text = "Lobbies :";
    for (const auto& [name, count] : lobbies)
        text += "\n  - " + name + " (" + std::to_string(count) + " joueur(s))";
    info(client, text + "\nRejoins-en un avec /join <nom>");
}

void Server::createLobby(Client& client, const std::string& lobby) {
    if (lobby.empty()) {
        info(client, "Usage : /create <nom>");
        return;
    }
    if (lobbyExists(lobby)) {
        info(client, "Le lobby " + lobby + " existe deja, utilise /join " + lobby);
        return;
    }
    leaveLobby(client);
    client.lobby = lobby;
    client.x = 100;
    client.y = 100;
    std::cout << client.name << " a cree le lobby " << lobby << std::endl;
    info(client, "Lobby " + lobby + " cree, tu es dedans");
    send(makePosition({client.id, client.x, client.y, client.name}), client.address);
}

void Server::joinLobby(Client& client, const std::string& lobby) {
    if (lobby.empty()) {
        info(client, "Usage : /join <nom>");
        return;
    }
    if (client.lobby == lobby) {
        info(client, "Tu es deja dans " + lobby);
        return;
    }
    if (!lobbyExists(lobby)) {
        info(client, "Le lobby " + lobby + " n'existe pas");
        return;
    }
    leaveLobby(client);
    client.lobby = lobby;
    client.x = 100;
    client.y = 100;
    std::cout << client.name << " a rejoint le lobby " << lobby << std::endl;
    info(client, "Tu as rejoint " + lobby);

    // Le nouveau recoit la position de tout le lobby, le lobby recoit la sienne
    Packet myPosition = makePosition({client.id, client.x, client.y, client.name});
    for (const Client& other : _clients) {
        if (other.lobby != lobby)
            continue;
        send(makePosition({other.id, other.x, other.y, other.name}), client.address);
        if (&other != &client) {
            send(myPosition, other.address);
            info(other, client.name + " a rejoint le lobby");
        }
    }
}

void Server::leaveLobby(Client& client) {
    if (client.lobby.empty())
        return;
    std::string lobby = client.lobby;
    client.lobby.clear();
    std::cout << client.name << " a quitte le lobby " << lobby << std::endl;
    info(client, "Tu as quitte " + lobby);
    sendToLobby(lobby, {PacketType::Info, client.name + " a quitte le lobby"});
}

void Server::move(Client& client, Action action) {
    switch (action) {
    case Action::Up:
        client.y = client.y >= STEP ? client.y - STEP : 0;
        std::cout << client.name << " -> (" << client.x << ", " << client.y << ")" << std::endl;
        break;
    case Action::Down:
        client.y = client.y + STEP <= MAP_HEIGHT ? client.y + STEP : MAP_HEIGHT;
        std::cout << client.name << " " << " -> (" << client.x << ", " << client.y << ")" << std::endl;
        break;
    case Action::Left:
        client.x = client.x >= STEP ? client.x - STEP : 0;
        std::cout << client.name << " -> (" << client.x << ", " << client.y << ")" << std::endl;
        break;
    case Action::Right:
        client.x = client.x + STEP <= MAP_WIDTH ? client.x + STEP : MAP_WIDTH;
        std::cout << client.name << " -> (" << client.x << ", " << client.y << ")" << std::endl;
        break;
    case Action::A:
        std::cout << client.name << " press A." << std::endl;
        break;
    case Action::B:
        std::cout << client.name << " press B." << std::endl;
        break;
    }
}

Server::Client* Server::find(const sockaddr_in& address) {
    for (Client& client : _clients) {
        if (client.address.sin_addr.s_addr == address.sin_addr.s_addr &&
            client.address.sin_port == address.sin_port)
            return &client;
    }
    return nullptr;
}
