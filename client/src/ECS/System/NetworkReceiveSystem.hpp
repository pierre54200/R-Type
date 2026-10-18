#pragma once
#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "Registery.hpp"
#include "../Modules/RemotePlayer.hpp"
#include "../Modules/Transform.hpp"
#include "Protocol.hpp"


class   NetworkReceiveSystem : public ECS::System
{
    public:
        using   ReceiveFunction =   std::function<std::optional<Packet>()>;
        using   SpawnFunction   =   std::function<Entity(ECS::Registery&, const PlayerPosition&)>;

        void    setReceiver(ReceiveFunction receive)    { _receive = std::move(receive); }
        void    setSpawner(SpawnFunction spawn)         { _spawn = std::move(spawn); }
        void    setLocalName(std::string name)          { _localName = std::move(name); }

        void    update(ECS::Registery& registery, float) override
        {
            if (!_receive)
                return;
            for (std::size_t i = 0; i < MAX_PACKETS_PER_FRAME; ++i)
            {
                std::optional<Packet>   packet = _receive();
                if (!packet)
                    break;
                handle(registery, *packet);
            }
        }

    private:
        static constexpr std::size_t    MAX_PACKETS_PER_FRAME   =   256;

        void    handle(ECS::Registery& registery, const Packet& packet)
        {
            switch (packet.type)
            {
                case PacketType::Position:
                {
                    PlayerPosition  position;
                    if (readPosition(packet, position))
                        applyPosition(registery, position);
                    break;
                }
                case PacketType::Info:
                    std::cout << "[serveur] " << packet.data << std::endl;
                    break;
                case PacketType::Message:
                    std::cout << packet.data << std::endl;
                    break;
                default:
                    break;
            }
        }

        void    applyPosition(ECS::Registery& registery, const PlayerPosition& position)
        {
            if (position.name == _localName)
                return;

            auto    known = _players.find(position.id);
            if (known != _players.end() && !registery.hasModule<RemotePlayer>(known->second))
            {
                _players.erase(known);
                known = _players.end();
            }

            if (known == _players.end())
            {
                if (!_spawn)
                    return;
                Entity  entity = _spawn(registery, position);
                registery.addModule(entity, RemotePlayer{position.id, position.name});
                _players[position.id] = entity;
                return;
            }

            registery.getModule<RemotePlayer>(known->second).name = position.name;
            Transform&  transform = registery.getModule<Transform>(known->second);
            transform.position.x = static_cast<float>(position.x);
            transform.position.y = static_cast<float>(position.y);
        }

        ReceiveFunction                         _receive;
        SpawnFunction                           _spawn;
        std::string                             _localName;
        std::unordered_map<uint8_t, Entity>     _players;
};
