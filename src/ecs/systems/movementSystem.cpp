#include "movementSystem.hpp"

void MovementSystem::update(Registry& _registry, float _deltaTime) {

    Registry::View2<TransformComponent, VelocityComponent> view = _registry.view<TransformComponent, VelocityComponent>();

    for (EntityID entity : view) {

        TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);

        transform.position += velocity.velocity * _deltaTime;

    }
}