#include "cameraSystem.hpp"

void CameraSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View2<CameraComponent, TransformComponent> view = _registry.view<CameraComponent, TransformComponent>();

    for (EntityID entity : view) {

        TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);
        CameraComponent& camera = _registry.getComponent<CameraComponent>(entity);

        if (_registry.hasComponent<InputReceiverComponent>(entity)) {
            InputReceiverComponent& input = _registry.getComponent<InputReceiverComponent>(entity);
            
            camera.yaw += input.mouseX * camera.mouseSensitivity;
            camera.pitch += input.mouseY * camera.mouseSensitivity;

            camera.pitch = glm::clamp(camera.pitch, -89.f, 89.f);

            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            camera.front = glm::normalize(front);

            camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.up = glm::normalize(glm::cross(camera.right, camera.front));
        }

        glm::vec3 cameraPos = transform.position + camera.offset;

        camera.viewMatrix = glm::lookAt(
            cameraPos,
            cameraPos + camera.front,
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

void CameraSystem::initCamera(Registry& _registry, EntityID _entity, float _yaw, float _pitch, glm::vec3 _offset) {
    if (!_registry.hasComponent<CameraComponent>(_entity)) return;

    CameraComponent& camera = _registry.getComponent<CameraComponent>(_entity);

    camera.offset = _offset;
    camera.yaw = _yaw;
    camera.pitch = _pitch;

    float yawR = glm::radians(_yaw);
    float pitchR = glm::radians(_pitch);

    camera.front = glm::normalize(glm::vec3(
        cos(yawR) * cos(pitchR),
        sin(pitchR),
        sin(yawR) * cos(pitchR)
    ));

    camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}