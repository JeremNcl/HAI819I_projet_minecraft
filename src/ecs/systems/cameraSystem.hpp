#pragma once
#ifndef CAMERASYSTEM_HPP
#define CAMERASYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/inputReceiver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraSystem {
    public:
        void update(Registry& _registry, float _deltaTime);

        void initCamera(Registry& _registry, EntityID _entity, float _yaw, float _pitch, glm::vec3 _offset);
};

#endif