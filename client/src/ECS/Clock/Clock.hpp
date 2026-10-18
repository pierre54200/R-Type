#pragma once

#include <chrono>

namespace Clock
{
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration  = std::chrono::duration<float>;

    class Clock
    {
        public:
            Clock();
            Clock(bool);
            ~Clock() = default;

            void  start();
            void  stop();
            void  restart();
            void  pause();
            void  tick();

            float getDeltaTime();
            float getTotalElapsedTime();

            void    setTargetTime(float);
            bool    isReady() const;
        private:
            TimePoint _start;
            TimePoint _lastTime;
            float     _elapsed;
            bool      _ticking;
            float     _targetTime;
    };
}
