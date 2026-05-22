#pragma once
#ifndef PHYSICSSYSTEM_HPP
#define PHYSICSSYSTEM_HPP

#include "../registry.hpp"
#include "../components/velocity.hpp"
#include "../components/rigidBody.hpp"

class PhysicsSystem {
    private:
        const float GRAVITY = -9.8f;
        
        //TODO : potentiellement la changer pour adapter selon la masse
        // la vitesse max de chute libre
        const float MAX_FALL_SPEED = -20.f; //Evite d'accelerer à l'infini

    public: 
        void update(Registry& _registry, float _deltaTime);
};

#endif