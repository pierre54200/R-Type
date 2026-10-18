#pragma once
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

#include "EntityManager.hpp"

namespace ECS
{
    class   IModuleMap
    {
        public:
            virtual ~IModuleMap() = default;
            virtual void    remove(Entity entity) = 0;
    };

    template <typename T>
    class   ModuleMap : public IModuleMap
    {
        public:
            void    add(Entity entity, T value)
            {
                _data.insert_or_assign(entity, std::move(value));
            }

            void    remove(Entity entity) override
            {
                _data.erase(entity);
            }

            bool    has(Entity entity) const
            {
                return _data.find(entity) != _data.end();
            }

            T&      get(Entity entity)
            {
                auto    it = _data.find(entity);
                if (it == _data.end())
                    throw std::out_of_range("ModuleMap: entity does not have this module");
                return it->second;
            }

        private:
            std::unordered_map<Entity, T>   _data;
    };

    class   ModuleManager
    {
        public:
            template <typename T>
            void    registerModule()
            {
                std::type_index type = typeid(T);
                if (_ids.count(type))
                    throw std::logic_error("ModuleManager: module registered twice");
                if (_nextId >= MAX_MODULES)
                    throw std::runtime_error("ModuleManager: too many module types");
                _ids[type]  = _nextId++;
                _maps[type] = std::make_unique<ModuleMap<T>>();
            }

            template <typename T>
            size_t  getId() const
            {
                auto    it = _ids.find(typeid(T));
                if (it == _ids.end())
                    throw std::logic_error("ModuleManager: module not registered");
                return it->second;
            }

            template <typename T>
            void    add(Entity entity, T value)     { getMap<T>().add(entity, std::move(value)); }

            template <typename T>
            void    remove(Entity entity)           { getMap<T>().remove(entity); }

            template <typename T>
            T&      get(Entity entity)              { return getMap<T>().get(entity); }

            template <typename T>
            bool    has(Entity entity)              { return getMap<T>().has(entity); }

            void    entityDestroyed(Entity entity)
            {
                for (auto& pair : _maps)
                    pair.second->remove(entity);
            }

            template <typename T>
            ModuleMap<T>&   getMap()
            {
                auto    it = _maps.find(typeid(T));
                if (it == _maps.end())
                    throw std::logic_error("ModuleManager: module not registered");
                return *static_cast<ModuleMap<T>*>(it->second.get());
            }

        private:
            std::unordered_map<std::type_index, size_t>                         _ids;
            std::unordered_map<std::type_index, std::unique_ptr<IModuleMap>>    _maps;
            size_t                                                              _nextId = 0;
    };
}
