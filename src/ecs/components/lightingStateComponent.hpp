#pragma once
#ifndef LIGHTING_STATE_COMPONENT_HPP
#define LIGHTING_STATE_COMPONENT_HPP

#include "component.hpp"
#include <glm/glm.hpp>

struct LightingStateComponent : public Component {
    // Direct light from sun
    glm::vec3 lightColor = glm::vec3(1.0f);
    glm::vec3 lightDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Ambient lighting
    float ambientStrength = 0.055f;
    glm::vec3 ambientSkyColor = glm::vec3(0.25f, 0.45f, 0.85f);
    glm::vec3 ambientGroundColor = glm::vec3(0.12f, 0.10f, 0.08f);
    glm::vec3 horizonColor = glm::vec3(0.65f, 0.75f, 0.85f);
    
    // Tone mapping
    float exposure = 1.0f;
    
    // Flags controlling rendering behavior
    bool useHemisphericalAmbient = true;
    bool useBakedAO = true;
    bool useReducedAmbient = false;
    float aoStrength = 0.50f;
    
    // Debug toggles
    bool debugTBN = false;
    bool useNormalMap = true;
    bool debugDiffuseOnly = false;
    
    LightingStateComponent() = default;
};

#endif
