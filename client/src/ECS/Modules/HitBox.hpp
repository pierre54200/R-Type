#pragma once

#include <set>
#include <cstdint>

#include "../Vec3.hpp"
#include "../Clock/Clock.hpp"

struct HitBox
{
    Math::Vec3<float>   topLeft;
    Math::Vec3<float>   size;
    std::set<uint32_t>  mask;
    Clock::Clock        hitCooldownClock    {};
    float               hitCooldownValue    =   1.f;
    bool                hasBeenHit          =   false;
    bool                destroyOnHit        =   false;
};
