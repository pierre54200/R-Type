#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"

#include "../Modules/FrontEnd.hpp"
#include "../Modules/Function.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/Window.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>

class ClickSystem : public ECS::System
{
public:
    static inline Entity screen = 0;

    void update(ECS::Registery& reg, float) override
    {
        auto& win = *reg.getModule<Window<sf::RenderWindow>>(screen).window;

        bool pressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        bool clicked = pressed && !_wasPressed && win.hasFocus();
        _wasPressed = pressed;
        if (!clicked)
            return;

        sf::Vector2i mouse = sf::Mouse::getPosition(win);
        reg.view<Transform, Function>(_entities).each([&](Entity e, Transform& t, Function& f)
        {
            if (reg.hasModule<FrontEnd>(e) && reg.getModule<FrontEnd>(e).isInvisible)
                return;
            if (t.position.x < mouse.x && mouse.x < t.position.x + t.size.x &&
                t.position.y < mouse.y && mouse.y < t.position.y + t.size.y)
                f.function(reg);
        });
    }

private:
    bool _wasPressed = false;
};