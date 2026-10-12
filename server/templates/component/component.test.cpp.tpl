#include <gtest/gtest.h>

#include "{{header}}"

TEST({{ClassName}}, ShouldCreate) {
    {{ClassName}} component;
    SUCCEED();
}

TEST({{ClassName}}, ShouldInitAndUpdate) {
    {{ClassName}} component;
    EXPECT_NO_THROW(component.init());
    EXPECT_NO_THROW(component.update());
}
