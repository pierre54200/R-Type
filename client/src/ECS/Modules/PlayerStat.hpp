#pragma once

#include "../Clock/Clock.hpp"

struct PlayerStat
{
    Clock::Clock    shootCooldown;
    Clock::Clock    attackCooldown;
    float   speed   =   200.f;
    bool    freeze  =   false;
    float   shootDelay  =   .1f;
    float bulletSpeed = 400.f;
    float bulletDamage = 1.f;
};
