#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/Window.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/Label.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <algorithm>
#include <cstdint>
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
            _drawQueue.push_back({e, f.layer, f.screenSpace});
        }

        std::sort(_drawQueue.begin(), _drawQueue.end(),
            [](const DrawItem& a, const DrawItem& b)
            {
                if (a.hud != b.hud)
                    return !a.hud;
                if (a.layer != b.layer)
                    return a.layer < b.layer;
                return a.entity < b.entity;
            });

        bool hudView = false;
        for (const auto& item : _drawQueue)
        {
            if (item.hud && !hudView)
            {
                win.window->setView(win.window->getDefaultView());
                hudView = true;
            }

            auto& t = reg.getModule<Transform>(item.entity);
            auto& f = reg.getModule<FrontEnd>(item.entity);

            sf::RectangleShape r({t.size.x, t.size.y});
            r.setPosition(t.position.x, t.position.y);
            r.setFillColor(f.color);
            win.window->draw(r);

            if (reg.hasModule<Label>(item.entity))
            {
                if (sf::Font* font = fontOrNull())
                {
                    auto& l = reg.getModule<Label>(item.entity);
                    sf::Text text(l.text, *font, l.size);
                    text.setFillColor(l.color);
                    sf::FloatRect b = text.getLocalBounds();
                    text.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
                    text.setPosition(t.position.x + t.size.x / 2.f, t.position.y + t.size.y / 2.f);
                    win.window->draw(text);
                }
            }
        }
    }

private:
    struct DrawItem
    {
        Entity   entity;
        uint32_t layer;
        bool     hud;
    };

    static sf::Font* fontOrNull()
    {
        static sf::Font font;
        static bool ok = font.loadFromFile("assets/font.ttf");
        return ok ? &font : nullptr;
    }

    std::vector<DrawItem> _drawQueue;
};