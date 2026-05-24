#pragma once
#ifndef COLLISIONSYSTEM_HPP
#define COLLISIONSYSTEM_HPP

#include "../registry.hpp"
#include "../components/transform.hpp"
#include "../components/velocity.hpp"
#include "../components/collider.hpp"
#include "../components/rigidBody.hpp"
#include "../components/chunk.hpp"
#include "../components/world.hpp"

#include <cmath>
#include <algorithm>

class CollisionSystem {

    private:
        static constexpr float EPSILON_COLLISION = .001f;
        static constexpr float EPSILON_POSITION = .01f;
        static constexpr float FALL_Y_EPSILON = .05f;

    public:
        void update(Registry& _registry, const WorldMapComponent& _worldMap, float _deltaTime);

    private:
        bool checkAABBCollision(Registry& _registry, const glm::vec3& _pos, const ColliderComponent& _collider, const WorldMapComponent& _worldMap) const;
};

#endif