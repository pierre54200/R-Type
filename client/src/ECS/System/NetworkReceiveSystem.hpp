#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ECS/Modules/SceneTag.hpp"
#include "Registery.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/RemotePlayer.hpp"
#include "../Modules/Transform.hpp"
#include "../Utils/SpawnEntities.hpp"
#include "Protocol.hpp"


class   NetworkReceiveSystem : public ECS::System
{
    public:
        using   ReceiveFunction     =   std::function<std::optional<Packet>()>;
        using   SpawnFunction       =   std::function<Entity(ECS::Registery&, const PlayerPosition&)>;
        using   WaveFunction        =   std::function<void(uint8_t)>;
        using   ExplosionFunction   =   std::function<void(float)>;
        using   InfoFunction        =   std::function<void(const std::string&)>;
        using   LobbyDataFunction   =   std::function<void(const std::vector<LobbyEntry>&)>;
        using   LobbyJoinedFunction =   std::function<void(const std::string&)>;
        using   HostFunction          = std::function<void(bool)>;
        using   EnemySpawnFunction    = std::function<void(float, float)>;

        void    setReceiver(ReceiveFunction receive)        { _receive = std::move(receive); }
        void    setSpawner(SpawnFunction spawn)             { _spawn = std::move(spawn); }
        void    setLocalName(std::string name)              { _localName = std::move(name); }
        void    setOnWaveStart(WaveFunction f)              { _onWaveStart = std::move(f); }
        void    setOnIntermission(WaveFunction f)           { _onIntermission = std::move(f); }
        void    setOnExplosion(ExplosionFunction f)         { _onExplosion = std::move(f); }
        void    setOnInfo(InfoFunction f)                   { _onInfo = std::move(f); }
        void    setOnLobbyData(LobbyDataFunction f)         { _onLobbyData = std::move(f); }
        void    setOnLobbyJoined(LobbyJoinedFunction f)     { _onLobbyJoined = std::move(f); }
        void    setOnHost(HostFunction f)                   { _onHost = std::move(f); }
        void    setOnEnemySpawn(EnemySpawnFunction f)       { _onEnemySpawn = std::move(f); }

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
                    if (_onInfo)
                        _onInfo(packet.data);
                    break;
                case PacketType::Message:
                    std::cout << packet.data << std::endl;
                    break;
                case PacketType::Shoot:
                {
                    Shoot   s;
                    if (readShoot(packet, s))
                        applyBullet(registery, s);
                    break;
                }
                case PacketType::WaveStart:
                    if (!packet.data.empty() && _onWaveStart)
                        _onWaveStart(static_cast<uint8_t>(packet.data[0]));
                    break;
                case PacketType::Intermission:
                    if (!packet.data.empty() && _onIntermission)
                        _onIntermission(static_cast<uint8_t>(packet.data[0]));
                    break;
                case PacketType::PlayerDied:
                {
                    DeathEvent  death;
                    if (readPlayerDied(packet, death))
                        applyDeath(registery, death);
                    break;
                }
                case PacketType::LobbyData:
                {
                    std::vector<LobbyEntry> lobbies;
                    if (readLobbyData(packet, lobbies) && _onLobbyData)
                        _onLobbyData(lobbies);
                    break;
                }
                case PacketType::LobbyJoined:
                    if (_onLobbyJoined)
                        _onLobbyJoined(packet.data);
                    break;
                case PacketType::Host:
                    if (!packet.data.empty() && _onHost)
                        _onHost(packet.data[0] == '1');
                    break;
                case PacketType::EnemySpawn:
                {
                    PlayerPosition  spawn;
                    if (readEnemySpawn(packet, spawn) && _onEnemySpawn)
                        _onEnemySpawn(static_cast<float>(spawn.x), static_cast<float>(spawn.y));
                    break;
                }
                default:
                    break;
            }
        }

        void    applyPosition(ECS::Registery& registery, const PlayerPosition& position)
        {
            if (position.name == _localName)
                return;

            auto    known = _players.find(position.id);
            if (known != _players.end() && !registery.hasModule<RemoteEntity>(known->second))
            {
                _players.erase(known);
                known = _players.end();
            }

            if (known == _players.end())
            {
                if (!_spawn)
                    return;
                Entity  entity = _spawn(registery, position);
                registery.addModule(entity, RemoteEntity{position.id, position.name});
                _players[position.id] = entity;
                return;
            }

            auto&   remote = registery.getModule<RemoteEntity>(known->second);
            remote.name = position.name;
            if (remote.dead)
            {
                remote.dead = false;
                registery.getModule<FrontEnd>(known->second).isInvisible = false;
            }
            Transform&  transform = registery.getModule<Transform>(known->second);
            transform.position.x = static_cast<float>(position.x);
            transform.position.y = static_cast<float>(position.y);
        }

        void    applyBullet(ECS::Registery& registery, const Shoot& shoot)
        {
            if (shoot.name == _localName)
                return;

            auto    known = _players.find(shoot.id);
            if (known == _players.end())
                return;
            if (!registery.hasModule<RemoteEntity>(known->second))
            {
                _players.erase(known);
                return;
            }
            if (!registery.hasModule<SceneTag>(known->second))
                return;
            Spawn::createBullet(registery,
                {static_cast<float>(shoot.x), static_cast<float>(shoot.y), 0.f},
                {static_cast<float>(shoot.dirx), static_cast<float>(shoot.diry), 0.f},
                registery.getModule<SceneTag>(known->second).scene,
                static_cast<float>(shoot.damage));
        }

        void    applyDeath(ECS::Registery& registery, const DeathEvent& death)
        {
            auto    known = _players.find(death.id);
            if (known != _players.end() && registery.hasModule<RemoteEntity>(known->second))
            {
                registery.getModule<FrontEnd>(known->second).isInvisible = true;
                registery.getModule<RemoteEntity>(known->second).dead = true;
            }
            if (_onExplosion)
                _onExplosion(static_cast<float>(death.damage));
        }

        ReceiveFunction                         _receive;
        SpawnFunction                           _spawn;
        WaveFunction                            _onWaveStart;
        WaveFunction                            _onIntermission;
        ExplosionFunction                       _onExplosion;
        InfoFunction                            _onInfo;
        LobbyDataFunction                       _onLobbyData;
        LobbyJoinedFunction                     _onLobbyJoined;
        HostFunction                            _onHost;
        EnemySpawnFunction                      _onEnemySpawn;
        std::string                             _localName;
        std::unordered_map<uint8_t, Entity>     _players;
};
