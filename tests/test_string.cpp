#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ECS/String/String.hpp"

static std::vector<std::string> toStd(const std::vector<String>& parts)
{
    return {parts.begin(), parts.end()};
}

TEST(String, SplitOnChar)
{
    String s("a,b,c");
    EXPECT_EQ(toStd(s.split(',')), (std::vector<std::string>{"a", "b", "c"}));
}

TEST(String, SplitWithoutSeparatorReturnsWholeString)
{
    String s("abc");
    EXPECT_EQ(toStd(s.split(',')), (std::vector<std::string>{"abc"}));
}

TEST(String, SplitKeepsEmptyFields)
{
    String s(",a,,b,");
    EXPECT_EQ(toStd(s.split(',')), (std::vector<std::string>{"", "a", "", "b", ""}));
}

TEST(String, SplitEmptyStringGivesOneEmptyField)
{
    String s("");
    EXPECT_EQ(toStd(s.split(',')), (std::vector<std::string>{""}));
}

TEST(String, SplitOnMultiCharSeparator)
{
    String s("one::two::three");
    EXPECT_EQ(toStd(s.split(String("::"))), (std::vector<std::string>{"one", "two", "three"}));
}

TEST(String, DefaultSplitUsesSpaces)
{
    String s("/join my lobby");
    EXPECT_EQ(toStd(s.split()), (std::vector<std::string>{"/join", "my", "lobby"}));
}

TEST(String, Contains)
{
    String s("hello world");
    EXPECT_TRUE(s.contains('w'));
    EXPECT_FALSE(s.contains('z'));
    EXPECT_TRUE(s.contains(String("lo wo")));
    EXPECT_FALSE(s.contains(String("bye")));
}

TEST(String, ToStdString)
{
    String s("abc");
    EXPECT_EQ(s.toStdString(), std::string("abc"));
}

TEST(String, DISABLED_ConstructorKeepsEmbeddedNullBytes)
{
    std::string raw("ab\0cd", 5);
    String s(raw);
    EXPECT_EQ(s.size(), 5u);
}
