#pragma once
#include <cstdint>

struct Health
{
    float    point    = 10.f;
    float    maxPoint = 100.f;
    float    damage   = 0.f;
    uint8_t  team     = 0;
};
