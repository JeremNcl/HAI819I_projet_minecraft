#pragma once
#ifndef CAMERA_HPP
#define CAMERA_HPP


#include "component.hpp"
#include <glm/glm.hpp>

struct CameraComponent : public Component {
    glm::vec3 front = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 right = glm::vec3(1.f, 0.f, 0.f);

    float pitch = 0.f;
    float yaw = -90.f;

    float fov = 45.f;
    float aspectRatio = 4.f / 3.f;
    float nearPlane = .1f;
    float farPlane = 1000.f;
    
    glm::vec3 offset = glm::vec3(0.f);

    glm::mat4 viewMatrix = glm::mat4(1.f);
    glm::mat4 projectionMatrix = glm::mat4(1.f);

    bool isActive = false;
    float mouseSensitivity = 0.05f;

    CameraComponent() = default;
};

#endif