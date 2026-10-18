#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/PlayerControl.hpp"
#include "../Modules/RigidBody.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/DespawnByRange.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/HitBox.hpp"
#include "../Modules/Window.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdint>

class PlayerControlSystem : public ECS::System
{
public:
    static inline Entity screen = 0;
    void update(ECS::Registery& reg, float) override
    {
        auto& window = reg.getModule<Window<sf::RenderWindow>>(screen).window;
        if (!window->hasFocus())
            return;
        for (auto e : _entities)
        {
            float dx = 0.f;
            float dy = 0.f;

            auto& controls = reg.getModule<PlayerControl<sf::Keyboard::Key>>(e);
            if (sf::Keyboard::isKeyPressed(controls.getKey("up"))) dy -= 1.f;
            if (sf::Keyboard::isKeyPressed(controls.getKey("down"))) dy += 1.f;
            if (sf::Keyboard::isKeyPressed(controls.getKey("left"))) dx -= 1.f;
            if (sf::Keyboard::isKeyPressed(controls.getKey("right"))) dx += 1.f;

            if (dx != 0.f && dy != 0.f)
            {
                dx *= 0.7071f;
                dy *= 0.7071f;
            }

            float speed = reg.getModule<PlayerControl<sf::Keyboard::Key>>(e).speed;
            reg.getModule<RigidBody>(e).velocity =
                Math::Vec3<float>(dx * speed, dy * speed, 0.f);

            if (sf::Keyboard::isKeyPressed(controls.getKey("shoot")))
            {
                if (controls.shootCooldown.isReady())
                {
                    std::cout << "Shooting" << std::endl;
                    this->creatBullet(reg, reg.getModule<Transform>(e).position, reg.getModule<Transform>(e).size);
                    controls.shootCooldown.setTargetTime(0.25f);
                }
            }
        }
    }
private:
    void    creatBullet(ECS::Registery& reg, Math::Vec3<float>  pos, Math::Vec3<float>  size)
    {
        Entity  bullet = reg.createEntity();

        reg.addModule(bullet, Transform{{pos.x + size.x, pos.y + size.y / 2, 0.f}, {size.x / 10, size.y / 10, 0.f}})
            .addModule(bullet, HitBox{
                .topLeft = {0.f, 0.f, 0.f},
                .size = {size.x / 10, size.y / 10, 0.f},
                .mask = std::set<uint32_t>{2},
                .destroyOnHit = true})
            .addModule(bullet, RigidBody{{100.f, 0.f, 0.f}, 9.8f, std::set<uint32_t>{0}})
            .addModule(bullet, Health{5.f, true, 0.f})
            .addModule(bullet, DespawnByRange{{pos.x, pos.y, 0.f}})
            .addModule(bullet, FrontEnd{true, false, 0, sf::Color::Yellow});
    }
};
