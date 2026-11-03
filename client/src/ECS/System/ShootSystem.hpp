#pragma once

#include "ECS/Modules/NetworkSync.hpp"
#include "ECS/System/NetworkSendSystem.hpp"
#include "ECS/Utils/SpawnEntities.hpp"
#include "Protocol.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"
#include "TargetSystem.hpp"
#include "../Modules/PlayerStat.hpp"
#include "../Modules/SceneTag.hpp"
#include "../Modules/Transform.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
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
            std::uint8_t    id;
            std::string     name;
        };
        std::vector<Shot> shots;
        for (auto e : _entities)
        {
            auto& stat = reg.getModule<PlayerStat>(e);
            auto& sync = reg.getModule<NetworkSync>(e);
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
                reg.getModule<SceneTag>(e).scene,
                sync.id,
                sync.name
            });

            stat.shootCooldown.setTargetTime(.1f);
        }

        for (const auto& s : shots) {
            Spawn::createBullet(reg, s.origin, s.vel, s.scene);
            NetworkSendSystem::queue.push_back(makeShoot(
                Shoot{
                    .id = s.id,
                    .x = static_cast<uint16_t>(std::clamp(s.origin.x, 0.f, 65535.f) + 0.5f),
                    .y = static_cast<uint16_t>(std::clamp(s.origin.y, 0.f, 65535.f) + 0.5f),
                    .dirx = static_cast<int16_t>(std::lround(s.vel.x)),
                    .diry = static_cast<int16_t>(std::lround(s.vel.y)),
                    .name = s.name,
                })
            );
        }
    }
};
