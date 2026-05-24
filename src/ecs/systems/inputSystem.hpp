#pragma once
#ifndef INPUTSYSTEM_HPP
#define INPUTSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/inputReceiver.hpp"
#include <GLFW/glfw3.h>

class InputSystem {
    private:
        double lastX;
        double lastY;
        bool firstMouse = true;
        bool cursorLocked = true; // TODO : Verifier que c'est vraiment utile

    public:
        InputSystem(GLFWwindow* _window);

        void update(Registry& _registry, GLFWwindow* _windows);

        void setCursorMode(GLFWwindow* _windows, bool _locked);
        void resetMouseTracking(GLFWwindow* window);
};

#endif