#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/RigidBody.hpp"
#include "../Modules/Transform.hpp"

class CollisionSystem : public ECS::System
{
public:
    void update(ECS::Registery& reg, float) override
    {
        for (auto it1 = _entities.begin(); it1 != _entities.end(); ++it1)
        {
            for (auto it2 = std::next(it1); it2 != _entities.end(); ++it2)
            {
                auto& rb1 = reg.getModule<RigidBody>(*it1);
                auto& rb2 = reg.getModule<RigidBody>(*it2);

                if (rb1.isStatic && rb2.isStatic)
                    continue;
                if (!shareMask(rb1.mask, rb2.mask))
                    continue;

                resolve(reg.getModule<Transform>(*it1),
                        reg.getModule<Transform>(*it2),
                        rb1.isStatic, rb2.isStatic);
            }
        }
    }

private:
    static bool shareMask(const std::set<uint32_t>& a, const std::set<uint32_t>& b)
    {
        for (auto m : a)
            if (b.count(m))
                return true;
        return false;
    }

    static void resolve(Transform& a, Transform& b, bool aStatic, bool bStatic)
    {
        float overlapX = std::min(a.position.x + a.size.x, b.position.x + b.size.x)
                       - std::max(a.position.x, b.position.x);
        float overlapY = std::min(a.position.y + a.size.y, b.position.y + b.size.y)
                       - std::max(a.position.y, b.position.y);

        if (overlapX <= 0.f || overlapY <= 0.f)
            return;

        float wa = aStatic ? 0.f : (bStatic ? 1.f : 0.5f);
        float wb = bStatic ? 0.f : (aStatic ? 1.f : 0.5f);

        if (overlapX < overlapY)
        {
            float ca  = a.position.x + a.size.x * 0.5f;
            float cb  = b.position.x + b.size.x * 0.5f;
            float dir = (ca < cb) ? 1.f : -1.f;

            a.position.x -= dir * overlapX * wa;
            b.position.x += dir * overlapX * wb;
        }
        else
        {
            float ca  = a.position.y + a.size.y * 0.5f;
            float cb  = b.position.y + b.size.y * 0.5f;
            float dir = (ca < cb) ? 1.f : -1.f;

            a.position.y -= dir * overlapY * wa;
            b.position.y += dir * overlapY * wb;
        }
    }
};
