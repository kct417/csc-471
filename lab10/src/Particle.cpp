//
// sueda - geometry edits Z. Wood
// 3/16
//

#include <iostream>
#include "Particle.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"

float randFloat(float l, float h)
{
    float r = rand() / (float)RAND_MAX;
    return (1.0f - r) * l + r * h;
}

Particle::Particle(vec3 start) : charge(1.0f),
                                 m(1.0f),
                                 d(0.0f),
                                 x(start),
                                 v(0.0f, 0.0f, 0.0f),
                                 lifespan(1.0f),
                                 tStart(0.0f),
                                 tEnd(0.0f),
                                 scale(1.0f),
                                 color(1.0f, 1.0f, 1.0f, 1.0f),
                                 active(false)
{
}

Particle::~Particle()
{
}

void Particle::load(vec3 start, float startOffset)
{
    // Random initialization
    // rebirth(startOffset, start);
    tStart = startOffset;
    active = false;
    color.a = 0.0f;
}

/* all particles born at the origin */
void Particle::rebirth(float t, vec3 start)
{
    charge = randFloat(0.0f, 1.0f) < 0.5 ? -1.0f : 1.0f;
    m = 1.0f;
    d = randFloat(0.0f, 0.02f);

    // x = start;
    // Random point on sphere of radius 0.5
    float theta = randFloat(0.0f, 2.0f * M_PI);
    float phi = randFloat(0.0f, M_PI);

    float r = 0.75f;

    x.x = start.x + r * sin(phi) * cos(theta);
    x.y = start.y + r * sin(phi) * sin(theta);
    x.z = start.z + r * cos(phi);

    // v.x = randFloat(-0.27f, 0.3f);
    // v.y = randFloat(-0.1f, 0.9f);
    // v.z = randFloat(-0.3f, 0.27f);

    vec3 outward = normalize(x - start);
    float speed = randFloat(0.4f, 0.8f);
    v = outward * speed;

    // Add upward bias
    v.y += randFloat(0.2f, 0.6f);

    // lifespan = randFloat(100.0f, 200.0f);
    lifespan = randFloat(25.0f, 50.0f);
    tStart = t;
    tEnd = t + lifespan;
    scale = randFloat(0.2, 1.0f);
    // color.r = randFloat(0.0f, 0.1f);
    // color.g = randFloat(0.0f, 0.1f);
    // color.b = randFloat(0.25f, 0.5f);
    color.r = randFloat(0.0f, 1.0f);
    color.g = randFloat(0.0f, 1.0f);
    color.b = randFloat(0.25f, 1.0f);
    color.a = 1.0f;
}

void Particle::update(float t, float h, const vec3 &g, const vec3 start)
{
    // if (t > tEnd)
    // {
    //     rebirth(t, start);
    // }

    if (!active)
    {
        if (t >= tStart)
        {
            rebirth(t, start);
            active = true;
            color.a = 1.0f;
        }
        else
            return;
    }

    // very simple update
    // x += h * v;
    // To do - how do you want to update the forces?
    // color.a = (tEnd - t) / lifespan;

    vec3 force = m * g;
    vec3 outward = glm::normalize(x - start);
    force += 0.01f * outward;
    force += -d * v;

    vec3 a = 0.01f * force / m;
    v += 0.1f * a * h;
    x += 0.1f * v * h;

    float lifeRatio = (tEnd - t) / lifespan;
    color.a = lifeRatio;

    if (t >= tEnd)
        rebirth(t, start);
}
