#pragma once
#ifndef WINDOWSYSTEM_HPP
#define WINDOWSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/inputReceiver.hpp"
#include "ecs/components/camera.hpp"

#include <GLFW/glfw3.h>

class WindowSystem {
    private:
        //Position de la fenêtre sur l'écran
        int windowedX = 100;
        int windowedY = 100;

        int windowedWidth = 1280;
        int windowedHeight = 720;
        bool isFullscreen = false;

    public:
        WindowSystem() = default;

        bool update(Registry& _registry, GLFWwindow* _window);
};

#endif