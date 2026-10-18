#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Window.hpp"
#include "../Modules/EventQueue.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

class EventSystem : public ECS::System
{
public:
    static inline Entity screen = 0;

    void update(ECS::Registery& reg, float) override
    {
        auto& win = reg.getModule<Window<sf::RenderWindow>>(screen);
        auto& q   = reg.getModule<EventQueue>(screen);

        q.events.clear();
        sf::Event ev;
        while (win.window->pollEvent(ev))
        {
            if (ev.type == sf::Event::Closed)
            {
                win.window->close();
                reg.stop();
            }
            q.events.push_back(ev);
        }
    }
};
