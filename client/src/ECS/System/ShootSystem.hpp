#pragma once

#include "ECS/Utils/SpawnEntities.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"
#include "TargetSystem.hpp"
#include "../Modules/PlayerStat.hpp"
#include "../Modules/SceneTag.hpp"
#include "../Modules/Transform.hpp"

#include <cmath>
#include <limits>
#include <vector>

class ShootSystem : public ECS::System
{
public:
    void update(ECS::Registery& reg, float) override
    {
        if (TargetSystem::targets[1].empty())
            return;

        struct Shot {
            Math::Vec3<float> origin,
            vel;
            SceneId scene;
        };
        std::vector<Shot> shots;
        for (auto e : _entities)
        {
            auto& stat = reg.getModule<PlayerStat>(e);
            if (!stat.shootCooldown.isReady())
                continue;

            auto& tr = reg.getModule<Transform>(e);

            const float cx = tr.position.x + tr.size.x * 0.5f;
            const float cy = tr.position.y + tr.size.y * 0.5f;

            float bestDist2 = std::numeric_limits<float>::max();
            float dx = 0.f, dy = 0.f;

            for (const auto& t : TargetSystem::targets[1])
            {
                float ddx = (t.position.x + t.size.x * 0.5f) - cx;
                float ddy = (t.position.y + t.size.y * 0.5f) - cy;
                float d2  = ddx * ddx + ddy * ddy;
                if (d2 < bestDist2)
                {
                    bestDist2 = d2;
                    dx = ddx;
                    dy = ddy;
                }
            }

            if (bestDist2 < 0.0001f)
                continue;

            const float len         = std::sqrt(bestDist2);
            const float bulletSpeed = 400.f;

            shots.push_back({
                {cx, cy, 0.f},
                {dx / len * bulletSpeed, dy / len * bulletSpeed, 0.f},
                reg.getModule<SceneTag>(e).scene
            });

            stat.shootCooldown.setTargetTime(.1f);
        }

        for (const auto& s : shots)
            Spawn::createBullet(reg, s.origin, {10.f, 10.f, 0.f}, s.vel, s.scene);
    }
};
