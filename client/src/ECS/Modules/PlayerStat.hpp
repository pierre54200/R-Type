#pragma once

#include "../Clock/Clock.hpp"

struct PlayerStat
{
    Clock::Clock    shootCooldown;
    Clock::Clock    attackCooldown;
    float   speed   =   200.f;
    bool    freeze  =   false;
};
