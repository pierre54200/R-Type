#pragma once

#include <memory>
#include <SFML/Graphics.hpp>

template<typename T>
struct Window
{
    std::shared_ptr<T>   window;
    sf::Color                           clearColor  = sf::Color::Black;
    bool                                shouldClose = false;
};
