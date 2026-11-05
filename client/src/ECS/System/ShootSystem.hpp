#pragma once

#include "ECS/Modules/NetworkSync.hpp"
#include "ECS/System/NetworkSendSystem.hpp"
#include "ECS/Utils/SpawnEntities.hpp"
#include "Protocol.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"
#include "TargetSystem.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/PlayerStat.hpp"
#include "../Modules/SceneTag.hpp"
#include "../Modules/Transform.hpp"

#include <algorithm>
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
        struct Shot {
            Math::Vec3<float> origin;
            Math::Vec3<float> vel;
            SceneId           scene;
            std::uint8_t      id;
            std::string       name;
            float             damage;
        };
        std::vector<Shot> shots;
        for (auto e : _entities)
        {
            auto& hp   = reg.getModule<Health>(e);
            auto& stat = reg.getModule<PlayerStat>(e);
            auto& sync = reg.getModule<NetworkSync>(e);

            bool ready = stat.shootCooldown.isReady();
            if (hp.point <= 0.f || !ready)
                continue;

            auto& tr = reg.getModule<Transform>(e);

            float cx = tr.position.x + tr.size.x * 0.5f;
            float cy = tr.position.y + tr.size.y * 0.5f;

            float bestDist2 = std::numeric_limits<float>::max();
            float dx = 0.f;
            float dy = 0.f;

            for (auto& t : TargetSystem::targets[1])
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

            float len         = std::sqrt(bestDist2);
            float bulletSpeed = stat.bulletSpeed;

            shots.push_back({
                {cx, cy, 0.f},
                {dx / len * bulletSpeed, dy / len * bulletSpeed, 0.f},
                reg.getModule<SceneTag>(e).scene,
                sync.id,
                sync.name,
                stat.bulletDamage
            });

            stat.shootCooldown.setTargetTime(stat.shootDelay);
        }

        for (const auto& s : shots) {
            Spawn::createBullet(reg, s.origin, s.vel, s.scene, s.damage);
            NetworkSendSystem::queue.push_back(makeShoot(
                Shoot{
                    .id = s.id,
                    .x = static_cast<uint16_t>(std::clamp(s.origin.x, 0.f, 65535.f) + 0.5f),
                    .y = static_cast<uint16_t>(std::clamp(s.origin.y, 0.f, 65535.f) + 0.5f),
                    .dirx = static_cast<int16_t>(std::clamp<long>(std::lround(s.vel.x), -32768, 32767)),
                    .diry = static_cast<int16_t>(std::clamp<long>(std::lround(s.vel.y), -32768, 32767)),
                    .name = s.name,
                    .damage = static_cast<uint8_t>(std::clamp(s.damage, 1.f, 255.f)),
                })
            );
        }
    }
};
