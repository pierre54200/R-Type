#include "SpawnEntities.hpp"

#include "../Modules/Transform.hpp"
#include "../Modules/HitBox.hpp"
#include "../Modules/RigidBody.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/DespawnByRange.hpp"
#include "../Modules/FrontEnd.hpp"

void    Spawn::createBullet(ECS::Registery& reg, Math::Vec3<float>  pos, Math::Vec3<float>  size, SceneId   scene)
{
    Entity  bullet = reg.createEntity();

    reg.addModule(bullet, Transform{{pos.x + size.x, pos.y + size.y / 2, 0.f}, {size.x / 10, size.y / 10, 0.f}})
        .addModule(bullet, HitBox{
            .topLeft = {0.f, 0.f, 0.f},
            .size = {size.x / 10, size.y / 10, 0.f},
            .mask = std::set<uint32_t>{2},
            .destroyOnHit = true})
        .addModule(bullet, RigidBody{{100.f, 0.f, 0.f}, 9.8f, std::set<uint32_t>{0}})
        .addModule(bullet, Health{.point = 5.f, .hurt = true, .maxPoint = 0.f})
        .addModule(bullet, DespawnByRange{{pos.x, pos.y, 0.f}})
        .addModule(bullet, FrontEnd{true, false, 0, sf::Color::Yellow})
        .addModule(bullet, SceneTag{scene});
}
