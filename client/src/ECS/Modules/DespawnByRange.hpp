#pragma once

#include "../Vec3.hpp"

struct DespawnByRange
{
    Math::Vec3<float>   spawnPosition;
    float               maxRange    =   2500;
};
