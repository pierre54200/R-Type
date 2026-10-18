#pragma once
#include "EntityManager.hpp"
#include "ModuleManager.hpp"
#include "SystemManager.hpp"
#include "View.hpp"
#include <chrono>

namespace ECS
{
    class   Registery
    {
        public:
            Registery() = default;

            Entity  createEntity()
            {
                return _entityManager.createEntity();
            }

            void    destroyEntity(Entity entity)
            {
                _entityManager.destroyEntity(entity);
                _moduleManager.entityDestroyed(entity);
                _systemManager.entityDestroyed(entity);
            }

            size_t  getLivingCount() const
            {
                return _entityManager.getLivingCount();
            }

            template <typename T>
            ECS::Registery& registerModule()
            {
                _moduleManager.registerModule<T>();
                return *this;
            }

            template <typename T>
            ECS::Registery& addModule(Entity entity, T value)
            {
                _moduleManager.add<T>(entity, std::move(value));
                EntitySignature signature = _entityManager.getSignature(entity);
                signature.set(_moduleManager.getId<T>());
                _entityManager.setSignature(entity, signature);
                _systemManager.entitySignatureChanged(entity, signature);
                return *this;
            }

            template <typename T>
            void    removeModule(Entity entity)
            {
                _moduleManager.remove<T>(entity);
                EntitySignature signature = _entityManager.getSignature(entity);
                signature.reset(_moduleManager.getId<T>());
                _entityManager.setSignature(entity, signature);
                _systemManager.entitySignatureChanged(entity, signature);
            }

            template <typename T>
            T&      getModule(Entity entity)
            {
                return _moduleManager.get<T>(entity);
            }

            template <typename T>
            bool    hasModule(Entity entity)
            {
                return _moduleManager.has<T>(entity);
            }

            template <typename... Ts>
            EntitySignature signatureOf() const
            {
                EntitySignature signature;
                (signature.set(_moduleManager.getId<Ts>()), ...);
                return signature;
            }

            template <typename T>
            std::shared_ptr<T> registerSystem(const EntitySignature& required)
            {
                return _systemManager.registerSystem<T>(required);
            }

            template <typename... Ts>
            ECSView<Ts...>  view(const std::set<Entity>& entities)
            {
                return ECSView<Ts...>(_moduleManager, entities);
            }

            void    update(float dt)
            {
                _systemManager.updateAll(*this, dt);
            }

            void    run()
            {
                this->_running = true;
                auto last = std::chrono::steady_clock::now();

                while (this->_running)
                {
                    auto    now = std::chrono::steady_clock::now();
                    float   dt = std::chrono::duration<float>(now - last).count();
                    last = now;
                    this->update(dt);
                }
            }

            void    stop()
            {
                this->_running = false;
            }

        private:
            bool            _running    =   false;
            EntityManager   _entityManager;
            ModuleManager   _moduleManager;
            SystemManager   _systemManager;
    };
}
