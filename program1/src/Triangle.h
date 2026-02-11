#ifndef TRI_H
#define TRI_H

#include <iostream>
#include <vector>
#include "Vec3.h"

/* Bounding Box */
typedef struct
{
    float minX, minY;
    float maxX, maxY;
} BBox;

/* triangle */
class Triangle
{
public:
    Triangle(Vec3 v0, Vec3 v1, Vec3 v2)
        : m_v0{v0.getX(), v0.getY(), v0.getZ()}, m_v1{v1.getX(), v1.getY(), v1.getZ()}, m_v2{v2.getX(), v2.getY(), v2.getZ()} {}
    Triangle(std::vector<float> &posBuf, std::vector<unsigned int> &triBuf, size_t triIndex, float translationX, float translationY, float scale)
    {
        int posBufIdx0 = triBuf[triIndex];
        int posBufIdx1 = triBuf[triIndex + 1];
        int posBufIdx2 = triBuf[triIndex + 2];

        float x0 = posBuf[3 * posBufIdx0];
        float y0 = posBuf[3 * posBufIdx0 + 1];
        float z0 = posBuf[3 * posBufIdx0 + 2];
        float sx0 = translationX + x0 * scale;
        float sy0 = translationY + y0 * scale;
        m_v0 = Vec3(sx0, sy0, z0);

        float x1 = posBuf[3 * posBufIdx1];
        float y1 = posBuf[3 * posBufIdx1 + 1];
        float sx1 = translationX + x1 * scale;
        float sy1 = translationY + y1 * scale;
        float z1 = posBuf[3 * posBufIdx1 + 2];
        m_v1 = Vec3(sx1, sy1, z1);

        float x2 = posBuf[3 * posBufIdx2];
        float y2 = posBuf[3 * posBufIdx2 + 1];
        float sx2 = translationX + x2 * scale;
        float sy2 = translationY + y2 * scale;
        float z2 = posBuf[3 * posBufIdx2 + 2];
        m_v2 = Vec3(sx2, sy2, z2);
    }

    Vec3 getV0() const { return m_v0; }
    float getX0() const { return m_v0.getX(); }
    float getY0() const { return m_v0.getY(); }
    float getZ0() const { return m_v0.getZ(); }

    Vec3 getV1() const { return m_v1; }
    float getX1() const { return m_v1.getX(); }
    float getY1() const { return m_v1.getY(); }
    float getZ1() const { return m_v1.getZ(); }

    Vec3 getV2() const { return m_v2; }
    float getX2() const { return m_v2.getX(); }
    float getY2() const { return m_v2.getY(); }
    float getZ2() const { return m_v2.getZ(); }

    BBox getBBox() const { return m_BBox; }

    void computeBBox();

private:
    Vec3 m_v0, m_v1, m_v2;
    BBox m_BBox;
};

void Triangle::computeBBox()
{
    m_BBox.minX = m_v0.getX();
    m_BBox.minY = m_v0.getY();
    m_BBox.maxX = m_v0.getX();
    m_BBox.maxY = m_v0.getY();
    if (m_v1.getX() < m_BBox.minX)
        m_BBox.minX = m_v1.getX();
    if (m_v2.getX() < m_BBox.minX)
        m_BBox.minX = m_v2.getX();

    if (m_v1.getY() < m_BBox.minY)
        m_BBox.minY = m_v1.getY();
    if (m_v2.getY() < m_BBox.minY)
        m_BBox.minY = m_v2.getY();

    if (m_v1.getX() > m_BBox.maxX)
        m_BBox.maxX = m_v1.getX();
    if (m_v2.getX() > m_BBox.maxX)
        m_BBox.maxX = m_v2.getX();

    if (m_v1.getY() > m_BBox.maxY)
        m_BBox.maxY = m_v1.getY();
    if (m_v2.getY() > m_BBox.maxY)
        m_BBox.maxY = m_v2.getY();
}

#endif
