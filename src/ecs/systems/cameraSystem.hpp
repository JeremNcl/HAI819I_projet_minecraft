#pragma once
#ifndef CAMERASYSTEM_HPP
#define CAMERASYSTEM_HPP

#include "../registry.hpp"
#include "../components/transform.hpp"
#include "../components/camera.hpp"
#include "../components/inputReceiver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraSystem {
    public:
        //TODO : Ajoutez un constructeur pour init les ratios des camera
        void update(Registry& _registry, float _deltaTime);
};

#endif