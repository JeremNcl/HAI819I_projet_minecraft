#pragma once
#ifndef MOVEMENTSYSTEM_HPP
#define MOVEMENTSYSTEM_HPP

#include "../registry.hpp"
#include "../components/transform.hpp"
#include "../components/camera.hpp"
#include "../components/velocity.hpp"
#include "../components/inputReceiver.hpp"
#include "../components/rigidBody.hpp"

class PlayerMovementSystem {
    public: 
        void update(Registry& _registry, float _deltaTime);
};

#endif