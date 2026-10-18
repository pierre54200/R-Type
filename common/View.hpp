#pragma once

#include <set>
#include <tuple>

#include "EntityManager.hpp"
#include "ModuleManager.hpp"

namespace ECS
{
    template <typename... Components>
    class   ECSView
    {
        public:
            ECSView(ModuleManager& modules, const std::set<Entity>& entities)
                : _entities(entities)
                , _maps(&modules.getMap<Components>()...)
            {
            }
            template <typename Func>
            void    each(Func&& func)
            {
                for (Entity entity : _entities)
                {
                    std::apply(
                        [&](auto*... maps)
                        {
                            func(entity, maps->get(entity)...);
                        },
                        _maps
                    );
                }
            }

            size_t  size() const { return _entities.size(); }

        private:
            const std::set<Entity>&             _entities;
            std::tuple<ModuleMap<Components>*...>   _maps;
    };
}
