#pragma once

#include "EntityManager.hpp"

#include <set>

enum Events
{
    FEUR,
    JSAISPAS,
    TROIS,
};

struct Event
{
    Events              event;
    Entity              author;
    std::set<Entity>    concernedEntities;  //Author can be concerned as well, and should put there
};
