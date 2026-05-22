#include "physicsSystem.hpp"

void PhysicsSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View2<VelocityComponent, RigidBodyComponent> view = _registry.view<VelocityComponent, RigidBodyComponent>();

    for (EntityID entity : view) {
        
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);
        RigidBodyComponent& rigidBody = _registry.getComponent<RigidBodyComponent>(entity);
        
        //Test pour la chute
        if (!rigidBody.isGrounded) {
            velocity.velocity.y += GRAVITY * rigidBody.gravityMultiplier * _deltaTime;

            if (velocity.velocity.y < MAX_FALL_SPEED){
                velocity.velocity.y = MAX_FALL_SPEED;
            }
        }
    }
}