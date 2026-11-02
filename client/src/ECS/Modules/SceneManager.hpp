#pragma once

#include "SceneTag.hpp"
#include "Registery.hpp"

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct SceneDef
{
    std::function<void(ECS::Registery&, SceneId)> onEnter   =   nullptr;
    std::function<void(ECS::Registery&, SceneId)> onExit    =   nullptr;
};

struct SceneManager
{
    enum Op { Push, Pop, Switch, Reset };
    struct Command { Op op; SceneId scene; };

    std::unordered_map<SceneId, SceneDef>    defs;
    std::unordered_map<std::string, SceneId> names;
    std::vector<SceneId>                     stack;
    std::vector<Command>                     pending;

    SceneId add(const std::string& name, SceneDef def = {})
    {
        SceneId id = static_cast<SceneId>(names.size() + 1);
        names[name] = id;
        defs[id]    = std::move(def);
        return id;
    }

    SceneId id(const std::string& name) const { return names.at(name); }

    void push(SceneId s)     { pending.push_back({Op::Push,   s}); }
    void pop()               { pending.push_back({Op::Pop,    0}); }
    void switchTo(SceneId s) { pending.push_back({Op::Switch, s}); }
    void reset(SceneId s)    { pending.push_back({Op::Reset,  s}); }

    void push(const std::string& n)     { push(id(n)); }
    void switchTo(const std::string& n) { switchTo(id(n)); }
    void reset(const std::string& n)    { reset(id(n)); }

    bool isRunning(SceneId s) const
    {
        return std::find(stack.begin(), stack.end(), s) != stack.end();
    }
    bool isTop(SceneId s) const { return !stack.empty() && stack.back() == s; }

    SceneId top() const { return stack.empty() ? 0 : stack.back(); }

    int stackIndex(SceneId s) const
    {
        auto it = std::find(stack.begin(), stack.end(), s);
        return it == stack.end() ? -1 : static_cast<int>(it - stack.begin());
    }
};
