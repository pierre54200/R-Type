#pragma once

#include "ECS/Clock/Clock.hpp"
#include "../Vec3.hpp"

struct SpawnerSettings
{
    float   time    =   .01f;
    Clock::Clock    spawnClock;
    Math::Vec3<float> minPos  {0.f,   0.f,   0.f};
    Math::Vec3<float> maxPos  {600.f, 800.f, 0.f};
    Math::Vec3<float> minSize {20.f,  20.f,  0.f};
    Math::Vec3<float> maxSize {60.f,  60.f,  0.f};
};
