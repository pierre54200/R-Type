#pragma once

#include "../String/String.hpp"
#include "../Clock/Clock.hpp"
#include <stdexcept>
#include <map>

template<typename T>
class PlayerControl
{
    public:
        PlayerControl() = default;

        float speed = 200.f;

        Clock::Clock    shootCooldown;

        PlayerControl& registerKey(const String& action, T bind)
        {
            keybinds[action] = bind;
            return *this;
        }

        PlayerControl& deleteKey(const String& action)
        {
            keybinds.erase(action);
            return *this;
        }

        const String& getAction(T bind) const
        {
            for (const auto& [action, key] : keybinds)
            {
                if (key == bind)
                    return action;
            }
            throw std::runtime_error("Key not found");
        }

        T getKey(const String& action) const
        {
            auto it = keybinds.find(action);
            if (it == keybinds.end())
                throw std::runtime_error("Key not registered");
            return it->second;
        }

    private:
        std::map<String, T> keybinds;
};
