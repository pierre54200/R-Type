#pragma once

#include "ECS/Modules/SceneManager.hpp"
#include "ECS/Modules/SpawnerSettings.hpp"
#include "ECS/Utils/SpawnEntities.hpp"
#include "ECS/Utils/Wave.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"
#include "ECS/System/NetworkSendSystem.hpp"
#include "Protocol.hpp"

#include <algorithm>
#include <cstdint>

class SpawnerSystem : public ECS::System
{
public:
    static inline Entity manager = 0;

    void update(ECS::Registery& reg, float) override
    {
        if (!Wave::isHost || !Wave::canSpawn())
            return;

        auto& sm = reg.getModule<SceneManager>(manager);

        for (auto& e : this->_entities)
        {
            if (!Wave::canSpawn())
                break;

            auto& settings = reg.getModule<SpawnerSettings>(e);
            if (!settings.spawnClock.isReady())
                continue;

            Math::Vec3<float> pos = randomVec3(settings.minPos, settings.maxPos);

            Spawn::createEnemy(reg, pos, sm.id("game"));
            Wave::spawned++;
            NetworkSendSystem::queue.push_back(makeEnemySpawn({
                0,
                static_cast<uint16_t>(std::clamp(pos.x, 0.f, 65535.f) + 0.5f),
                static_cast<uint16_t>(std::clamp(pos.y, 0.f, 65535.f) + 0.5f),
                ""}));
            settings.spawnClock.setTargetTime(
                std::max(.25f, settings.time - Wave::number * .05f));
        }
    }

private:
    static float randomFloat(float a, float b)
    {
        auto [lo, hi] = std::minmax(a, b);
        uint32_t range = static_cast<uint32_t>(hi - lo);
        return lo + static_cast<float>(Wave::rng() % (range + 1));
    }

    static Math::Vec3<float> randomVec3(const Math::Vec3<float>& min,
                                        const Math::Vec3<float>& max)
    {
        return Math::Vec3<float>{
            randomFloat(min.x, max.x),
            randomFloat(min.y, max.y),
            randomFloat(min.z, max.z)
        };
    }
};
