#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "TargetSystem.hpp"
#include "../Modules/Window.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

class CameraSystem : public ECS::System
{
public:
    static inline Entity screen    = 0;
    static inline float  mapWidth  = 4000.f;
    static inline float  mapHeight = 4000.f;
    static inline float  padding   = 200.f;
    static inline float  smoothing = 6.f;

    void update(ECS::Registery& reg, float dt) override
    {
        auto& win = *reg.getModule<Window<sf::RenderWindow>>(screen).window;
        sf::Vector2f winSize(win.getSize());

        auto& players = TargetSystem::targets[0];
        if (players.empty())
        {
            win.setView(win.getDefaultView());
            _init = false;
            return;
        }

        float minX = std::numeric_limits<float>::max();
        float minY = minX;
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = maxX;
        for (auto& t : players)
        {
            minX = std::min(minX, t.position.x);
            minY = std::min(minY, t.position.y);
            maxX = std::max(maxX, t.position.x + t.size.x);
            maxY = std::max(maxY, t.position.y + t.size.y);
        }

        float needW    = (maxX - minX) + 2.f * padding;
        float needH    = (maxY - minY) + 2.f * padding;
        float maxScale = std::min(mapWidth / winSize.x, mapHeight / winSize.y);
        float scale    = std::min(std::max({needW / winSize.x, needH / winSize.y, 1.f}), maxScale);
        sf::Vector2f center{(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};

        if (!_init)
        {
            _scale  = scale;
            _center = center;
            _init   = true;
        }
        else
        {
            float k = 1.f - std::exp(-smoothing * dt);
            _scale  += (scale - _scale) * k;
            _center += (center - _center) * k;
        }

        sf::Vector2f size(winSize.x * _scale, winSize.y * _scale);
        sf::Vector2f c = _center;
        float halfW = size.x * 0.5f;
        float halfH = size.y * 0.5f;
        c.x = std::clamp(c.x, halfW, std::max(halfW, mapWidth  - halfW));
        c.y = std::clamp(c.y, halfH, std::max(halfH, mapHeight - halfH));

        _view.setSize(size);
        _view.setCenter(c);
        win.setView(_view);
    }

private:
    sf::View     _view;
    sf::Vector2f _center{};
    float        _scale = 1.f;
    bool         _init  = false;
};
