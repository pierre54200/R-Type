#pragma once

class TestComponent {
public:
    TestComponent();
    ~TestComponent();

    void init();
    void update();

    int getUpdateCount() const;
    bool isInitialized() const;

private:
    int _updateCount = 0;
    bool _initialized = false;
};