#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/HitBox.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/Transform.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>

class HurtSystem:   public  ECS::System
{
    public:
        void    update(ECS::Registery& reg, float) override
        {
            auto                view = reg.view<HitBox, Transform, Health>(_entities);
            std::set<Entity>    toDestroy;

            view.each([&](Entity e1, HitBox& h1, Transform& t1, Health& hp1)
            {
                view.each([&](Entity e2, HitBox& h2, Transform& t2, Health& hp2)
                {
                    if (e2 <= e1)
                        return;

                    if (!shareMask(h1.mask, h2.mask))
                        return;

                    float   overlapX = std::min(h1.topLeft.x + h1.size.x + t1.position.x,
                                                h2.topLeft.x + h2.size.x + t2.position.x)
                                     - std::max(h1.topLeft.x + t1.position.x,
                                                h2.topLeft.x + t2.position.x);
                    float   overlapY = std::min(h1.topLeft.y + h1.size.y + t1.position.y,
                                                h2.topLeft.y + h2.size.y + t2.position.y)
                                     - std::max(h1.topLeft.y + t1.position.y,
                                                h2.topLeft.y + t2.position.y);

                    if (overlapX <= 0.f || overlapY <= 0.f)
                        return;

                    if (!resolve(h1, hp1, h2, hp2))
                        return;

                    if (h1.destroyOnHit && hp1.hurt)
                        toDestroy.insert(e1);
                    if (h2.destroyOnHit && hp2.hurt)
                        toDestroy.insert(e2);
                });
            });

            for (Entity e : toDestroy)
                reg.destroyEntity(e);
        }

    private:
        static bool shareMask(const std::set<uint32_t>& a, const std::set<uint32_t>& b)
        {
            for (auto m : a)
                if (b.count(m))
                    return true;
            return false;
        }

        static bool ready(const HitBox& box)
        {
            return !box.hasBeenHit || box.hitCooldownClock.isReady();
        }

        static void applyHit(HitBox& victimBox, Health& victim, const Health& attacker)
        {
            if (!ready(victimBox))
                return;

            victim.point -= attacker.point;

            victimBox.hitCooldownClock.setTargetTime(victimBox.hitCooldownValue);
            victimBox.hitCooldownClock.restart();
            victimBox.hasBeenHit = true;

            std::cout << victim.point << std::endl;
        }

        static bool resolve(HitBox& b1, Health& hp1, HitBox& b2, Health& hp2)
        {
            if (hp1.hurt == hp2.hurt)
                return false;
            if (hp1.hurt)
                applyHit(b2, hp2, hp1);
            else
                applyHit(b1, hp1, hp2);
            return true;
        }
};
