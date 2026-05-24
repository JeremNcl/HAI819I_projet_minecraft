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
    bool jump = false;
    bool sprint = false;

    bool toggleFullscreen = false;

    double mouseX = 0.;
    double mouseY = 0.;
    
    InputReceiverComponent() = default;
};

#endif