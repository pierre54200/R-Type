#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Window.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/FrontEnd.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <algorithm>
#include <vector>

class DisplaySystem : public ECS::System
{
public:
    static inline Entity screen = 0;

    void update(ECS::Registery& reg, float) override
    {
        auto& win = reg.getModule<Window<sf::RenderWindow>>(screen);

        _drawQueue.clear();
        for (auto e : _entities)
        {
            auto& f = reg.getModule<FrontEnd>(e);
            if (!f.isDrawable || f.isInvisible)
                continue;
            _drawQueue.push_back({e, f.layer});
        }

        std::sort(_drawQueue.begin(), _drawQueue.end(),
            [](const DrawItem& a, const DrawItem& b)
            {
                if (a.layer != b.layer)
                    return a.layer < b.layer;
                return a.entity < b.entity;
            });

        for (const auto& item : _drawQueue)
        {
            auto& t = reg.getModule<Transform>(item.entity);
            auto& f = reg.getModule<FrontEnd>(item.entity);

            sf::RectangleShape r({t.size.x, t.size.y});
            r.setPosition(t.position.x, t.position.y);
            r.setFillColor(f.color);
            win.window->draw(r);
        }
    }

private:
    struct DrawItem
    {
        Entity   entity;
        uint32_t layer;
    };

    std::vector<DrawItem> _drawQueue;
};
