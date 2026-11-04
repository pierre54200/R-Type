#include <gtest/gtest.h>

#include <stdexcept>

#include "ECS/Vec3.hpp"

using Vec = Math::Vec3<float>;

TEST(Vec3, Addition)
{
    Vec r = Vec(1.f, 2.f, 3.f) + Vec(10.f, 20.f, 30.f);
    EXPECT_FLOAT_EQ(r.x, 11.f);
    EXPECT_FLOAT_EQ(r.y, 22.f);
    EXPECT_FLOAT_EQ(r.z, 33.f);
}

TEST(Vec3, Subtraction)
{
    Vec r = Vec(1.f, 2.f, 3.f) - Vec(10.f, 20.f, 30.f);
    EXPECT_FLOAT_EQ(r.x, -9.f);
    EXPECT_FLOAT_EQ(r.y, -18.f);
    EXPECT_FLOAT_EQ(r.z, -27.f);
}

TEST(Vec3, ScalarMultiplication)
{
    Vec r = Vec(1.f, -2.f, 0.5f) * 4.f;
    EXPECT_FLOAT_EQ(r.x, 4.f);
    EXPECT_FLOAT_EQ(r.y, -8.f);
    EXPECT_FLOAT_EQ(r.z, 2.f);
}

TEST(Vec3, PlusEqualsModifiesInPlaceAndChains)
{
    Vec v(1.f, 1.f, 1.f);
    (v += Vec(1.f, 2.f, 3.f)) += Vec(1.f, 1.f, 1.f);
    EXPECT_FLOAT_EQ(v.x, 3.f);
    EXPECT_FLOAT_EQ(v.y, 4.f);
    EXPECT_FLOAT_EQ(v.z, 5.f);
}

TEST(Vec3, IndexOperatorReadsAndWrites)
{
    Vec v(1.f, 2.f, 3.f);
    EXPECT_FLOAT_EQ(v[0], 1.f);
    EXPECT_FLOAT_EQ(v[1], 2.f);
    EXPECT_FLOAT_EQ(v[2], 3.f);

    v[1] = 42.f;
    EXPECT_FLOAT_EQ(v.y, 42.f);
}

TEST(Vec3, IndexOutOfRangeThrows)
{
    Vec v(0.f, 0.f, 0.f);
    EXPECT_THROW(v[3], std::runtime_error);
}

TEST(Vec3, WorksWithIntegers)
{
    Math::Vec3<int> v = Math::Vec3<int>(1, 2, 3) * 2;
    EXPECT_EQ(v.x, 2);
    EXPECT_EQ(v.z, 6);
}
