#pragma once
#ifndef DELTATIME_HPP
#define DELTATIME_HPP

#include "component.hpp"

struct DeltaTimeComponent : public Component {
    float deltaTime = 0.f;
    float rawDeltaTime = 0.f; // Delta Time pas capé (pour les choses qui en on besoin)
    float lastFrame = 0.f;
    float maxDeltaTime = .05f;
    float timeScale = 1.f; // HEHE LES RALENTIS (on l'utilisera jamais)
    
    DeltaTimeComponent() = default;
};

#endif