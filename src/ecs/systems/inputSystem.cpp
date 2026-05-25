#include "inputSystem.hpp"

InputSystem::InputSystem(GLFWwindow* _window){
    int width, height;
    glfwGetWindowSize(_window, &width, &height);
    lastX = width * .5f;
    lastY = height * .5f;
}

void InputSystem::update(Registry& _registry, GLFWwindow* _window) {

    Registry::View<InputReceiverComponent> view = _registry.view<InputReceiverComponent>();

    double mouseX, mouseY;
    glfwGetCursorPos(_window, &mouseX, &mouseY);

    if (firstMouse) {
        firstMouse = false;
        lastX = mouseX;
        lastY = mouseY;
    }

    double deltaX = mouseX - lastX;
    double deltaY = lastY - mouseY;

    lastX = mouseX;
    lastY = mouseY;

    bool f11Pressed = (glfwGetKey(_window, GLFW_KEY_F11) == GLFW_PRESS);

    for (EntityID entity : view) {
        
        InputReceiverComponent& input = _registry.getComponent<InputReceiverComponent>(entity);
        
        input.moveForward = (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS);
        input.moveBackward = (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS);
        input.moveLeft = (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS);
        input.moveRight = (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS);
        input.jump = (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS);
        input.sprint = (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);

        if (f11Pressed && !f11PressedLastFrame) {
            input.toggleFullscreen = true;
        } else {
            input.toggleFullscreen = false;
        }

        input.mouseX = deltaX;
        input.mouseY = deltaY;
    }

    f11PressedLastFrame = f11Pressed;
}

void InputSystem::setCursorMode(GLFWwindow* window, bool _locked) {
    cursorLocked = _locked;
    if (_locked) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void InputSystem::resetMouseTracking(GLFWwindow* window) {
    firstMouse = true;

    int width = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);

    if (width > 0 && height > 0) {
        glfwSetCursorPos(window, width * 0.5, height * 0.5);
    }
}