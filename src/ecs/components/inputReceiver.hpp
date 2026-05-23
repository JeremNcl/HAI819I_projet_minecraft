#pragma once
#ifndef INPUTRECEIVER_HPP
#define INPUTRECEIVER_HPP

#include "component.hpp"

struct InputReceiverComponent : public Component {

    // Input pressed
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;

    // Potentiellement Temporaire
    bool moveUp = false;
    bool moveDown = false;

    bool toggleFullscreen = false;

    double mouseX = 0.;
    double mouseY = 0.;

    float mouseSensitivity = .1f;
    float movementSpeed = 50.f;

    InputReceiverComponent() = default;
};

#endif