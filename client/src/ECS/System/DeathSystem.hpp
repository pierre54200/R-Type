#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "Protocol.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/NetworkSync.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>

class DeathSystem : public ECS::System
{
public:
    static inline Entity player = 0;
    static inline std::function<void(const Packet&)> send;
    static constexpr float REVIVE_FRACTION = .25f;
    static constexpr float HEAL_FRACTION   = .40f;

    void update(ECS::Registery& reg, float dt) override
    {
        for (auto e : _entities)
        {
            auto& hp = reg.getModule<Health>(e);
            if (hp.point > 0.f)
            {
                _dead = false;
                continue;
            }
            if (!_dead)
            {
                _dead   = true;
                _resend = 0.f;
                reg.getModule<FrontEnd>(e).isInvisible = true;
                notify(hp);
                continue;
            }
            _resend += dt;
            if (_resend >= 1.f)
            {
                _resend = 0.f;
                notify(hp);
            }
        }
    }

    static void hurt(ECS::Registery& reg, float amount)
    {
        auto& hp = reg.getModule<Health>(player);
        if (hp.point > 0.f)
            hp.point -= amount;
    }

    static void intermission(ECS::Registery& reg)
    {
        auto& hp = reg.getModule<Health>(player);

        if (hp.point <= 0.f)
        {
            hp.point = std::max(1.f, hp.maxPoint * REVIVE_FRACTION);
            reg.getModule<FrontEnd>(player).isInvisible = false;
            reg.getModule<NetworkSync>(player).hasSent = false;
            return;
        }
        hp.point = std::min(hp.maxPoint, hp.point + hp.maxPoint * HEAL_FRACTION);
    }

private:
    void notify(const Health& hp)
    {
        if (send)
            send(makeDeath(static_cast<uint16_t>(std::clamp(hp.maxPoint, 1.f, 65535.f))));
    }

    bool  _dead   = false;
    float _resend = 0.f;
};