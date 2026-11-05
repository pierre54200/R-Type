#pragma once

#include "Registery.hpp"
#include "Protocol.hpp"
#include "Upgrades.hpp"
#include "Wave.hpp"
#include "ECS/Modules/SceneTag.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/Function.hpp"
#include "../Modules/Label.hpp"
#include "../Modules/Transform.hpp"

#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstddef>
#include <functional>

namespace UpgradeUI
{
    inline Entity                               player = 0;
    inline std::array<Entity, 3>                cards{};
    inline std::function<void(const Packet&)>   send;
    inline bool                                 picked = true;

    inline void hide(ECS::Registery& reg)
    {
        for (Entity c : cards)
            reg.getModule<FrontEnd>(c).isInvisible = true;
    }

    inline void pick(ECS::Registery& reg, const UpgradeDef* up)
    {
        if (picked)
            return;
        picked = true;
        up->apply(reg.getModule<PlayerStat>(player), reg.getModule<Health>(player));

        hide(reg);
        reg.getModule<Label>(cards[1]).text = "Waiting for other players...";
        reg.getModule<FrontEnd>(cards[1]).isInvisible = false;

        if (send)
            send({PacketType::Ready, "1"});
    }

    inline void show(ECS::Registery& reg)
    {
        picked = false;
        auto& pool    = Upgrades::all();
        auto  choices = Upgrades::draw(cards.size(), Wave::rng);

        for (std::size_t i = 0; i < cards.size(); ++i)
        {
            auto& front = reg.getModule<FrontEnd>(cards[i]);
            if (i >= choices.size())
            {
                front.isInvisible = true;
                continue;
            }
            UpgradeDef* up = &pool[choices[i]];
            reg.getModule<Label>(cards[i]).text = up->name + "\n" + up->description;
            reg.getModule<Function>(cards[i]).function =
                [up](ECS::Registery& r)
            {
                pick(r, up);
            };
            front.isInvisible = false;
        }
    }

    inline void create(ECS::Registery& reg, SceneId scene, sf::Vector2f win)
    {
        float w = win.x * .26f;
        float h = 160.f;
        float gap = win.x * .04f;
        float x0 = (win.x - (3.f * w + 2.f * gap)) / 2.f;
        float y  = (win.y - h) / 2.f;

        for (std::size_t i = 0; i < cards.size(); ++i)
        {
            Entity card = reg.createEntity();
            reg.addModule(card, SceneTag{scene})
                .addModule(card, Transform{{x0 + i * (w + gap), y, 0.f}, {w, h, 0.f}})
                .addModule(card, FrontEnd{true, true, 5, sf::Color(40, 40, 90), true})
                .addModule(card, Label{"", 20, sf::Color::White})
                .addModule(card, Function{[](ECS::Registery&) {}});
            cards[i] = card;
        }
    }
}
