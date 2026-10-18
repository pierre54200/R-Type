#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Window.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

class UpdateDisplaySystem:  public  ECS::System
{
    public:
        static inline Entity screen = 0;

        void update(ECS::Registery& reg, float) override
        {
            auto& win = reg.getModule<Window<sf::RenderWindow>>(screen);
            win.window->display();
        }
};
