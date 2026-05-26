#include "playerMovementSystem.hpp"

void PlayerMovementSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View3<CameraComponent, InputReceiverComponent, VelocityComponent> view = _registry.view<CameraComponent, InputReceiverComponent, VelocityComponent>();

    for (EntityID entity : view) {

        InputReceiverComponent& input = _registry.getComponent<InputReceiverComponent>(entity);
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);
        CameraComponent& camera = _registry.getComponent<CameraComponent>(entity);

        if (camera.isActive) {
            glm::vec3 moveDir(0.0f);

            bool hasRigidBody = _registry.hasComponent<RigidBodyComponent>(entity);

            glm::vec3 forward = camera.front;

            if (hasRigidBody) {
                forward.y = 0.f;
                if (glm::length(forward) > 0.f) {
                    forward = glm::normalize(forward);
                }
            }

            if (input.moveForward)  moveDir += forward;
            if (input.moveBackward) moveDir -= forward;
            if (input.moveLeft)     moveDir -= camera.right;
            if (input.moveRight)    moveDir += camera.right;

            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir);
            }

            float currentSpeed = velocity.movementSpeed;
            if (input.sprint) currentSpeed *= velocity.sprintMultiplier;

            if (_registry.hasComponent<RigidBodyComponent>(entity)) {

                velocity.velocity.x = moveDir.x * currentSpeed;
                velocity.velocity.z = moveDir.z * currentSpeed;
                
                RigidBodyComponent& rigidBody = _registry.getComponent<RigidBodyComponent>(entity);
                    
                if (input.jump && rigidBody.isGrounded) {
                    velocity.velocity.y = velocity.jumpStrenght; 
                }
            } else {
                //Cette partie n'est pas propre, il faudrait passer par des components style PlayerControlledTag etc 
                //Pour y dédier des System.
                
                TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);

                velocity.velocity = moveDir * currentSpeed;
                transform.position += velocity.velocity * _deltaTime;
            }
        }
    }
}