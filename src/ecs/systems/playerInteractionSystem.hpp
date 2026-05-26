#pragma once
#ifndef PLAYERINTERACTIONSYSTEM_HPP
#define PLAYERINTERACTIONSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/systems/TerrainSystem.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/inputReceiver.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/player.hpp"
#include "ecs/components/collider.hpp"

#include <glm/glm.hpp>
#include <limits>
#include <cmath>

struct RaycastResult {
    bool hit = false;
    glm::ivec3 hitVoxelPos{0, 0, 0};
    glm::ivec3 normal{0, 0, 0};
    EntityID chunkEntity = 0;
};

class PlayerInteractionSystem {
private:
    RaycastResult raycast(Registry& _registry, TerrainSystem& _terrain, const glm::vec3& _start, const glm::vec3& _direction, float _reach) const;
    bool checkAABBIntersection(const AABB& _a, const AABB& _b);
public:
    void update(Registry& _registry, TerrainSystem& _terrain);
};

#endif