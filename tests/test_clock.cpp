#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "ECS/Clock/Clock.hpp"

using namespace std::chrono_literals;

TEST(Clock, TargetNotReadyImmediately)
{
    Clock::Clock clock;
    clock.setTargetTime(10.f);
    EXPECT_FALSE(clock.isReady());
}

TEST(Clock, TargetReadyAfterDelay)
{
    Clock::Clock clock;
    clock.setTargetTime(0.02f);
    std::this_thread::sleep_for(40ms);
    EXPECT_TRUE(clock.isReady());
}

TEST(Clock, ZeroTargetIsReadyRightAway)
{
    Clock::Clock clock;
    clock.setTargetTime(0.f);
    EXPECT_TRUE(clock.isReady());
}

TEST(Clock, SetTargetTimeRestartsTheCountdown)
{
    Clock::Clock clock;
    clock.setTargetTime(0.02f);
    std::this_thread::sleep_for(40ms);
    ASSERT_TRUE(clock.isReady());

    clock.setTargetTime(10.f);
    EXPECT_FALSE(clock.isReady());
}

TEST(Clock, PausedClockIsNeverReady)
{
    Clock::Clock clock;
    clock.setTargetTime(0.f);
    clock.pause();
    EXPECT_FALSE(clock.isReady());

    clock.start();
    EXPECT_TRUE(clock.isReady());
}

TEST(Clock, DeltaTimeMeasuresElapsedTimeBetweenCalls)
{
    Clock::Clock clock;
    clock.getDeltaTime();
    std::this_thread::sleep_for(20ms);
    float dt = clock.getDeltaTime();

    EXPECT_GE(dt, 0.015f);
    EXPECT_LT(dt, 1.f);
}

TEST(Clock, StoppedClockReturnsZeroDelta)
{
    Clock::Clock clock(false);
    std::this_thread::sleep_for(5ms);
    EXPECT_FLOAT_EQ(clock.getDeltaTime(), 0.f);
}

TEST(Clock, RestartResetsTotalElapsedTime)
{
    Clock::Clock clock;
    std::this_thread::sleep_for(30ms);
    EXPECT_GE(clock.getTotalElapsedTime(), 0.025f);

    clock.restart();
    EXPECT_LT(clock.getTotalElapsedTime(), 0.025f);
}
