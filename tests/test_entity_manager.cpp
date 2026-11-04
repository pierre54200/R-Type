#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <stdexcept>

#include "EntityManager.hpp"

using ECS::EntityManager;
using ECS::EntitySignature;
using ECS::MAX_ENTITIES;

static std::unique_ptr<EntityManager> makeManager()
{
    return std::make_unique<EntityManager>();
}

TEST(EntityManager, StartsEmpty)
{
    auto em = makeManager();
    EXPECT_EQ(em->getLivingCount(), 0u);
}

TEST(EntityManager, CreateReturnsDistinctLivingEntities)
{
    auto em = makeManager();
    std::set<Entity> ids;

    for (int i = 0; i < 100; ++i) {
        Entity e = em->createEntity();
        EXPECT_TRUE(em->isAlive(e));
        ids.insert(e);
    }
    EXPECT_EQ(ids.size(), 100u);
    EXPECT_EQ(em->getLivingCount(), 100u);
}

TEST(EntityManager, DestroyMarksEntityDeadAndDecrementsCount)
{
    auto em = makeManager();
    Entity a = em->createEntity();
    Entity b = em->createEntity();

    em->destroyEntity(a);

    EXPECT_FALSE(em->isAlive(a));
    EXPECT_TRUE(em->isAlive(b));
    EXPECT_EQ(em->getLivingCount(), 1u);
}

TEST(EntityManager, DestroyingTwiceThrows)
{
    auto em = makeManager();
    Entity e = em->createEntity();

    em->destroyEntity(e);
    EXPECT_THROW(em->destroyEntity(e), std::out_of_range);
    EXPECT_EQ(em->getLivingCount(), 0u);
}

TEST(EntityManager, InvalidIdsAreNeverAlive)
{
    auto em = makeManager();

    EXPECT_FALSE(em->isAlive(-1));
    EXPECT_FALSE(em->isAlive(static_cast<Entity>(MAX_ENTITIES)));
    EXPECT_FALSE(em->isAlive(0)); // not created yet
    EXPECT_THROW(em->destroyEntity(-1), std::out_of_range);
    EXPECT_THROW(em->getSignature(static_cast<Entity>(MAX_ENTITIES)), std::out_of_range);
}

TEST(EntityManager, ThrowsWhenFullAndReusesFreedIds)
{
    auto em = makeManager();
    for (std::size_t i = 0; i < MAX_ENTITIES; ++i)
        em->createEntity();

    EXPECT_EQ(em->getLivingCount(), MAX_ENTITIES);
    EXPECT_THROW(em->createEntity(), std::runtime_error);

    em->destroyEntity(42);
    EXPECT_EQ(em->createEntity(), 42);
}

TEST(EntityManager, SignatureIsStoredAndResetOnReuse)
{
    auto em = makeManager();
    Entity e = em->createEntity();

    EntitySignature sig;
    sig.set(3);
    sig.set(10);
    em->setSignature(e, sig);
    EXPECT_EQ(em->getSignature(e), sig);

    em->destroyEntity(e);
    EXPECT_THROW(em->setSignature(e, sig), std::out_of_range);

    // Exhaust the free list until e comes back, its signature must be clean
    Entity reused = -1;
    for (std::size_t i = 0; i < MAX_ENTITIES && reused != e; ++i)
        reused = em->createEntity();
    ASSERT_EQ(reused, e);
    EXPECT_TRUE(em->getSignature(e).none());
}
