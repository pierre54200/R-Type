#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/RigidBody.hpp"
#include "../Modules/Transform.hpp"

class MovementSystem : public ECS::System
{
public:
    void update(ECS::Registery& reg, float dt) override
    {
        for (auto e : _entities)
        {
            auto& rb  = reg.getModule<RigidBody>(e);
            if (rb.isStatic)
                continue;

            auto& pos = reg.getModule<Transform>(e).position;
            pos += rb.velocity * dt;
        }
    }
};
