#pragma once

#include <SFML/Graphics.hpp>
#include <string>

// Text drawn centered inside the entity's Transform rectangle (see DisplaySystem)
struct Label
{
    std::string  text;
    unsigned int size  = 20;
    sf::Color    color = sf::Color::White;
};