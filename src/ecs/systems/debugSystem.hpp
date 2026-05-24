#pragma once
#ifndef DEBUGSYSTEM_HPP
#define DEBUGSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/transform.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

class DebugSystem {
    public:
        void update(Registry& _registry, GLFWwindow* _window, float _deltaTime, float _rawDeltaTime);
};

#endif