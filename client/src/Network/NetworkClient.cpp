#include "Network/NetworkClient.hpp"

#include <stdexcept>
#include <vector>

NetworkClient::NetworkClient(const std::string& ip, int port, const std::string& name)
{
    if (port < 1 || port > 65535)
        throw std::invalid_argument("port invalide : " + std::to_string(port));
    if (name.empty())
        throw std::invalid_argument("pseudo vide");

    ensureNetworkInit();

    _server.sin_family = AF_INET;
    _server.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &_server.sin_addr) != 1)
        throw std::invalid_argument("adresse IP invalide : " + ip);

    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket == INVALID_SOCK)
        throw std::runtime_error("socket() failed");
    if (!setNonBlocking(_socket))
    {
        closeSocket(_socket);
        throw std::runtime_error("impossible de passer la socket en non bloquant");
    }

    send({PacketType::Connect, name});
}

NetworkClient::~NetworkClient()
{
    send({PacketType::Disconnect, ""});
    closeSocket(_socket);
}

void    NetworkClient::send(const Packet& packet)
{
    std::vector<uint8_t>    buffer = encode(packet);
    sendto(_socket, reinterpret_cast<const char*>(buffer.data()),
           static_cast<int>(buffer.size()), 0,
           reinterpret_cast<const sockaddr*>(&_server), sizeof(_server));
}

std::optional<Packet>   NetworkClient::receive()
{
    uint8_t buffer[MAX_PACKET_SIZE];

    while (true)
    {
        sockaddr_in     from{};
        socket_len_t    fromLength = sizeof(from);
        auto            received = recvfrom(_socket, reinterpret_cast<char*>(buffer),
                                            static_cast<int>(sizeof(buffer)), 0,
                                            reinterpret_cast<sockaddr*>(&from), &fromLength);
        if (received < 0)
            return std::nullopt;
        if (from.sin_addr.s_addr != _server.sin_addr.s_addr || from.sin_port != _server.sin_port)
            continue;
        Packet  packet;
        if (decode(buffer, static_cast<std::size_t>(received), packet))
            return packet;
    }
}
