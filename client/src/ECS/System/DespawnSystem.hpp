#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/DespawnByRange.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>

class DespawnSystem : public ECS::System
{
public:
    static inline Entity screen = 0;

    void update(ECS::Registery& reg, float) override
    {
        auto entities = this->_entities;
        for (auto e : entities)
        {
            auto& t1 = reg.getModule<Transform>(e);
            auto& range = reg.getModule<DespawnByRange>(e);
            auto& t2 = range.spawnPosition;

            float dx = t1.position.x - t2.x;
            float dy = t1.position.y - t2.y;

            if (dx * dx + dy * dy > range.maxRange * range.maxRange)
            {
                reg.destroyEntity(e);
                std::cout << "Destroying Entity, Entity Count: " << reg.getLivingCount() << std::endl;
            }
        }
    }
};
