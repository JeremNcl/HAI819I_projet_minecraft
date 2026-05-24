#pragma once
#ifndef VELOCITY_HPP
#define VELOCITY_HPP

#include "component.hpp"
#include <glm/glm.hpp>

struct VelocityComponent : public Component {

    glm::vec3 velocity = glm::vec3(0.f);

    float movementSpeed = 4.f;
    float jumpStrenght = 5.f;
    float sprintMultiplier = 1.5f;
    
    VelocityComponent() = default;
};

#endif