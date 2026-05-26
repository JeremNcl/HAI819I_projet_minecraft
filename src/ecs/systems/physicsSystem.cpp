#include "physicsSystem.hpp"

void PhysicsSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View2<TransformComponent, VelocityComponent> view = _registry.view<TransformComponent, VelocityComponent>();

    for (EntityID entity : view) {
        
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);
        TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);

        if (_registry.hasComponent<RigidBodyComponent>(entity)) {
            RigidBodyComponent& rigidBody = _registry.getComponent<RigidBodyComponent>(entity);

            if (!rigidBody.isGrounded) {
                velocity.velocity.y += GRAVITY * rigidBody.gravityMultiplier * _deltaTime;

                if (velocity.velocity.y < MAX_FALL_SPEED) {
                    velocity.velocity.y = MAX_FALL_SPEED;
                }
            } else {
                if (velocity.velocity.y < 0.0f) {
                    velocity.velocity.y = 0.0f;
                }
            }
        }
    }
}