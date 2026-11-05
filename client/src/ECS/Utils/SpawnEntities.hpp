#pragma once

#include "ECS/Modules/SceneTag.hpp"
#include "Registery.hpp"
#include "../Vec3.hpp"

namespace Spawn
{
    void    createBullet(ECS::Registery& reg, Math::Vec3<float>  pos, Math::Vec3<float>  vel, SceneId scene, float damage = 1.f);
    void    createEnemy(ECS::Registery&  reg, Math::Vec3<float>  pos, SceneId    scene);
};
