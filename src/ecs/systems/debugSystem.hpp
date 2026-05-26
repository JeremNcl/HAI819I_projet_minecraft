#pragma once
#ifndef DEBUGSYSTEM_HPP
#define DEBUGSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/inputReceiver.hpp"
#include "ecs/components/inventory.hpp"

#include <glm/glm.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <array>
#include <iostream>

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

extern float aoStrength;
extern float dayTime;
extern float daySpeed;
extern bool dayPaused;
extern bool debugWireframe;

class DebugSystem {
    private:
        float statsUpdateTimer = 0.0f;
        static constexpr float STATS_UPDATE_INTERVAL = 0.5f;
        float lastDisplayedFps = 60.0f;
        float lastDisplayedDeltaTime = 0.016f;
        int frameCount = 0;
        float frameTimeAccum = 0.0f;

    public:
        void update(Registry& _registry, GLFWwindow* _window, float _deltaTime, const RenderDebugState& renderState);
        void renderLoadingScreen(GLFWwindow* _window, int _currentMeshesReady, int _totalExpectedMeshes);
        void renderInventoryUI(Registry& registry, EntityID playerID, GLFWwindow* window);
        bool isWireframe() const { return debugWireframe; }
};

#endif