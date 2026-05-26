#include "monsterMovementSystem.hpp"
#include "../components/rigidBody.hpp"
#include <glm/gtc/constants.hpp>

void MonsterMovementSystem::update(Registry& _registry, float _deltaTime) {
    Registry::View3<MonsterComponent, TransformComponent, VelocityComponent> view = 
        _registry.view<MonsterComponent, TransformComponent, VelocityComponent>();

    for (EntityID entity : view) {
        if (!_registry.hasComponent<IAComponent>(entity)) continue;

        IAComponent& ai = _registry.getComponent<IAComponent>(entity);
        TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);

        if (ai.currentPath.empty()) {
            velocity.velocity.x = 0.0f;
            velocity.velocity.z = 0.0f;
            continue;
        }

        glm::ivec3 nextNode = ai.currentPath.front();

        glm::vec3 targetPos = glm::vec3(nextNode) + glm::vec3(0.5f, 0.0f, 0.5f);

        glm::vec3 currentPos = transform.position;
        float distToTargetXZ = glm::distance(glm::vec2(currentPos.x, currentPos.z), glm::vec2(targetPos.x, targetPos.z));

        if (distToTargetXZ < 0.25f) {
            ai.currentPath.erase(ai.currentPath.begin());
            if (ai.currentPath.empty()) {
                velocity.velocity.x = 0.0f;
                velocity.velocity.z = 0.0f;
                continue;
            }
            nextNode = ai.currentPath.front();
            targetPos = glm::vec3(nextNode) + glm::vec3(0.5f, 0.0f, 0.5f);
        }

        glm::vec3 moveDir = targetPos - currentPos;
        moveDir.y = 0.0f;

        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
        }

        velocity.velocity.x = moveDir.x * velocity.movementSpeed;
        velocity.velocity.z = moveDir.z * velocity.movementSpeed;

        if (_registry.hasComponent<RigidBodyComponent>(entity)) {
            RigidBodyComponent& rb = _registry.getComponent<RigidBodyComponent>(entity);
            
            if (targetPos.y > currentPos.y + 0.5f && rb.isGrounded) {
                velocity.velocity.y = velocity.jumpStrenght;
            }
        }
    }
}