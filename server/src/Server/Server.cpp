#include "Server.hpp"
#include "Protocol.hpp"
#include "Socket.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port) {
    ensureNetworkInit();
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket == INVALID_SOCK)
        throw std::runtime_error("socket() a echoue");

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        closeSocket(_socket);
        throw std::runtime_error("bind() a echoue sur le port " + std::to_string(port));
    }
    std::cout << "Serveur en ecoute sur le port " << port << std::endl;
}

Server::~Server() {
    closeSocket(_socket);
}

void Server::run() {
    uint8_t buffer[MAX_PACKET_SIZE];

    while (true) {
        if (!waitReadable(_socket, 100))
            continue;

        sockaddr_in from{};
        socket_len_t fromLength = sizeof(from);
        auto received = recvfrom(_socket, reinterpret_cast<char*>(buffer),
                                 static_cast<int>(sizeof(buffer)), 0,
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

    if (packet.type == PacketType::Connect) {
        if (client || packet.data.empty())
            return;
        if (nameTaken(packet.data)) {
            send({PacketType::Info, "Le pseudo " + packet.data + " est deja pris"}, from);
            return;
        }
        _clients.push_back({from, packet.data, _nextId++, 100, 100, ""});
        Client& added = _clients.back();
        std::cout << added.name << " s'est connecte (" << _clients.size() << " joueur(s))"
                  << std::endl;
        info(added, "Bienvenue " + added.name + " !");
        listLobbies(added);
        return;
    }
    if (!client)
    {
        std::cout << "Unknown client" << std::endl;
        return;
    }

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

    case PacketType::Position: {
        PlayerPosition position;
        if (client->lobby.empty() || client->dead || !readPosition(packet, position))
            return;
        client->x = std::min<uint16_t>(position.x, MAP_WIDTH);
        client->y = std::min<uint16_t>(position.y, MAP_HEIGHT);
        sendToLobby(client->lobby, makePosition({client->id, client->x, client->y, client->name}),
                    client);
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

    case PacketType::EnemySpawn: {
        auto it = _lobbies.find(client->lobby);
        if (it == _lobbies.end() || it->second.phase != Phase::Playing ||
            !it->second.hasHost || it->second.host != client->id)
            return;
        sendToLobby(client->lobby, packet, client);
        break;
    }
    case PacketType::Shoot: {
        Shoot shoot;
        if (client->lobby.empty() || client->dead || !readShoot(packet, shoot))
            return;
        shoot.id = client->id;
        shoot.name = client->name;
        sendToLobby(client->lobby, makeShoot(shoot), client);
        break;
    }

    case PacketType::Ready: {
        auto it = _lobbies.find(client->lobby);
        if (it == _lobbies.end() || it->second.phase == Phase::Playing)
            return;
        client->ready = (packet.data == "1");
        checkReady(client->lobby);
        break;
    }

    case PacketType::WaveClear: {
        auto it = _lobbies.find(client->lobby);
        if (it == _lobbies.end() || it->second.phase != Phase::Playing)
            return;
        it->second.phase = Phase::Intermission;
        for (Client& c : _clients) {
            if (c.lobby != client->lobby)
                continue;
            c.ready = false;
            c.dead = false;
        }
        sendToLobby(client->lobby, {PacketType::Intermission, std::string(1, static_cast<char>(it->second.wave))});
        break;
    }

    case PacketType::Death: {
        uint16_t maxHp;
        auto it = _lobbies.find(client->lobby);
        if (it == _lobbies.end() || it->second.phase != Phase::Playing ||
            client->dead || !readDeath(packet, maxHp))
            return;
        client->dead = true;
        const uint16_t damage = std::max<uint16_t>(1, maxHp / 5);   // 20% des PV max
        std::cout << client->name << " est mort (explosion : " << damage << ")" << std::endl;
        sendToLobby(client->lobby,
                    makePlayerDied({client->id, client->x, client->y, damage}), client);
        break;
    }


    default:
        // Info n'est envoye que par le serveur
        break;
    }
}

bool Server::nameTaken(const std::string& name) const {
    for (const Client& client : _clients) {
        if (client.name == name)
            return true;
    }
    return false;
}

void Server::send(const Packet& packet, const sockaddr_in& to) {
    std::vector<uint8_t> buffer = encode(packet);
    sendto(_socket, reinterpret_cast<const char*>(buffer.data()),
           static_cast<int>(buffer.size()), 0,
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
    std::map<std::string, int> counts;
    for (const Client& other : _clients) {
        if (!other.lobby.empty())
            counts[other.lobby]++;
    }

    std::vector<LobbyEntry> lobbies;
    for (const auto& [name, count] : counts) {
        if (lobbies.size() >= MAX_LISTED_LOBBIES)
            break;
        uint8_t state = 0;
        auto it = _lobbies.find(name);
        if (it != _lobbies.end())
            state = static_cast<uint8_t>(it->second.phase);
        lobbies.push_back({name, static_cast<uint8_t>(std::min(count, 255)), state});
    }
    send(makeLobbyData(lobbies), client.address);
}

void Server::createLobby(Client& client, const std::string& lobby) {
    if (lobby.empty()) {
        info(client, "Usage : /create <nom>");
        return;
    }
    if (lobby.size() > MAX_LOBBY_NAME) {
        info(client, "Nom trop long (" + std::to_string(MAX_LOBBY_NAME) + " max)");
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
    client.ready = false;
    client.dead = false;
    _lobbies[lobby] = LobbyState{};
    std::cout << client.name << " a cree le lobby " << lobby << std::endl;
    info(client, "Lobby " + lobby + " cree, tu es dedans");
    send({PacketType::LobbyJoined, lobby}, client.address);
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
    auto state = _lobbies.find(lobby);
    if (state != _lobbies.end() && state->second.phase == Phase::Playing) {
        info(client, "Partie deja lancee");
        return;
    }
    leaveLobby(client);
    client.lobby = lobby;
    client.x = 100;
    client.y = 100;
    client.ready = false;
    client.dead = false;
    std::cout << client.name << " a rejoint le lobby " << lobby << std::endl;
    info(client, "Tu as rejoint " + lobby);
    send({PacketType::LobbyJoined, lobby}, client.address);

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
    client.ready = false;
    client.dead = false;
    std::cout << client.name << " a quitte le lobby " << lobby << std::endl;
    info(client, "Tu as quitte " + lobby);
    sendToLobby(lobby, {PacketType::Info, client.name + " a quitte le lobby"});

    auto it = _lobbies.find(lobby);
    if (it == _lobbies.end())
        return;
    if (!lobbyExists(lobby))
        _lobbies.erase(it);
    else {
        if (it->second.hasHost && it->second.host == client.id)
            electHost(lobby);
        if (it->second.phase != Phase::Playing)
            checkReady(lobby);
    }
}

void Server::electHost(const std::string& lobby) {
    LobbyState& state = _lobbies[lobby];
    Client* chosen = nullptr;
    for (Client& c : _clients) {
        if (c.lobby != lobby)
            continue;
        if (state.hasHost && c.id == state.host) {
            chosen = &c;
            break;
        }
        if (!chosen)
            chosen = &c;
    }
    if (!chosen) {
        state.hasHost = false;
        return;
    }
    state.host = chosen->id;
    state.hasHost = true;
    for (const Client& c : _clients) {
        if (c.lobby == lobby)
            send({PacketType::Host, c.id == state.host ? "1" : "0"}, c.address);
    }
}

void Server::resetReady(const std::string& lobby) {
    for (Client& c : _clients) {
        if (c.lobby == lobby)
            c.ready = false;
    }
}

void Server::checkReady(const std::string& lobby) {
    int total = 0;
    int ready = 0;
    for (Client& c : _clients) {
        if (c.lobby != lobby)
            continue;
        total++;
        if (c.ready)
            ready++;
    }
    if (total == 0)
        return;
    sendToLobby(lobby, {PacketType::Info,
                        "Prets : " + std::to_string(ready) + "/" + std::to_string(total)});
    if (ready != total)
        return;

    LobbyState& state = _lobbies[lobby];
    state.phase = Phase::Playing;
    state.wave++;
    resetReady(lobby);
    electHost(lobby);
    std::cout << "Lobby " << lobby << " : vague " << state.wave << std::endl;
    sendToLobby(lobby, {PacketType::WaveStart, std::string(1, static_cast<char>(state.wave))});
}

Server::Client* Server::find(const sockaddr_in& address) {
    for (Client& client : _clients) {
        if (client.address.sin_addr.s_addr == address.sin_addr.s_addr &&
            client.address.sin_port == address.sin_port)
            return &client;
    }
    return nullptr;
}
