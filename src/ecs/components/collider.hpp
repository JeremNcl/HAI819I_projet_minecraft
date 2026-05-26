#pragma once
#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "component.hpp"
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

struct ColliderComponent : public Component {
    
    glm::vec3 size = glm::vec3(0.f);
    glm::vec3 offset = glm::vec3(0.f);

    ColliderComponent() = default;
    ColliderComponent(const glm::vec3 _size, const glm::vec3 _offset)
        : size(_size), offset(_offset) {}

    AABB getAABB(const glm::vec3& entityPosition) const {
        glm::vec3 center = entityPosition + offset;
        return AABB {
            center - (size * 0.5f),
            center + (size * 0.5f)
        };
    }
};

#endif