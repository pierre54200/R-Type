#pragma once
#include "ECS/Modules/Targetable.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/RemotePlayer.hpp"
#include "../Modules/Transform.hpp"
#include <cstdint>
#include <map>
#include <vector>

class TargetSystem : public ECS::System
{
public:
    static inline std::map<std::uint8_t,    std::vector<Transform>> targets;

    void update(ECS::Registery& reg, float) override
    {
        targets.clear();
        for (auto e : _entities)
        {
            if (reg.hasModule<Health>(e) && reg.getModule<Health>(e).point <= 0.f)
                continue;
            if (reg.hasModule<RemoteEntity>(e) && reg.getModule<RemoteEntity>(e).dead)
                continue;
            targets[reg.getModule<Targetable>(e).id].push_back(reg.getModule<Transform>(e));
        }
    }
};
