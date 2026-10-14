#include <gtest/gtest.h>

#include "{{header}}"

TEST({{ClassName}}, ShouldBeASingleton) {
    EXPECT_EQ(&{{ClassName}}::instance(), &{{ClassName}}::instance());
}
