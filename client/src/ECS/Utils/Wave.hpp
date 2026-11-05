#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

struct Wave
{
    static inline bool        running = false;
    static inline int         number = 0;
    static inline int         toSpawn = 0;
    static inline int         spawned = 0;
    static inline bool        isHost = false;
    static inline std::string clientName;

    static inline std::mt19937 rng{};

    static void setClient(const std::string& name)
    {
        clientName = name;
    }

    static void start(std::uint8_t n)
    {
        number = n;
        toSpawn = 5 + n * 3;
        spawned = 0;
        running = true;

        std::vector<std::uint32_t> seed;

        for (unsigned char c : clientName)
            seed.push_back(c);

        seed.push_back(n);
        seed.push_back(std::random_device{}());

        std::seed_seq seq(seed.begin(), seed.end());
        rng.seed(seq);
    }

    static bool canSpawn()
    {
        return running && spawned < toSpawn;
    }
};