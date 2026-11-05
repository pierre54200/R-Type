#pragma once

#include "../Modules/Health.hpp"
#include "../Modules/PlayerStat.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <random>
#include <string>
#include <vector>

struct UpgradeDef
{
    std::string name;
    std::string description;
    std::function<void(PlayerStat&, Health&)> apply;
};

namespace Upgrades
{
    inline std::vector<UpgradeDef>& all()
    {
        static std::vector<UpgradeDef> pool = {
            {"Swift", "+15% move speed",
                [](PlayerStat& s, Health&) { s.speed *= 1.15f; }},
            {"Rapid Fire", "-15% shoot delay",
                [](PlayerStat& s, Health&) { s.shootDelay = std::max(.03f, s.shootDelay * .85f); }},
            {"Heavy Rounds", "+1 bullet damage",
                [](PlayerStat& s, Health&) { s.bulletDamage += 1.f; }},
            {"Fast Bullets", "+20% bullet speed",
                [](PlayerStat& s, Health&) { s.bulletSpeed *= 1.2f; }},
            {"Tough", "+25 max HP and heal 25",
                [](PlayerStat&, Health& h) { h.maxPoint += 25.f; h.point += 25.f; }},
            {"Medkit", "Restore all HP",
                [](PlayerStat&, Health& h) { h.point = h.maxPoint; }},
        };
        return pool;
    }

    inline std::vector<std::size_t> draw(std::size_t n, std::mt19937& rng)
    {
        std::vector<std::size_t> idx(all().size());
        std::iota(idx.begin(), idx.end(), 0);
        std::shuffle(idx.begin(), idx.end(), rng);
        idx.resize(std::min(n, idx.size()));
        return idx;
    }
}
