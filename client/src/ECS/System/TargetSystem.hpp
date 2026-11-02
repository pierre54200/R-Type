#pragma once
#include "ECS/Modules/Targetable.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"
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
            targets[reg.getModule<Targetable>(e).id].push_back(reg.getModule<Transform>(e));
    }
};
