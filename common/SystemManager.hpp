#pragma once
#include <cstddef>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "EntityManager.hpp"

namespace ECS
{
    class   Registery;

    class   System
    {
        public:
            virtual ~System() = default;
            virtual void    update(Registery& registery, float dt) = 0;

            size_t  getEntityCount() const  { return _entities.size(); }
            const std::set<Entity>&     getEntities() const     { return _entities; }

        protected:
            std::set<Entity>    _entities;

        private:
            friend class SystemManager;
    };

    class   SystemManager
    {
        public:
            template <typename T>
            std::shared_ptr<T>  registerSystem(const EntitySignature& required)
            {
                static_assert(std::is_base_of<System, T>::value, "T must derive from ECS::System");
                std::type_index type = typeid(T);
                if (_systems.count(type))
                    throw std::logic_error("SystemManager: system registered twice");
                auto    system = std::make_shared<T>();
                _systems[type]    = system;
                _signatures[type] = required;
                _order.push_back(system);
                return system;
            }

            void    entitySignatureChanged(Entity entity, const EntitySignature& entitySignature)
            {
                for (auto& pair : _systems)
                {
                    const EntitySignature&  required = _signatures[pair.first];
                    if ((entitySignature & required) == required)
                        pair.second->_entities.insert(entity);
                    else
                        pair.second->_entities.erase(entity);
                }
            }

            void    entityDestroyed(Entity entity)
            {
                for (auto& pair : _systems)
                    pair.second->_entities.erase(entity);
            }

            void    updateAll(Registery& registery, float dt)
            {
                for (auto& system : _order)
                    system->update(registery, dt);
            }

        private:
            std::unordered_map<std::type_index, std::shared_ptr<System>>    _systems;
            std::unordered_map<std::type_index, EntitySignature>            _signatures;
            std::vector<std::shared_ptr<System>>                            _order;
    };
}
