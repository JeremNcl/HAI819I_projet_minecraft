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
    bool toggleWireframe = false;
    bool togglePbr = false;
    bool toggleTBN = false;
    bool toggleNormalMap = false;
    bool toggleDiffuse = false;
    bool toggleAmbient = false;
    bool toggleFrustumCulling = true;
    bool toggleOcclusionCulling = true;

    bool toggleCameraSwap = false;
    bool toggleMonsterSpawn = false;

    double mouseX = 0.;
    double mouseY = 0.;
    
    bool leftClick = false;
    bool rightClick = false;

    InputReceiverComponent() = default;
};

#endif