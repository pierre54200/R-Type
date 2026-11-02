#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Controls.hpp"
#include "../Modules/PlayerStat.hpp"
#include "../Modules/RigidBody.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>

class PlayerControlSystem : public ECS::System
{
public:
    static inline Entity    controls    =   0;
    void update(ECS::Registery& reg, float) override
    {
        for (auto e : _entities)
        {
            float dx = 0.f;
            float dy = 0.f;

            auto& contr = reg.getModule<Controls<sf::Keyboard::Key>>(controls);
            if (sf::Keyboard::isKeyPressed(contr.getKey("up"))) dy -= 1.f;
            if (sf::Keyboard::isKeyPressed(contr.getKey("down"))) dy += 1.f;
            if (sf::Keyboard::isKeyPressed(contr.getKey("left"))) dx -= 1.f;
            if (sf::Keyboard::isKeyPressed(contr.getKey("right"))) dx += 1.f;

            if (dx != 0.f && dy != 0.f)
            {
                dx *= 0.7071f;
                dy *= 0.7071f;
            }

            auto&   ps = reg.getModule<PlayerStat>(e);
            float speed = ps.speed;
            reg.getModule<RigidBody>(e).velocity =
                Math::Vec3<float>(dx * speed, dy * speed, 0.f);
        }
    }
};
