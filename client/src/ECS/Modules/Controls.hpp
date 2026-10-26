#pragma once

#include "../String/String.hpp"
#include <stdexcept>
#include <map>

template<typename T>
class Controls
{
    public:
        Controls() = default;

        Controls& registerKey(const String& action, T bind)
        {
            keybinds[action] = bind;
            return *this;
        }

        Controls& deleteKey(const String& action)
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
