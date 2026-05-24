#include "windowSystem.hpp"

bool WindowSystem::update(Registry& _registry, GLFWwindow* _window){

    Registry::View<InputReceiverComponent> view = _registry.view<InputReceiverComponent>();

    for (EntityID entity : view) {
        InputReceiverComponent& input = _registry.getComponent<InputReceiverComponent>(entity);

        if (input.toggleFullscreen) {
            
            if (!isFullscreen) {
                glfwGetWindowPos(_window, &windowedX, &windowedY);
                glfwGetWindowSize(_window, &windowedWidth, &windowedHeight);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);

                glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                isFullscreen = true;
            } 
            else {
                glfwSetWindowMonitor(_window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
                isFullscreen = false;
            }

            input.toggleFullscreen = false; 
            return true;
        }
    }

    static int lastWidth = 0;
    static int lastHeight = 0;

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    
    if (width != lastWidth || height != lastHeight) {
    float newAspectRatio = (height > 0) ? (float)width / (float)height : 1.f;
    auto cameraView = _registry.view<CameraComponent>();
    for (EntityID entity : cameraView) {
        _registry.getComponent<CameraComponent>(entity).aspectRatio = newAspectRatio;
    }
    lastWidth = width;
    lastHeight = height;
}

    return false;
}