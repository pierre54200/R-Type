#include "test.component.hpp"

TestComponent::TestComponent() {}

TestComponent::~TestComponent() {}

void TestComponent::init() {
    _updateCount = 0;
    _initialized = true;
}

void TestComponent::update() {
    if (_initialized)
        _updateCount++;
}

int TestComponent::getUpdateCount() const { return _updateCount; }

bool TestComponent::isInitialized() const { return _initialized; }