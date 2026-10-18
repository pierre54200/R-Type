#pragma once

#include "../Vec3.hpp"

#include <set>

struct RigidBody
{
    Math::Vec3<float>   velocity;
    float               gravity = 9.8f;
    std::set<uint32_t>  mask;
    bool                isStatic = false;
};
