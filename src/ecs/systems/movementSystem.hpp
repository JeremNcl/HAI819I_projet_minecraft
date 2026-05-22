#pragma once
#ifndef MOVEMENTSYSTEM_HPP
#define MOVEMENTSYSTEM_HPP

#include "../registry.hpp"
#include "../components/transform.hpp"
#include "../components/velocity.hpp"

class MovementSystem {
    public: 
        void update(Registry& _registry, float _deltaTime);
};

#endif