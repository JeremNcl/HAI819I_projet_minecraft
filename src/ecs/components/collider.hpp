#pragma once
#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "component.hpp"
#include <glm/glm.hpp>

struct ColliderComponent : public Component {
    
    glm::vec3 size = glm::vec3(0.f);
    glm::vec3 offset = glm::vec3(0.f);

    ColliderComponent() = default;
    ColliderComponent(const glm::vec3 _size, const glm::vec3 _offset)
        : size(_size), offset(_offset) {}
};

#endif