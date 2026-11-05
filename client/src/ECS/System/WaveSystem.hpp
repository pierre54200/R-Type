#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "Protocol.hpp"
#include "../Utils/Wave.hpp"

#include <functional>

class WaveSystem : public ECS::System
{
public:
    static inline std::function<void(const Packet&)> send;

    void update(ECS::Registery&, float) override
    {
        if (!Wave::running || Wave::spawned < Wave::toSpawn || !_entities.empty())
            return;
        Wave::running = false;
        if (send)
            send({PacketType::WaveClear, ""});
    }
};