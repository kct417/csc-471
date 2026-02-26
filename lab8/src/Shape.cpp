
#include "Shape.h"
#include <iostream>
#include <cassert>

#include "GLSL.h"
#include "Program.h"

using namespace std;

// copy the data from the shape to this object
void Shape::createShape(tinyobj::shape_t &shape)
{
    posBuf = shape.mesh.positions;
    norBuf = shape.mesh.normals;
    texBuf = shape.mesh.texcoords;
    eleBuf = shape.mesh.indices;
}

void Shape::measure()
{
    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = -std::numeric_limits<float>::max();

    // Go through all vertices to determine min and max of each dimension
    for (size_t v = 0; v < posBuf.size() / 3; v++)
    {
        if (posBuf[3 * v + 0] < minX)
            minX = posBuf[3 * v + 0];
        if (posBuf[3 * v + 0] > maxX)
            maxX = posBuf[3 * v + 0];

        if (posBuf[3 * v + 1] < minY)
            minY = posBuf[3 * v + 1];
        if (posBuf[3 * v + 1] > maxY)
            maxY = posBuf[3 * v + 1];

        if (posBuf[3 * v + 2] < minZ)
            minZ = posBuf[3 * v + 2];
        if (posBuf[3 * v + 2] > maxZ)
            maxZ = posBuf[3 * v + 2];
    }

    min.x = minX;
    min.y = minY;
    min.z = minZ;
    max.x = maxX;
    max.y = maxY;
    max.z = maxZ;
}

glm::mat4 Shape::getNormalizationMatrix(glm::vec3 min, glm::vec3 max) const
{
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 size = max - min;
    float maxExtent = std::max(size.x, std::max(size.y, size.z));
    float scale = 2.0f / maxExtent;
    glm::vec3 translate(-0.5f * (min.x + max.x), -min.y, -0.5f * (min.z + max.z));
    glm::mat4 T = glm::translate(glm::mat4(1.0f), translate);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    return S * T;
}

void Shape::init()
{
    // Initialize the vertex array object
    CHECKED_GL_CALL(glGenVertexArrays(1, &vaoID));
    CHECKED_GL_CALL(glBindVertexArray(vaoID));

    // Send the position array to the GPU
    CHECKED_GL_CALL(glGenBuffers(1, &posBufID));
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
    CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW));

    // Send the normal array to the GPU
    if (norBuf.empty())
    {
        // generate normals from the position and element arrays
        norBuf = vector<float>(posBuf.size(), 0.0f);
        for (size_t i = 0; i < eleBuf.size(); i += 3)
        {
            std::size_t idx0 = eleBuf[i + 0];
            std::size_t idx1 = eleBuf[i + 1];
            std::size_t idx2 = eleBuf[i + 2];
            glm::vec3 v0(posBuf[3 * idx0 + 0], posBuf[3 * idx0 + 1], posBuf[3 * idx0 + 2]);
            glm::vec3 v1(posBuf[3 * idx1 + 0], posBuf[3 * idx1 + 1], posBuf[3 * idx1 + 2]);
            glm::vec3 v2(posBuf[3 * idx2 + 0], posBuf[3 * idx2 + 1], posBuf[3 * idx2 + 2]);
            glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);

            // add the normal to each vertex of the triangle
            for (int j = 0; j < 3; j++)
            {
                norBuf[3 * eleBuf[i + j] + 0] += normal.x;
                norBuf[3 * eleBuf[i + j] + 1] += normal.y;
                norBuf[3 * eleBuf[i + j] + 2] += normal.z;
            }
        }

        // normalize the normals
        for (size_t i = 0; i < norBuf.size(); i += 3)
        {
            glm::vec3 normal(norBuf[i + 0], norBuf[i + 1], norBuf[i + 2]);
            normal = glm::normalize(normal);

            norBuf[i + 0] = normal.x;
            norBuf[i + 1] = normal.y;
            norBuf[i + 2] = normal.z;
        }
    }
    CHECKED_GL_CALL(glGenBuffers(1, &norBufID));
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
    CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW));

    // Send the texture array to the GPU
    if (texBuf.empty())
    {
        texBufID = 0;
    }
    else
    {
        CHECKED_GL_CALL(glGenBuffers(1, &texBufID));
        CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
        CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, texBuf.size() * sizeof(float), &texBuf[0], GL_STATIC_DRAW));
    }

    // Send the element array to the GPU
    CHECKED_GL_CALL(glGenBuffers(1, &eleBufID));
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));
    CHECKED_GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size() * sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW));

    // Unbind the arrays
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Shape::draw(const shared_ptr<Program> prog) const
{
    int h_pos, h_nor, h_tex;
    h_pos = h_nor = h_tex = -1;

    CHECKED_GL_CALL(glBindVertexArray(vaoID));

    // Bind position buffer
    h_pos = prog->getAttribute("vertPos");
    GLSL::enableVertexAttribArray(h_pos);
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
    CHECKED_GL_CALL(glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0));

    // Bind normal buffer
    h_nor = prog->getAttribute("vertNor");
    if (h_nor != -1 && norBufID != 0)
    {
        GLSL::enableVertexAttribArray(h_nor);
        CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
        CHECKED_GL_CALL(glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0));
    }

    // Bind texcoords buffer
    if (h_tex != -1 && texBufID != 0)
    {
        h_tex = prog->getAttribute("vertTex");
        GLSL::enableVertexAttribArray(h_tex);
        CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, texBufID));
        CHECKED_GL_CALL(glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0));
    }

    // Bind element buffer
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));

    // Draw
    CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, (int)eleBuf.size(), GL_UNSIGNED_INT, (const void *)0));

    // Disable and unbind
    if (h_tex != -1)
    {
        GLSL::disableVertexAttribArray(h_tex);
    }
    if (h_nor != -1)
    {
        GLSL::disableVertexAttribArray(h_nor);
    }
    GLSL::disableVertexAttribArray(h_pos);
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
