#pragma once
#ifndef COLLISIONSYSTEM_HPP
#define COLLISIONSYSTEM_HPP

#include "../registry.hpp"
#include "../components/transform.hpp"
#include "../components/velocity.hpp"
#include "../components/collider.hpp"
#include "../components/rigidBody.hpp"
#include "../components/chunk.hpp"

#include <cmath>
#include <algorithm>

#include "hashUtils.hpp"

class CollisionSystem {

    private:
        static constexpr float EPSILON_COLLISION = .001f;
        static constexpr float EPSILON_POSITION = .01f;
        static constexpr float FALL_Y_EPSILON = .05f;

        std::unordered_map<glm::ivec3, const SubChunkComponent*, GLMVec3Hash> subChunkMap;

    public:
        void update(Registry& _registry, float _deltaTime);

    private:
        bool checkAABBCollision(Registry& _registry, const glm::vec3& _pos, const ColliderComponent& _collider) const;
        VoxelType getVoxelAt(int x, int y, int z) const;
        bool isVoxelSolid(VoxelType type) const;
};

#endif