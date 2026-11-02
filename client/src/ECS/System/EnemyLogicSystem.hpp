#pragma once
#include "Registery.hpp"
#include "SystemManager.hpp"
#include "TargetSystem.hpp"
#include "../Modules/EnemyStat.hpp"
#include "../Modules/RigidBody.hpp"
#include "../Modules/Transform.hpp"
#include <cmath>
#include <limits>

class EnemyLogicSystem : public ECS::System
{
public:
    void update(ECS::Registery& reg, float) override
    {
        if (TargetSystem::targets[0].empty())
            return;

        for (auto e : _entities)
        {
            auto& pos = reg.getModule<Transform>(e).position;
            auto& rb  = reg.getModule<RigidBody>(e);

            float bestDist2 = std::numeric_limits<float>::max();
            float dx = 0.f, dy = 0.f;

            for (const auto& t : TargetSystem::targets[0])
            {
                float ddx = t.position.x - pos.x;
                float ddy = t.position.y - pos.y;
                float d2  = ddx * ddx + ddy * ddy;
                if (d2 < bestDist2)
                {
                    bestDist2 = d2;
                    dx = ddx;
                    dy = ddy;
                }
            }

            if (bestDist2 < 0.0001f)
            {
                rb.velocity = {0.f, 0.f, 0.f};
                continue;
            }

            const float len   = std::sqrt(bestDist2);
            rb.velocity = {dx / len * reg.getModule<EnemyStat>(e).speed, dy / len * reg.getModule<EnemyStat>(e).speed, 0.f};
        }
    }
};
