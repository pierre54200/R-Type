#pragma once
#include <cstdint>
#include <stdexcept>

namespace Math
{
    template<typename T>
    class Vec3
    {
        public:
            Vec3() = delete;

            Vec3(T x, T y, T z):
                x(x), y(y), z(z)
            {
            }

            T   x;
            T   y;
            T   z;

            T&  operator[](uint32_t value)
            {
                switch (value) {
                    case 0:  return x;
                    case 1:  return y;
                    case 2:  return z;
                    default: throw std::runtime_error("Index is invalid");
                }
            }

            Vec3<T> operator+(const Vec3<T>& other) const
            {
                return Vec3<T>(x + other.x, y + other.y, z + other.z);
            }

            Vec3<T> operator-(const Vec3<T>& other) const
            {
                return Vec3<T>(x - other.x, y - other.y, z - other.z);
            }

            Vec3<T> operator*(T nb) const
            {
                return Vec3<T>(x * nb, y * nb, z * nb);
            }

            Vec3<T>& operator+=(const Vec3<T>& other)
            {
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
            }
    };
}
