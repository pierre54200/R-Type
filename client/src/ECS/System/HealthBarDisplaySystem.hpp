#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/Window.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

class HealtBarDisplaySystem: public ECS::System
{
    public:
        static inline Entity screen = 0;

        void update(ECS::Registery& reg, float) override
        {
            auto& win = reg.getModule<Window<sf::RenderWindow>>(screen);

            for (auto &e: this->_entities)
            {
                auto&   health      =   reg.getModule<Health>(e);
                if (health.hurt)
                    continue;
                auto&   transform   =   reg.getModule<Transform>(e);
                auto&   front       =   reg.getModule<FrontEnd>(e);
                if (!front.isDrawable || front.isInvisible)
                    return;

                if (health.point < 0)
                    health.point = 0;
                sf::RectangleShape  left({transform.size.x * (health.point / health.maxPoint), transform.size.y * 0.15f});
                sf::RectangleShape  missing({transform.size.x * (1 - (health.point / health.maxPoint)), transform.size.y * 0.15f});
                left.setPosition(transform.position.x, transform.position.y - transform.size.y * 0.20f);
                missing.setPosition(transform.position.x + transform.size.x * (health.point / health.maxPoint), transform.position.y - transform.size.y * 0.20f);
                left.setFillColor(sf::Color::Green);
                missing.setFillColor(sf::Color::Red);
                win.window->draw(left);
                win.window->draw(missing);
            }
        }
};
