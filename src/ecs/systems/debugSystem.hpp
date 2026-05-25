#pragma once
#ifndef DEBUGSYSTEM_HPP
#define DEBUGSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/transform.hpp"

#include <glm/glm.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>

struct RenderDebugState {
    bool usePbrShader = true;
    bool debugTBN = false;
    bool useNormalMap = true;
    bool debugDiffuseOnly = false;
    bool useBakedAO = true;
    bool useHemisphericalAmbient = true;
    bool useReducedAmbient = false;
    float ambientStrength = 0.0f;
    float aoStrength = 0.0f;
    float exposure = 1.0f;
    glm::vec3 lightColor{0.0f};
    glm::vec3 ambientSkyColor{0.0f};
    glm::vec3 ambientGroundColor{0.0f};
    glm::vec3 horizonColor{0.0f};
    float dayTime = 0.0f;
    float daySpeed = 0.0f;
    bool dayPaused = false;
};

// Global AO strength controlled by UI
extern float aoStrength;
// Globals controlled by UI/keys
extern float dayTime;
extern float daySpeed;
extern bool dayPaused;

class DebugSystem {
    private:
        // Stats averaging
        float statsUpdateTimer = 0.0f;
        static constexpr float STATS_UPDATE_INTERVAL = 0.5f;  // Update stats every 0.5 seconds
        float lastDisplayedFps = 60.0f;
        float lastDisplayedDeltaTime = 0.016f;
        int frameCount = 0;
        float frameTimeAccum = 0.0f;

    public:
        void update(Registry& _registry, GLFWwindow* _window, float _deltaTime, const RenderDebugState& renderState);
};

#endif