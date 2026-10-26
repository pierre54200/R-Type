#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"

#include "../Modules/Transform.hpp"
#include "../Modules/Function.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

class ClickSystem : public ECS::System
{
public:

    void update(ECS::Registery& reg, float) override
    {
        sf::Vector2i    mousePos    =   sf::Mouse::getPosition();
        auto            view        =   reg.view<Transform, Function>(_entities);

        view.each([&](Entity, Transform& t, Function& f)
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
                t.position.x < mousePos.x && mousePos.x < t.position.x + t.size.x
            &&  t.position.y < mousePos.y && mousePos.y < t.position.y + t.size.y)
            f.function(reg);
        });
    }
};
