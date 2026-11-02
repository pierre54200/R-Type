#pragma once

#include "ECS/Modules/SceneManager.hpp"
#include "ECS/Modules/SpawnerSettings.hpp"
#include "ECS/Utils/SpawnEntities.hpp"
#include "Registery.hpp"
#include "SystemManager.hpp"

#include <algorithm>
#include <cstdlib>

class SpawnerSystem : public ECS::System
{
public:
    static inline Entity manager = 0;

    void update(ECS::Registery& reg, float) override
    {
        std::srand(time(0));

        auto& sm = reg.getModule<SceneManager>(manager);

        for (auto& e : this->_entities)
        {
            auto& settings = reg.getModule<SpawnerSettings>(e);

            if (!settings.spawnClock.isReady())
                continue;

            Math::Vec3<float> pos  = randomVec3(settings.minPos,  settings.maxPos);
            Math::Vec3<float> size = randomVec3(settings.minSize, settings.maxSize);

            Spawn::createEnemy(reg, pos, size, sm.id("game"));
            settings.spawnClock.setTargetTime(settings.time);
        }
    }

private:
    static float randomFloat(float a, float b)
    {
        auto [lo, hi] = std::minmax(a, b);

        int range = static_cast<int>(hi - lo);
        int value = std::rand() % (range + 1);

        return lo + static_cast<float>(value);
;
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
