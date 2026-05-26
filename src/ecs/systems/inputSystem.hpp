#pragma once
#ifndef INPUTSYSTEM_HPP
#define INPUTSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/inputReceiver.hpp"
#include <GLFW/glfw3.h>
#include <unordered_map>

class InputSystem {
    private:
        double lastX;
        double lastY;
        bool firstMouse = true;
        bool cursorLocked = true; // TODO : Verifier que c'est vraiment utile
        bool f11PressedLastFrame = false;

        std::unordered_map<int, bool> m_previousKeyState;
        std::unordered_map<int, bool> m_previousMouseState;
        
        bool keyPressedOnce(GLFWwindow* _window, int _key);
        bool mousePressedOnce(GLFWwindow* _window, int _button);

    public:
        InputSystem(GLFWwindow* _window);

        void update(Registry& _registry, GLFWwindow* _windows);

        void setCursorMode(GLFWwindow* _windows, bool _locked);
        void resetMouseTracking(GLFWwindow* window);
};

#endif