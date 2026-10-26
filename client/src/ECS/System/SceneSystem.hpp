#pragma once

#include "Registery.hpp"
#include "SystemManager.hpp"
#include "../Modules/SceneManager.hpp"
#include "../Modules/SceneTag.hpp"

class SceneSystem : public ECS::System
{
public:
    static inline Entity manager = 0;

    void update(ECS::Registery& reg, float) override
    {
        auto& sm = reg.getModule<SceneManager>(manager);

        auto cmds = std::move(sm.pending);
        sm.pending.clear();

        for (auto& c : cmds)
        {
            switch (c.op)
            {
            case SceneManager::Op::Push:
                enter(reg, sm, c.scene);
                break;
            case SceneManager::Op::Pop:
                if (!sm.stack.empty())
                    exit(reg, sm, sm.stack.back());
                break;
            case SceneManager::Op::Switch:
                while (!sm.stack.empty())
                    exit(reg, sm, sm.stack.back());
                enter(reg, sm, c.scene);
                break;
            case SceneManager::Op::Reset:
                destroyOwned(reg, c.scene);
                if (auto& d = sm.defs[c.scene]; d.onEnter)
                    d.onEnter(reg, c.scene);
                break;
            }
        }
    }

private:
    void enter(ECS::Registery& reg, SceneManager& sm, SceneId s)
    {
        sm.stack.push_back(s);
        if (auto& d = sm.defs[s]; d.onEnter)
            d.onEnter(reg, s);
    }

    void exit(ECS::Registery& reg, SceneManager& sm, SceneId s)
    {
        if (auto& d = sm.defs[s]; d.onExit)
            d.onExit(reg, s);
        destroyOwned(reg, s);
        sm.stack.pop_back();
    }

    void destroyOwned(ECS::Registery& reg, SceneId s)
    {
        std::vector<Entity> toKill;
        for (auto e : _entities)
            if (reg.getModule<SceneTag>(e).scene == s)
                toKill.push_back(e);
        for (auto e : toKill)
            reg.destroyEntity(e);
    }
};
