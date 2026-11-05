#pragma once

#include "Registery.hpp"
#include "Protocol.hpp"
#include "ECS/Modules/SceneManager.hpp"
#include "ECS/Modules/SceneTag.hpp"
#include "ECS/System/SceneSystem.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/Function.hpp"
#include "../Modules/Label.hpp"
#include "../Modules/Transform.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace MenuUI
{
    inline std::function<void(const Packet&)> send;
    inline std::string playerName;
    inline std::string currentLobby;
    inline bool        inGame        = false;
    inline bool        browserActive = false;
    inline Entity      status        = 0;
    inline bool        statusValid   = false;

    inline void invalidate()
    {
        statusValid   = false;
        browserActive = false;
    }

    inline void setStatus(ECS::Registery& reg, const std::string& text)
    {
        if (statusValid)
            reg.getModule<Label>(status).text = text;
    }

    inline Entity label(ECS::Registery& reg, SceneId scene, float x, float y, float w, float h,
                        const std::string& text, unsigned size = 22,
                        sf::Color color = sf::Color::White)
    {
        Entity e = reg.createEntity();
        reg.addModule(e, SceneTag{scene})
            .addModule(e, Transform{{x, y, 0.f}, {w, h, 0.f}})
            .addModule(e, FrontEnd{true, false, 5, sf::Color(0, 0, 0, 0), true})
            .addModule(e, Label{text, size, color});
        return e;
    }

    inline Entity button(ECS::Registery& reg, SceneId scene, float x, float y, float w, float h,
                         sf::Color fill, const std::string& text,
                         std::function<void(ECS::Registery&)> onClick)
    {
        Entity e = reg.createEntity();
        reg.addModule(e, SceneTag{scene})
            .addModule(e, Transform{{x, y, 0.f}, {w, h, 0.f}})
            .addModule(e, FrontEnd{true, false, 4, fill, true})
            .addModule(e, Label{text, 22, sf::Color::White})
            .addModule(e, Function{std::move(onClick)});
        return e;
    }

    inline void createStatus(ECS::Registery& reg, SceneId scene, sf::Vector2f win, const std::string& text)
    {
        status      = label(reg, scene, 0.f, 625.f, win.x, 40.f, text, 18, sf::Color(220, 220, 120));
        statusValid = true;
    }
}

namespace LobbyBrowser
{
    inline constexpr std::size_t ROWS = 8;
    inline std::array<Entity, ROWS>      rows{};
    inline std::array<std::string, ROWS> rowLobby{};
    inline Entity                        hint = 0;

    inline void requestList()
    {
        if (MenuUI::send)
            MenuUI::send({PacketType::LobbyList, ""});
    }

    inline void create(ECS::Registery& reg, SceneId scene, sf::Vector2f win)
    {
        MenuUI::inGame        = false;
        MenuUI::browserActive = true;

        MenuUI::label(reg, scene, 0.f, 20.f, win.x, 60.f, "Choose a lobby", 34);
        MenuUI::createStatus(reg, scene, win, "");

        float w = std::min(640.f, win.x * .8f);
        float h = 48.f;
        float gap = 8.f;
        float x = (win.x - w) / 2.f;
        for (std::size_t i = 0; i < ROWS; ++i)
        {
            rowLobby[i].clear();
            rows[i] = MenuUI::button(reg, scene, x, 90.f + i * (h + gap), w, h,
                                     sf::Color(40, 60, 120), "",
                [i](ECS::Registery& r)
                {
                    if (rowLobby[i].empty() || !MenuUI::send)
                        return;
                    MenuUI::setStatus(r, "Joining " + rowLobby[i] + "...");
                    MenuUI::send({PacketType::LobbyJoin, rowLobby[i]});
                });
            reg.getModule<FrontEnd>(rows[i]).isInvisible = true;
        }
        hint = MenuUI::label(reg, scene, 0.f, 250.f, win.x, 40.f,
                             "No lobby yet - create one!", 22, sf::Color(170, 170, 170));

        MenuUI::button(reg, scene, (win.x - 280.f) / 2.f, 560.f, 280.f, 50.f,
                       sf::Color(30, 130, 60), "Create a lobby",
            [](ECS::Registery& r)
            {
                MenuUI::setStatus(r, "Creating...");
                if (MenuUI::send)
                    MenuUI::send({PacketType::LobbyCreate, MenuUI::playerName + "'s lobby"});
            });

        requestList();
    }

    inline void update(ECS::Registery& reg, const std::vector<LobbyEntry>& lobbies)
    {
        if (!MenuUI::browserActive)
            return;

        
        static const std::array<std::string, 3> stateText = {
            "waiting",
            "in game",
            "between waves"
        };

        reg.getModule<FrontEnd>(hint).isInvisible = !lobbies.empty();
        for (std::size_t i = 0; i < ROWS; ++i)
        {
            auto& front = reg.getModule<FrontEnd>(rows[i]);
            if (i >= lobbies.size())
            {
                front.isInvisible = true;
                rowLobby[i].clear();
                continue;
            }
            const LobbyEntry& l = lobbies[i];
            rowLobby[i] = l.name;
            reg.getModule<Label>(rows[i]).text =
                l.name + "   -   " + std::to_string(l.players) + " player(s)   -   "
                + stateText[std::min<std::size_t>(l.state, 2)];
            front.color       = (l.state == 1) ? sf::Color(70, 70, 70) : sf::Color(40, 60, 120);
            front.isInvisible = false;
        }
    }
}

namespace RoomScreen
{
    inline bool   ready       = false;
    inline Entity readyButton = 0;

    inline void create(ECS::Registery& reg, SceneId scene, sf::Vector2f win)
    {
        MenuUI::inGame = false;
        ready          = false;

        MenuUI::label(reg, scene, 0.f, 40.f, win.x, 60.f, "Lobby: " + MenuUI::currentLobby, 34);
        MenuUI::createStatus(reg, scene, win, "Click Ready when you are set");

        float bw = 320.f, x = (win.x - bw) / 2.f;
        readyButton = MenuUI::button(reg, scene, x, 220.f, bw, 80.f, sf::Color(150, 40, 40), "Not ready",
            [](ECS::Registery& r)
            {
                ready = !ready;
                r.getModule<FrontEnd>(readyButton).color = ready ? sf::Color(30, 130, 60) : sf::Color(150, 40, 40);
                r.getModule<Label>(readyButton).text     = ready ? "Ready!" : "Not ready";
                if (MenuUI::send)
                    MenuUI::send({PacketType::Ready, ready ? "1" : "0"});
            });

        MenuUI::button(reg, scene, x, 330.f, bw, 56.f, sf::Color(60, 60, 60), "Leave lobby",
            [](ECS::Registery& r)
            {
                if (MenuUI::send)
                    MenuUI::send({PacketType::LobbyLeave, ""});
                MenuUI::invalidate();
                r.getModule<SceneManager>(SceneSystem::manager).switchTo("browser");
            });
    }
}
