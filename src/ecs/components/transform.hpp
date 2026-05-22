#pragma once

#include "component.hpp"
#include <glm/glm.hpp>

struct TransformComponent : public Component {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    TransformComponent() = default;
    TransformComponent(const glm::vec3& pos, const glm::vec3& s = glm::vec3(1.0f))
        : position(pos), scale(s) {}
};
