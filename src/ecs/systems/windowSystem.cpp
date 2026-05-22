#include "windowSystem.hpp"
#include "../../ecs/components/inputReceiver.hpp"
#include "../../ecs/components/camera.hpp"

void WindowSystem::update(Registry& _registry, GLFWwindow* _window){

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
        }
    }

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    
    float newAspectRatio = (height > 0) ? (float)width / (float)height : 1.f;

    Registry::View<CameraComponent> cameraView = _registry.view<CameraComponent>();
    for (EntityID entity : cameraView) {
        CameraComponent& camera = _registry.getComponent<CameraComponent>(entity);

        if (camera.aspectRatio != newAspectRatio) {
            camera.aspectRatio = newAspectRatio;
        }
    }
}