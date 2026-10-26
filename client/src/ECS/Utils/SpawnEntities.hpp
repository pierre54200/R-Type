#pragma once

#include "ECS/Modules/SceneTag.hpp"
#include "Registery.hpp"
#include "../Vec3.hpp"

namespace Spawn
{
    void    createBullet(ECS::Registery& reg, Math::Vec3<float>  pos, Math::Vec3<float>  size, SceneId scene);
};
