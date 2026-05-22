#pragma once
#ifndef DEBUGSYSTEM_HPP
#define DEBUGSYSTEM_HPP

#include "../registry.hpp"
#include "../components/camera.hpp"
#include "../components/transform.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

class DebugSystem {
    public:
        void update(Registry& _registry, GLFWwindow* _window, float _deltaTime);
};

#endif