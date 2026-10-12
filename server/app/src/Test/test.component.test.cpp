#include <gtest/gtest.h>

#include "test.component.hpp"

// A la creation, le composant n'est pas initialise
TEST(TestComponent, NotInitializedByDefault) {
    TestComponent component;

    EXPECT_FALSE(component.isInitialized());
    EXPECT_EQ(component.getUpdateCount(), 0);
}

// init() marque le composant comme initialise
TEST(TestComponent, InitSetsInitialized) {
    TestComponent component;

    component.init();

    EXPECT_TRUE(component.isInitialized());
}

// Chaque update() augmente le compteur de 1
TEST(TestComponent, UpdateIncrementsCounter) {
    TestComponent component;
    component.init();

    component.update();
    component.update();
    component.update();

    EXPECT_EQ(component.getUpdateCount(), 3);
}

// update() ne fait rien tant que init() n'a pas ete appele
TEST(TestComponent, UpdateDoesNothingBeforeInit) {
    TestComponent component;

    component.update();

    EXPECT_EQ(component.getUpdateCount(), 0);
}

// init() remet le compteur a zero
TEST(TestComponent, InitResetsCounter) {
    TestComponent component;
    component.init();
    component.update();
    component.update();

    component.init();

    EXPECT_EQ(component.getUpdateCount(), 0);
}