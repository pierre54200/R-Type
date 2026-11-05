#pragma once

#include <cstdint>
#include <SFML/Graphics.hpp>

struct FrontEnd
{
    bool        isDrawable  = true;
    bool        isInvisible = false;
    uint32_t    layer       = 0;
    sf::Color   color       = sf::Color::White;
    bool        screenSpace = false;
};
