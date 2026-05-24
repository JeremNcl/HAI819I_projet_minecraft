#include "playerMovementSystem.hpp"

void PlayerMovementSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View3<CameraComponent, InputReceiverComponent, VelocityComponent> view = _registry.view<CameraComponent, InputReceiverComponent, VelocityComponent>();

    for (EntityID entity : view) {

        InputReceiverComponent& input = _registry.getComponent<InputReceiverComponent>(entity);
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);
        CameraComponent& camera = _registry.getComponent<CameraComponent>(entity);

        glm::vec3 moveDir(0.0f);

        if (input.moveForward)  moveDir += camera.front;
        if (input.moveBackward) moveDir -= camera.front;
        if (input.moveLeft)     moveDir -= camera.right;
        if (input.moveRight)    moveDir += camera.right;

        moveDir.y = 0.0f;

        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
        }

        float currentSpeed = velocity.movementSpeed;
        if (input.sprint) currentSpeed *= velocity.sprintMultiplier;

        velocity.velocity.x = moveDir.x * currentSpeed;
        velocity.velocity.z = moveDir.z * currentSpeed;

        if (_registry.hasComponent<RigidBodyComponent>(entity)) {
            RigidBodyComponent& rigidBody = _registry.getComponent<RigidBodyComponent>(entity);
            
            if (input.jump && rigidBody.isGrounded) {
                velocity.velocity.y = velocity.jumpStrenght; 
            }
        }
    }
}