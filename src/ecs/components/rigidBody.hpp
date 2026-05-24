#pragma once
#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include "component.hpp"

struct RigidBodyComponent : public Component {

    float mass = 1.f;
    float gravityMultiplier = 1.f; //definit l'acceleration en cas de chute libre
                                   //surtout dans le cas ou il faut la réduire
    bool isGrounded = false;

    RigidBodyComponent() = default;
};

#endif