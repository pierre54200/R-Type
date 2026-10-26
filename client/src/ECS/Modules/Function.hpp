#pragma once

#include <functional>

#include "Registery.hpp"

struct Function
{
    std::function<void(ECS::Registery&)> function   =   nullptr;
};
