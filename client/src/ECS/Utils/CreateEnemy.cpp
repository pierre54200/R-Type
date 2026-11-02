#include "ECS/Modules/SceneTag.hpp"
#include "ECS/Modules/Targetable.hpp"
#include "SpawnEntities.hpp"
#include "../Modules/Transform.hpp"
#include "../Modules/FrontEnd.hpp"
#include "../Modules/HitBox.hpp"
#include "../Modules/Health.hpp"
#include "../Modules/EnemyStat.hpp"
#include "../Modules/RigidBody.hpp"
#include <cstdint>
#include <set>

void    Spawn::createEnemy(ECS::Registery &reg, Math::Vec3<float> pos, Math::Vec3<float> size, SceneId scene)
{
    Entity enemy = reg.createEntity();

    reg.addModule(enemy, Transform{ .position = pos, .size = size})
        .addModule(enemy, FrontEnd{ true, false, 0, sf::Color::Yellow})
        .addModule(enemy, HitBox{  {0.f, 0.f, 0.f}, {100.f, 100.f, 0.f}, std::set<uint32_t>{1}})
        .addModule(enemy, Health{3.f, 3.f, 1.f, 1})
        .addModule(enemy, RigidBody{.velocity = {0.f, 0.f, 0.f}, .mask = std::set<uint32_t>()})
        .addModule(enemy, EnemyStat{})
        .addModule(enemy, SceneTag{scene})
        .addModule(enemy, Targetable{.id = 1});
}
