#pragma once
#include <cstdint>
#include <string>

struct  NetworkSync
{
    uint8_t     id          =   0;
    std::string name;
    float       sendRate    =   60.f;

    float       elapsed     =   0.f;
    bool        hasSent     =   false;
    uint16_t    lastX       =   0;
    uint16_t    lastY       =   0;
};
