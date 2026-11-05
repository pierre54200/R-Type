#pragma once
#include <cstdint>
#include <string>

struct  RemoteEntity
{
    uint8_t     id;
    std::string name;
    bool    dead = false;
};
