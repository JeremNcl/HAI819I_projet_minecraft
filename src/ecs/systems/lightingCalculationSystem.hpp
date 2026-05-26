#pragma once

#include "../../ecs/registry.hpp"
#include "../components/timeComponent.hpp"
#include "../components/lightingStateComponent.hpp"
#include <glm/glm.hpp>

class LightingCalculationSystem {
public:
    void update(Registry& registry);
    
private:
    // Helper function for linear interpolation with clamping
    static float inverseLerp(float a, float b, float t);
    
    // Helper to get sun position angle from day time
    static float getSunAngle(float t);
    
    // Lighting calculation functions (migrated from main.cpp)
    static glm::vec3 computeLightColorFromTime(float t, bool useReducedAmbient);
    static glm::vec3 computeLightDirectionFromTime(float t);
    static float computeAmbientStrengthFromTime(float t, bool useReducedAmbient);
    static glm::vec3 computeAmbientSkyColorFromTime(float t, bool useReducedAmbient);
    static glm::vec3 computeHorizonColorFromTime(float t, bool useReducedAmbient);
    static glm::vec3 computeAmbientGroundColorFromTime(float t, bool useReducedAmbient);
    static float computeExposureFromTime(float t);
};
