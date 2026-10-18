#include "Clock.hpp"

using HRC = std::chrono::high_resolution_clock;

/*=================
    CTOR
=================*/
Clock::Clock::Clock()
    : _elapsed(0.f), _ticking(true)
{
    this->_start    = HRC::now();
    this->_lastTime = this->_start;
}

Clock::Clock::Clock(bool state)
    : _elapsed(0.f), _ticking(state)
{
    this->_start    = HRC::now();
    this->_lastTime = this->_start;
}

/*=================
    METHODS
=================*/
void Clock::Clock::start()
{
    this->_ticking = true;

}
void Clock::Clock::stop()
{
    this->_ticking = false;
}

void Clock::Clock::pause()
{
    this->_ticking = false;
}

void Clock::Clock::restart()
{
    this->_start    = HRC::now();
    this->_lastTime = this->_start;
    this->_elapsed  = 0.f;
    this->_ticking  = true;
}

void Clock::Clock::tick()
{
    if (!this->_ticking) return;

    this->_lastTime = HRC::now();
    this->_elapsed  = 0.f;
}

/*=================
    GETTERS
=================*/
float Clock::Clock::getDeltaTime()
{
    if (!this->_ticking) return 0.f;

    auto now      = HRC::now();
    this->_elapsed      = Duration(now - this->_lastTime).count();
    this->_lastTime     = now;
    return this->_elapsed;
}

float Clock::Clock::getTotalElapsedTime()
{
    return Duration(HRC::now() - this->_start).count();
}

void Clock::Clock::setTargetTime(float seconds)
{
    _targetTime = seconds;

    _start = HRC::now();
    _lastTime = _start;
    _elapsed = 0.f;
    _ticking = true;
}

bool Clock::Clock::isReady() const
{
    if (!_ticking)
        return false;

    float elapsed = Duration(HRC::now() - _start).count();
    return elapsed >= _targetTime;
}
