#include "ModelUtils.h"

#include <limits>
#include <glm/gtc/matrix_transform.hpp>

namespace ModelUtils
{
    std::pair<glm::vec3, glm::vec3> measureModel(const std::vector<std::shared_ptr<Shape>> &shapes)
    {
        glm::vec3 globalMin(std::numeric_limits<float>::max());
        glm::vec3 globalMax(-std::numeric_limits<float>::max());

        for (auto &shape : shapes)
        {
            shape->measure();
            globalMin = glm::min(globalMin, shape->min);
            globalMax = glm::max(globalMax, shape->max);
        }

        return {globalMin, globalMax};
    }

    glm::mat4
    getNormalizationMatrix(const glm::vec3 &min, const glm::vec3 &max)
    {
        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 size = max - min;
        float maxExtent = std::max(size.x, std::max(size.y, size.z));
        glm::mat4 Translate = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f * (min.x + max.x), -min.y, -0.5f * (min.z + max.z)));
        glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f / maxExtent));
        return Scale * Translate;
    }

    bool sphereCollision(glm::vec3 c1, float r1, glm::vec3 c2, float r2)
    {
        return glm::length(c1 - c2) < (r1 + r2);
    }

    void resolveCameraCollision(glm::vec3 &eye, glm::vec3 objCenter, float cameraRadius, float objRadius)
    {
        glm::vec3 dir = eye - objCenter;
        float dist = glm::length(dir);
        float minDist = cameraRadius + objRadius;

        if (dist < minDist)
        {
            dir = glm::normalize(dir);
            eye = objCenter + dir * minDist;
        }
    }
}
