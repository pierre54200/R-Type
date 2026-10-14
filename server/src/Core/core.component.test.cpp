#include <gtest/gtest.h>

#include "core.component.hpp"

TEST(CoreComponent, ShouldCreate) {
    CoreComponent component;
    SUCCEED();
}

TEST(CoreComponent, ShouldInitAndUpdate) {
    CoreComponent component;
    EXPECT_NO_THROW(component.init());
    EXPECT_NO_THROW(component.update());
}
