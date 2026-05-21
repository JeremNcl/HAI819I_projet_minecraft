#include "cameraSystem.hpp"

void CameraSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View2<CameraComponent, TransformComponent> view = _registry.view<CameraComponent, TransformComponent>();

    for (EntityID entity : view) {

        TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);
        CameraComponent& camera = _registry.getComponent<CameraComponent>(entity);

        if (_registry.hasComponent<InputReceiverComponent>(entity)) {
            InputReceiverComponent& input = _registry.getComponent<InputReceiverComponent>(entity);
            
            camera.yaw += input.mouseX * input.mouseSensitivity;
            camera.pitch += input.mouseY * input.mouseSensitivity;

            camera.pitch = glm::clamp(camera.pitch, -89.f, 89.f);

            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            camera.front = glm::normalize(front);

            glm::vec3 right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.up = glm::normalize(glm::cross(right, camera.front));

            float speed = input.movementSpeed * _deltaTime;
            if (input.moveForward) transform.position += camera.front * speed;
            if (input.moveBackward) transform.position -= camera.front * speed;
            if (input.moveLeft) transform.position -= right * speed;
            if (input.moveRight) transform.position += right * speed;
            if (input.moveUp) transform.position += camera.up * speed;
            if (input.moveDown) transform.position -= camera.up * speed;
        }

        camera.viewMatrix = glm::lookAt(
            transform.position,
            transform.position + camera.front,
            camera.up
        );
        camera.projectionMatrix = glm::perspective(
            glm::radians(camera.fov),
            camera.aspectRatio,
            camera.nearPlane,
            camera.farPlane
        );
    }
}