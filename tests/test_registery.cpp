#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ModuleManager.hpp"
#include "Registery.hpp"

namespace {

struct Position { float x = 0.f; float y = 0.f; };
struct Velocity { float dx = 0.f; float dy = 0.f; };
struct Name     { std::string value; };

class RecordingSystem : public ECS::System
{
public:
    void update(ECS::Registery&, float dt) override
    {
        lastDt = dt;
        calls++;
        order.push_back(tag);
    }

    float lastDt = -1.f;
    int calls = 0;
    std::string tag = "A";
    static inline std::vector<std::string> order;
};

class OtherSystem : public RecordingSystem
{
public:
    OtherSystem() { tag = "B"; }
};

class MoveSystem : public ECS::System
{
public:
    void update(ECS::Registery& reg, float dt) override
    {
        reg.view<Position, Velocity>(getEntities()).each(
            [dt](Entity, Position& p, Velocity& v) {
                p.x += v.dx * dt;
                p.y += v.dy * dt;
            });
    }
};

class StopAfterThree : public ECS::System
{
public:
    void update(ECS::Registery& reg, float) override
    {
        if (++calls == 3)
            reg.stop();
    }
    int calls = 0;
};

std::unique_ptr<ECS::Registery> makeRegistery()
{
    auto reg = std::make_unique<ECS::Registery>();
    reg->registerModule<Position>().registerModule<Velocity>().registerModule<Name>();
    return reg;
}

}

TEST(ModuleManager, RegisteringTwiceThrows)
{
    ECS::ModuleManager mm;
    mm.registerModule<Position>();
    EXPECT_THROW(mm.registerModule<Position>(), std::logic_error);
}

TEST(ModuleManager, UnregisteredModuleThrows)
{
    ECS::ModuleManager mm;
    EXPECT_THROW(mm.getId<Position>(), std::logic_error);
    EXPECT_THROW(mm.add<Position>(0, {}), std::logic_error);
}

TEST(ModuleManager, IdsAreSequentialAndDistinct)
{
    ECS::ModuleManager mm;
    mm.registerModule<Position>();
    mm.registerModule<Velocity>();

    EXPECT_EQ(mm.getId<Position>(), 0u);
    EXPECT_EQ(mm.getId<Velocity>(), 1u);
}

TEST(ModuleManager, AddGetHasRemove)
{
    ECS::ModuleManager mm;
    mm.registerModule<Position>();

    EXPECT_FALSE(mm.has<Position>(5));
    mm.add<Position>(5, {1.f, 2.f});
    ASSERT_TRUE(mm.has<Position>(5));
    EXPECT_FLOAT_EQ(mm.get<Position>(5).y, 2.f);

    mm.add<Position>(5, {9.f, 9.f}); // overwrite
    EXPECT_FLOAT_EQ(mm.get<Position>(5).x, 9.f);

    mm.remove<Position>(5);
    EXPECT_FALSE(mm.has<Position>(5));
    EXPECT_THROW(mm.get<Position>(5), std::out_of_range);
}

TEST(ModuleManager, EntityDestroyedClearsEveryModule)
{
    ECS::ModuleManager mm;
    mm.registerModule<Position>();
    mm.registerModule<Velocity>();
    mm.add<Position>(1, {});
    mm.add<Velocity>(1, {});
    mm.add<Position>(2, {});

    mm.entityDestroyed(1);

    EXPECT_FALSE(mm.has<Position>(1));
    EXPECT_FALSE(mm.has<Velocity>(1));
    EXPECT_TRUE(mm.has<Position>(2));
}

/* ---------- Registery and systems ---------- */

TEST(Registery, SystemOnlySeesEntitiesWithAllRequiredModules)
{
    auto reg = makeRegistery();
    auto sys = reg->registerSystem<RecordingSystem>(reg->signatureOf<Position, Velocity>());

    Entity both = reg->createEntity();
    reg->addModule(both, Position{}).addModule(both, Velocity{});
    Entity onlyPos = reg->createEntity();
    reg->addModule(onlyPos, Position{});

    EXPECT_EQ(sys->getEntityCount(), 1u);
    EXPECT_TRUE(sys->getEntities().count(both));
}

TEST(Registery, RemovingAModuleRemovesEntityFromSystem)
{
    auto reg = makeRegistery();
    auto sys = reg->registerSystem<RecordingSystem>(reg->signatureOf<Position, Velocity>());
    Entity e = reg->createEntity();
    reg->addModule(e, Position{}).addModule(e, Velocity{});

    reg->removeModule<Velocity>(e);

    EXPECT_EQ(sys->getEntityCount(), 0u);
    EXPECT_FALSE(reg->hasModule<Velocity>(e));
    EXPECT_TRUE(reg->hasModule<Position>(e));
}

TEST(Registery, DestroyEntityCleansModulesAndSystems)
{
    auto reg = makeRegistery();
    auto sys = reg->registerSystem<RecordingSystem>(reg->signatureOf<Position>());
    Entity e = reg->createEntity();
    reg->addModule(e, Position{});

    reg->destroyEntity(e);

    EXPECT_EQ(sys->getEntityCount(), 0u);
    EXPECT_FALSE(reg->hasModule<Position>(e));
    EXPECT_EQ(reg->getLivingCount(), 0u);
}

TEST(Registery, AddModuleOnDeadEntityThrows)
{
    auto reg = makeRegistery();
    Entity e = reg->createEntity();
    reg->destroyEntity(e);

    EXPECT_THROW(reg->addModule(e, Position{}), std::out_of_range);
}

TEST(Registery, EmptySignatureMatchesEveryEntity)
{
    auto reg = makeRegistery();
    auto sys = reg->registerSystem<RecordingSystem>(reg->signatureOf<>());
    Entity e = reg->createEntity();
    reg->addModule(e, Name{"x"});

    EXPECT_EQ(sys->getEntityCount(), 1u);
}

TEST(Registery, RegisteringSameSystemTwiceThrows)
{
    auto reg = makeRegistery();
    reg->registerSystem<RecordingSystem>(reg->signatureOf<>());
    EXPECT_THROW(reg->registerSystem<RecordingSystem>(reg->signatureOf<>()), std::logic_error);
}

TEST(Registery, UpdateRunsSystemsInRegistrationOrderWithDt)
{
    RecordingSystem::order.clear();
    auto reg = makeRegistery();
    auto b = reg->registerSystem<OtherSystem>(reg->signatureOf<>());
    auto a = reg->registerSystem<RecordingSystem>(reg->signatureOf<>());

    reg->update(0.25f);

    EXPECT_EQ(RecordingSystem::order, (std::vector<std::string>{"B", "A"}));
    EXPECT_FLOAT_EQ(a->lastDt, 0.25f);
    EXPECT_EQ(b->calls, 1);
}

TEST(Registery, ViewGivesMutableAccessToModules)
{
    auto reg = makeRegistery();
    reg->registerSystem<MoveSystem>(reg->signatureOf<Position, Velocity>());
    Entity e = reg->createEntity();
    reg->addModule(e, Position{1.f, 1.f}).addModule(e, Velocity{2.f, -4.f});

    reg->update(0.5f);

    EXPECT_FLOAT_EQ(reg->getModule<Position>(e).x, 2.f);
    EXPECT_FLOAT_EQ(reg->getModule<Position>(e).y, -1.f);
}

TEST(Registery, StopEndsTheRunLoop)
{
    auto reg = makeRegistery();
    auto sys = reg->registerSystem<StopAfterThree>(reg->signatureOf<>());

    reg->run();

    EXPECT_EQ(sys->calls, 3);
}
