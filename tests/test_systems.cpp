#include <gtest/gtest.h>

#include <memory>
#include <set>

#include "Registery.hpp"
#include "ECS/Modules/DespawnByRange.hpp"
#include "ECS/Modules/RigidBody.hpp"
#include "ECS/Modules/Transform.hpp"
#include "ECS/System/CollisionSystem.hpp"
#include "ECS/System/DespawnSystem.hpp"
#include "ECS/System/MovementSystem.hpp"

namespace {

using Vec = Math::Vec3<float>;

std::unique_ptr<ECS::Registery> makeRegistery()
{
    auto reg = std::make_unique<ECS::Registery>();
    reg->registerModule<Transform>().registerModule<RigidBody>().registerModule<DespawnByRange>();
    return reg;
}

Entity makeBox(ECS::Registery& reg, Vec pos, Vec size, bool isStatic,
               std::set<uint32_t> mask = {1}, Vec velocity = {0.f, 0.f, 0.f})
{
    Entity e = reg.createEntity();
    reg.addModule(e, Transform{pos, size})
        .addModule(e, RigidBody{velocity, 0.f, mask, isStatic});
    return e;
}

const Vec& posOf(ECS::Registery& reg, Entity e)
{
    return reg.getModule<Transform>(e).position;
}

} // namespace

/* ---------- MovementSystem ---------- */

TEST(MovementSystem, MovesByVelocityTimesDt)
{
    auto reg = makeRegistery();
    reg->registerSystem<MovementSystem>(reg->signatureOf<Transform, RigidBody>());
    Entity e = makeBox(*reg, {10.f, 10.f, 0.f}, {1.f, 1.f, 0.f}, false, {}, {100.f, -50.f, 0.f});

    reg->update(0.5f);

    EXPECT_FLOAT_EQ(posOf(*reg, e).x, 60.f);
    EXPECT_FLOAT_EQ(posOf(*reg, e).y, -15.f);
}

TEST(MovementSystem, IsFrameRateIndependent)
{
    auto reg = makeRegistery();
    reg->registerSystem<MovementSystem>(reg->signatureOf<Transform, RigidBody>());
    Entity oneStep = makeBox(*reg, {0.f, 0.f, 0.f}, {1.f, 1.f, 0.f}, false, {}, {30.f, 0.f, 0.f});

    reg->update(1.f);
    float afterOneBigStep = posOf(*reg, oneStep).x;

    auto reg2 = makeRegistery();
    reg2->registerSystem<MovementSystem>(reg2->signatureOf<Transform, RigidBody>());
    Entity manySteps = makeBox(*reg2, {0.f, 0.f, 0.f}, {1.f, 1.f, 0.f}, false, {}, {30.f, 0.f, 0.f});
    for (int i = 0; i < 60; ++i)
        reg2->update(1.f / 60.f);

    EXPECT_NEAR(posOf(*reg2, manySteps).x, afterOneBigStep, 1e-3f);
}

TEST(MovementSystem, StaticBodiesDoNotMove)
{
    auto reg = makeRegistery();
    reg->registerSystem<MovementSystem>(reg->signatureOf<Transform, RigidBody>());
    Entity e = makeBox(*reg, {10.f, 10.f, 0.f}, {1.f, 1.f, 0.f}, true, {}, {100.f, 100.f, 0.f});

    reg->update(1.f);

    EXPECT_FLOAT_EQ(posOf(*reg, e).x, 10.f);
    EXPECT_FLOAT_EQ(posOf(*reg, e).y, 10.f);
}

class CollisionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reg = makeRegistery();
        reg->registerSystem<CollisionSystem>(reg->signatureOf<Transform, RigidBody>());
    }
    std::unique_ptr<ECS::Registery> reg;
};

TEST_F(CollisionTest, TwoDynamicBoxesArePushedApartEqually)
{
    Entity a = makeBox(*reg, {0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false);
    Entity b = makeBox(*reg, {40.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false);

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, a).x, -5.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).x, 45.f);
    EXPECT_FLOAT_EQ(posOf(*reg, a).y, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).y, 0.f);
}

TEST_F(CollisionTest, ResolvesAlongTheSmallestOverlapAxis)
{
    Entity a = makeBox(*reg, {0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false);
    Entity b = makeBox(*reg, {0.f, 40.f, 0.f}, {50.f, 50.f, 0.f}, false);

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, a).y, -5.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).y, 45.f);
    EXPECT_FLOAT_EQ(posOf(*reg, a).x, 0.f);
}

TEST_F(CollisionTest, StaticWallPushesOnlyTheDynamicBody)
{
    Entity wall = makeBox(*reg, {0.f, 0.f, 0.f}, {20.f, 800.f, 0.f}, true);
    Entity player = makeBox(*reg, {15.f, 100.f, 0.f}, {50.f, 50.f, 0.f}, false);

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, wall).x, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, player).x, 20.f);
}

TEST_F(CollisionTest, DynamicBodyFirstThenWallAlsoWorks)
{
    // Same as above but the player entity is created before the wall
    Entity player = makeBox(*reg, {15.f, 100.f, 0.f}, {50.f, 50.f, 0.f}, false);
    Entity wall = makeBox(*reg, {0.f, 0.f, 0.f}, {20.f, 800.f, 0.f}, true);

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, wall).x, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, player).x, 20.f);
}

TEST_F(CollisionTest, NoSharedMaskMeansNoCollision)
{
    Entity a = makeBox(*reg, {0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false, {1});
    Entity b = makeBox(*reg, {10.f, 10.f, 0.f}, {50.f, 50.f, 0.f}, false, {2});

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, a).x, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).x, 10.f);
}

TEST_F(CollisionTest, EmptyMaskNeverCollides)
{
    Entity a = makeBox(*reg, {0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false, {});
    Entity b = makeBox(*reg, {10.f, 10.f, 0.f}, {50.f, 50.f, 0.f}, false, {1});

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, a).x, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).x, 10.f);
}

TEST_F(CollisionTest, TouchingEdgesAreNotACollision)
{
    Entity a = makeBox(*reg, {0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false);
    Entity b = makeBox(*reg, {50.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, false);

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, a).x, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).x, 50.f);
}

TEST_F(CollisionTest, TwoStaticBodiesAreLeftAlone)
{
    Entity a = makeBox(*reg, {0.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, true);
    Entity b = makeBox(*reg, {10.f, 0.f, 0.f}, {50.f, 50.f, 0.f}, true);

    reg->update(0.f);

    EXPECT_FLOAT_EQ(posOf(*reg, a).x, 0.f);
    EXPECT_FLOAT_EQ(posOf(*reg, b).x, 10.f);
}

TEST(DespawnSystem, DestroysEntitiesBeyondRangeOnly)
{
    auto reg = makeRegistery();
    reg->registerSystem<DespawnSystem>(reg->signatureOf<Transform, DespawnByRange>());

    Entity nearEntity = reg->createEntity();
    reg->addModule(nearEntity, Transform{{90.f, 0.f, 0.f}, {1.f, 1.f, 0.f}})
        .addModule(nearEntity, DespawnByRange{{0.f, 0.f, 0.f}, 100.f});
    Entity farEntity = reg->createEntity();
    reg->addModule(farEntity, Transform{{80.f, 80.f, 0.f}, {1.f, 1.f, 0.f}})
        .addModule(farEntity, DespawnByRange{{0.f, 0.f, 0.f}, 100.f});

    reg->update(0.f);

    EXPECT_TRUE(reg->hasModule<Transform>(nearEntity));
    EXPECT_FALSE(reg->hasModule<Transform>(farEntity));
    EXPECT_EQ(reg->getLivingCount(), 1u);
}

TEST(DespawnSystem, RangeIsMeasuredFromSpawnPosition)
{
    auto reg = makeRegistery();
    reg->registerSystem<DespawnSystem>(reg->signatureOf<Transform, DespawnByRange>());

    Entity e = reg->createEntity();
    reg->addModule(e, Transform{{1050.f, 0.f, 0.f}, {1.f, 1.f, 0.f}})
        .addModule(e, DespawnByRange{{1000.f, 0.f, 0.f}, 100.f});

    reg->update(0.f);

    EXPECT_EQ(reg->getLivingCount(), 1u);
}
