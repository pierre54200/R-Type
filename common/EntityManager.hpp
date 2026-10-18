#pragma once
#include <array>
#include <bitset>
#include <cstddef>
#include <queue>
#include <stdexcept>

using   Entity          =   int;

namespace ECS
{
    constexpr   size_t  MAX_ENTITIES    =   5000;
    constexpr   size_t  MAX_MODULES     =   64;

    using   EntitySignature =   std::bitset<MAX_MODULES>;

    class   EntityManager
    {
        public:
            EntityManager()
            {
                _isAlive.fill(false);
                for (size_t i = 0; i < MAX_ENTITIES; ++i)
                    _freeIds.push(static_cast<Entity>(i));
            }

            Entity                  createEntity()
            {
                if (_freeIds.empty())
                    throw std::runtime_error("EntityManager: max entity count reached");
                Entity  entity = _freeIds.front();
                _freeIds.pop();
                _isAlive[entity] = true;
                _signatures[entity].reset();
                _livingCount++;
                return entity;
            }

            void                    destroyEntity(Entity entity)
            {
                checkAlive(entity);
                _signatures[entity].reset();
                _isAlive[entity] = false;
                _freeIds.push(entity);
                _livingCount--;
            }

            bool                    isAlive(Entity entity) const
            {
                return entity >= 0 && static_cast<size_t>(entity) < MAX_ENTITIES && _isAlive[entity];
            }

            void                    setSignature(Entity entity, const EntitySignature& signature)
            {
                checkAlive(entity);
                _signatures[entity] = signature;
            }

            const EntitySignature&  getSignature(Entity entity) const
            {
                checkAlive(entity);
                return _signatures[entity];
            }

            size_t  getLivingCount() const
            {
                return _livingCount;
            }

        private:
            void    checkAlive(Entity entity) const
            {
                if (!isAlive(entity))
                    throw std::out_of_range("EntityManager: invalid or dead entity");
            }

            std::array<EntitySignature, MAX_ENTITIES>   _signatures;
            std::array<bool, MAX_ENTITIES>              _isAlive;
            std::queue<Entity>                          _freeIds;
            size_t                                      _livingCount = 0;
    };
}
