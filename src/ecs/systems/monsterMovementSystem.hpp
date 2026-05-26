#pragma once
#ifndef MONSTERMOVEMENTSYSTEM_HPP
#define MONSTERMOVEMENTSYSTEM_HPP

#include "../registry.hpp"
#include "../components/transform.hpp"
#include "../components/velocity.hpp"
#include "../components/monster.hpp"
#include "../components/ia.hpp"
#include "PathFindingSystem.hpp"

class MonsterMovementSystem {
    public:
        void update(Registry& _registry, float _deltaTime);
};

#endif