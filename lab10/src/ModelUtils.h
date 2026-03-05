#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Shape.h"

namespace ModelUtils
{
    std::pair<glm::vec3, glm::vec3>
    measureModel(const std::vector<std::shared_ptr<Shape>> &shapes);

    glm::mat4
    getNormalizationMatrix(const glm::vec3 &min,
                           const glm::vec3 &max);
}
