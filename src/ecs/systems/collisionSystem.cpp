#include "collisionSystem.hpp"
#include <iostream>

bool CollisionSystem::checkAABBCollision(Registry& _registry, const glm::vec3& _pos,
    const ColliderComponent& _collider, const WorldMapComponent& _worldMap) const {

    glm::vec3 minBox = _pos + _collider.offset - (_collider.size * .5f);
    glm::vec3 maxBox = _pos + _collider.offset + (_collider.size * .5f);

    minBox += EPSILON_COLLISION;
    maxBox -= EPSILON_COLLISION;

    // BROAD PHASE :
    // On utilise la grille spatial à la place d'un tree
    // C'est une solution spécifique à notre voxel engine qui permet de
    // ce passer d'un arbre car tout est un voxel que l'on peut directement isoler.
    int minX = std::floor(minBox.x);
    int maxX = std::floor(maxBox.x);
    int minY = std::floor(minBox.y);
    int maxY = std::floor(maxBox.y);
    int minZ = std::floor(minBox.z);
    int maxZ = std::floor(maxBox.z);

    // NARROW PHASE :
    // Vérification de contact réel. Même principe, tout étant en AABB,
    // pas besoin de SAT 15 axes, uniquement XYZ
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (_worldMap.isVoxelSolid(_registry, x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CollisionSystem::update(Registry& _registry, const WorldMapComponent& _worldMap, float _deltaTime) {

    Registry::View3<VelocityComponent, ColliderComponent, TransformComponent> view = _registry.view<VelocityComponent, ColliderComponent, TransformComponent>();

    for (EntityID entity : view) {
        VelocityComponent& velocity = _registry.getComponent<VelocityComponent>(entity);
        ColliderComponent& collider = _registry.getComponent<ColliderComponent>(entity);
        TransformComponent& transform = _registry.getComponent<TransformComponent>(entity);

        bool hasRB = _registry.hasComponent<RigidBodyComponent>(entity);
        RigidBodyComponent* rigidBody = hasRB ? &_registry.getComponent<RigidBodyComponent>(entity) : nullptr;

        glm::vec3 pos = transform.position;

        // AXE Y
        pos.y += velocity.velocity.y * _deltaTime;
        if (checkAABBCollision(_registry, pos, collider, _worldMap)) {
            if (velocity.velocity.y > 0.0f) {
                float maxY = pos.y + collider.offset.y + collider.size.y * 0.5f;
                pos.y = std::floor(maxY) - (collider.offset.y + collider.size.y * 0.5f) - EPSILON_POSITION;
            } else if (velocity.velocity.y < 0.0f) {
                float minY = pos.y + collider.offset.y - collider.size.y * 0.5f;
                pos.y = std::floor(minY) + 1.0f - (collider.offset.y - collider.size.y * 0.5f) + EPSILON_POSITION;
            }
            velocity.velocity.y = 0.0f;
        }

        // AXE X
        pos.x += velocity.velocity.x * _deltaTime;
        if (checkAABBCollision(_registry, pos, collider, _worldMap)) {
            if (velocity.velocity.x > 0.0f) {
                float maxX = pos.x + collider.offset.x + collider.size.x * 0.5f;
                pos.x = std::floor(maxX) - (collider.offset.x + collider.size.x * 0.5f) - EPSILON_POSITION;
            } else if (velocity.velocity.x < 0.0f) {
                float minX = pos.x + collider.offset.x - collider.size.x * 0.5f;
                pos.x = std::floor(minX) + 1.0f - (collider.offset.x - collider.size.x * 0.5f) + EPSILON_POSITION;
            }
            velocity.velocity.x = 0.0f;
        }

        // AXE Z
        pos.z += velocity.velocity.z * _deltaTime;
        if (checkAABBCollision(_registry, pos, collider, _worldMap)) {
            if (velocity.velocity.z > 0.0f) {
                float maxZ = pos.z + collider.offset.z + collider.size.z * 0.5f;
                pos.z = std::floor(maxZ) - (collider.offset.z + collider.size.z * 0.5f) - EPSILON_POSITION;
            } else if (velocity.velocity.z < 0.0f) {
                float minZ = pos.z + collider.offset.z - collider.size.z * 0.5f;
                pos.z = std::floor(minZ) + 1.0f - (collider.offset.z - collider.size.z * 0.5f) + EPSILON_POSITION;
            }
            velocity.velocity.z = 0.0f;
        }

        transform.position = pos;

        // MAJ grounded
        if (rigidBody) {
            if (velocity.velocity.y > 0.001f) {
                rigidBody->isGrounded = false;
            } else {
                glm::vec3 groundCheck = pos;
                groundCheck.y -= FALL_Y_EPSILON;
                rigidBody->isGrounded = checkAABBCollision(_registry, groundCheck, collider, _worldMap);
            }
        }
    }
}