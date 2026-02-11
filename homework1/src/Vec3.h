#pragma once

#include <cmath>
#include <iostream>

#include "util.h"

using std::sqrt;

class vec3
{
public:
    vec3() : m_x(0), m_y(0), m_z(0) {}
    vec3(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}
    ~vec3() = default;

    inline float getX() const { return m_x; }
    inline float getY() const { return m_y; }
    inline float getZ() const { return m_z; }
    inline void setX(float x) { m_x = x; }
    inline void setY(float y) { m_y = y; }
    inline void setZ(float z) { m_z = z; }

    vec3 &operator+=(const vec3 &v)
    {
        m_x += v.m_x;
        m_y += v.m_y;
        m_z += v.m_z;
        return *this;
    }

    vec3 &operator*=(const float Sc)
    {
        m_x *= Sc;
        m_y *= Sc;
        m_z *= Sc;
        return *this;
    }

private:
    float m_x, m_y, m_z;
};

inline std::ostream &operator<<(std::ostream &out, const vec3 &v)
{
    out << "vec3(" << v.getX() << ", " << v.getY() << ", " << v.getZ() << ")";
    return out;
}
