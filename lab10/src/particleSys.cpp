#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <algorithm>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include "particleSys.h"
#include "GLSL.h"

using namespace std;

particleSys::particleSys(vec3 source)
{

    numP = 300;
    t = 0.0f;
    h = 0.01f;
    g = vec3(0.0f, -0.098, 0.0f);
    start = source;
    theCamera = glm::mat4(1.0);
}

void particleSys::gpuSetup()
{

    cout << "start: " << start.x << " " << start.y << " " << start.z << endl;
    for (int i = 0; i < numP; i++)
    {
        points[i * 3 + 0] = start.x;
        points[i * 3 + 1] = start.y;
        points[i * 3 + 2] = start.z;

        auto particle = make_shared<Particle>(start);
        particles.push_back(particle);
        float r = rand() / (float)RAND_MAX;
        float offset = (1.0f - r) * 0.0f + r * 200.0f;
        particle->load(start, offset);

        // initial color
        vec4 c = particle->getColor();
        pointColors[i * 4 + 0] = c.r;
        pointColors[i * 4 + 1] = c.g;
        pointColors[i * 4 + 2] = c.b;
        pointColors[i * 4 + 3] = c.a;
    }

    // generate the VAO
    glGenVertexArrays(1, &vertArrObj);
    glBindVertexArray(vertArrObj);

    // generate vertex buffer to hand off to OGL - using instancing
    glGenBuffers(1, &vertBuffObj);
    // set the current state to focus on our vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffObj);
    // actually memcopy the data - only do this once
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), &points[0], GL_STREAM_DRAW);

    // Color buffer
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), &pointColors[0], GL_STREAM_DRAW);

    assert(glGetError() == GL_NO_ERROR);
}

void particleSys::reSet()
{
    for (int i = 0; i < numP; i++)
    {
        float r = rand() / (float)RAND_MAX;
        float offset = (1.0f - r) * 0.0f + r * 200.0f;
        particles[i]->load(start, offset);
    }
}

void particleSys::drawMe(std::shared_ptr<Program> prog)
{

    glBindVertexArray(vertArrObj);

    int h_pos = prog->getAttribute("vertPos");
    int h_col = prog->getAttribute("vertColor");

    GLSL::enableVertexAttribArray(h_pos);
    // std::cout << "Any Gl errors1: " << glGetError() << std::endl;
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffObj);
    // std::cout << "Any Gl errors2: " << glGetError() << std::endl;
    glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(h_pos, 1);

    if (h_col != -1)
    {
        GLSL::enableVertexAttribArray(h_col);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(h_col, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(h_col, 1);
    }

    // Draw the points !
    glDrawArraysInstanced(GL_POINTS, 0, 1, numP);

    // glVertexAttribDivisor(0, 0);
    // glVertexAttribDivisor(1, 0);

    // Reset divisors
    if (h_pos != -1)
        glVertexAttribDivisor(h_pos, 0);
    if (h_col != -1)
        glVertexAttribDivisor(h_col, 0);

    // Disable attributes
    if (h_pos != -1)
        GLSL::disableVertexAttribArray(h_pos);
    if (h_col != -1)
        GLSL::disableVertexAttribArray(h_col);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDisableVertexAttribArray(0);
}

void particleSys::update()
{

    vec3 pos;
    vec4 col;

    // update the particles
    for (auto particle : particles)
    {
        float r = rand() / (float)RAND_MAX;
        float offset = (1.0f - r) * 0.0f + r * 200.0f;
        particle->update(t, h, g, start);
        // t += h;
    }
    t += h;

    // Sort the particles by Z
    // temp->rotate(camRot, vec3(0, 1, 0));
    // be sure that camera matrix is updated prior to this update
    vec3 s, t, sk;
    vec4 p;
    quat r;
    glm::decompose(theCamera, s, r, t, sk, p);
    sorter.C = glm::toMat4(r);
    sort(particles.begin(), particles.end(), sorter);

    // go through all the particles and update the CPU buffer
    for (int i = 0; i < numP; i++)
    {
        pos = particles[i]->getPosition();
        col = particles[i]->getColor();
        points[i * 3 + 0] = pos.x;
        points[i * 3 + 1] = pos.y;
        points[i * 3 + 2] = pos.z;
        /*  To do - how can you integrate unique colors per particle? */
        pointColors[i * 4 + 0] = col.r + col.a / 10;
        pointColors[i * 4 + 1] = col.g + col.g / 10;
        pointColors[i * 4 + 2] = col.b + col.b / 10;
        pointColors[i * 4 + 3] = col.a;
    }

    // update the GPU data
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * numP * 3, points);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * numP * 4, pointColors);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
