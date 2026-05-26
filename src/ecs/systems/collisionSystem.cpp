#include "collisionSystem.hpp"
#include <iostream>

VoxelType CollisionSystem::getVoxelAt(int x, int y, int z) const {
    int chunkX = std::floor(static_cast<float>(x) / 16.0f);
    int chunkY = std::floor(static_cast<float>(y) / 16.0f);
    int chunkZ = std::floor(static_cast<float>(z) / 16.0f);

    if (chunkY < 0 || chunkY >= 16) return VoxelType::AIR;

    glm::ivec3 targetPos(chunkX, chunkY, chunkZ);

    auto it = subChunkMap.find(targetPos);
    if (it != subChunkMap.end()) {

        int localX = (x % 16 + 16) % 16;
        int localY = (y % 16 + 16) % 16;
        int localZ = (z % 16 + 16) % 16;
        return it->second->getVoxel(localX, localY, localZ);
    }

    return VoxelType::AIR;
}

bool CollisionSystem::isVoxelSolid(VoxelType type) const {
    return type != VoxelType::AIR && type != VoxelType::WATER && type != VoxelType::LAVA;
}

bool CollisionSystem::checkAABBCollision(Registry& _registry, const glm::vec3& _pos,
    const ColliderComponent& _collider) const {

    AABB box = _collider.getAABB(_pos);

    box.min += EPSILON_COLLISION;
    box.max -= EPSILON_COLLISION;

    // BROAD PHASE :
    // On utilise la grille spatial à la place d'un tree
    // C'est une solution spécifique à notre voxel engine qui permet de
    // ce passer d'un arbre car tout est un voxel que l'on peut directement isoler.
    int minX = std::floor(box.min.x);
    int maxX = std::floor(box.max.x);
    int minY = std::floor(box.min.y);
    int maxY = std::floor(box.max.y);
    int minZ = std::floor(box.min.z);
    int maxZ = std::floor(box.max.z);

    // NARROW PHASE :
    // Vérification de contact réel. Même principe, tout étant en AABB,
    // pas besoin de SAT 15 axes, uniquement XYZ
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (isVoxelSolid(getVoxelAt(x, y, z))) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CollisionSystem::resolveAxisCollision(Registry& _registry, glm::vec3& _pos, float& _axisVelocity, const ColliderComponent& _collider, int _axisIndex) const {
    if (_axisVelocity == 0.0f) return;

    if (checkAABBCollision(_registry, _pos, _collider)) {
        AABB box = _collider.getAABB(_pos);
        
        if (_axisVelocity > 0.0f) {
            _pos[_axisIndex] = std::floor(box.max[_axisIndex]) - (_collider.offset[_axisIndex] + _collider.size[_axisIndex] * 0.5f) - EPSILON_POSITION;
        } else {
            _pos[_axisIndex] = std::floor(box.min[_axisIndex]) + 1.0f - (_collider.offset[_axisIndex] - _collider.size[_axisIndex] * 0.5f) + EPSILON_POSITION;
        }
        _axisVelocity = 0.0f;
    }
}

void CollisionSystem::update(Registry& _registry, float _deltaTime) {

    subChunkMap.clear();
    auto subChunkView = _registry.view<SubChunkComponent>();
    for (EntityID entity : subChunkView) {
        const auto& subChunk = _registry.getComponent<SubChunkComponent>(entity);
        subChunkMap[subChunk.subChunkPosition] = &subChunk;
    }

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
        resolveAxisCollision(_registry, pos, velocity.velocity.y, collider, 1);

        // AXE X
        pos.x += velocity.velocity.x * _deltaTime;
        resolveAxisCollision(_registry, pos, velocity.velocity.x, collider, 0);

        // AXE Z
        pos.z += velocity.velocity.z * _deltaTime;
        resolveAxisCollision(_registry, pos, velocity.velocity.z, collider, 2);

        transform.position = pos;

        // MAJ grounded
        if (rigidBody) {
            if (velocity.velocity.y > 0.001f) {
                rigidBody->isGrounded = false;
            } else {
                glm::vec3 groundCheck = pos;
                groundCheck.y -= FALL_Y_EPSILON;
                rigidBody->isGrounded = checkAABBCollision(_registry, groundCheck, collider);
            }
        }
    }
}