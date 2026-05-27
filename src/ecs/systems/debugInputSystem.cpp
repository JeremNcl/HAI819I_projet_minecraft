#include "debugInputSystem.hpp"
#include "ecs/components/timeComponent.hpp"
#include "ecs/components/lightingStateComponent.hpp"
#include "ecs/components/inputReceiver.hpp"
#include <cstdio>
#include <glm/glm.hpp>

extern float dayTime;
extern float daySpeed;
extern bool dayPaused;
extern bool usePbrShader;
extern bool debugTBN;
extern bool useNormalMap;
extern bool debugDiffuseOnly;
extern bool useBakedAO;
extern bool useHemisphericalAmbient;
extern bool useReducedAmbient;
extern bool debugWireframe;
extern float aoStrength;
extern bool useFrustumCulling;
extern bool useOcclusionCulling;
extern bool startFpsRecording;

bool DebugInputSystem::checkKeyHeld(GLFWwindow* window, int key) {
    int state = glfwGetKey(window, key);
    return (state == GLFW_PRESS || state == GLFW_REPEAT);
}

bool DebugInputSystem::keyPressedOnce(GLFWwindow* window, int key, bool& prev) {
    int state = glfwGetKey(window, key);
    bool curr = (state == GLFW_PRESS || state == GLFW_REPEAT);
    bool triggered = curr && !prev;
    if (triggered) {
        fprintf(stderr, "[DEBUG] Key pressed once: key=%d\n", key);
    }
    prev = curr;
    return triggered;
}

bool DebugInputSystem::checkArrowWithDebounce(GLFWwindow* window, int key, bool& prevState, 
                                              float& repeatTimer, float deltaTime) {
    bool isHeld = checkKeyHeld(window, key);
    bool wasHeld = prevState;
    prevState = isHeld;
    
    if (isHeld && !wasHeld) {
        repeatTimer = 0.0f;
        return true;
    }
    
    if (isHeld) {
        repeatTimer += deltaTime;
        if (repeatTimer >= REPEAT_DELAY) {
            repeatTimer -= REPEAT_INTERVAL;
            return true;
        }
    }
    
    return false;
}

void DebugInputSystem::update(Registry& registry, GLFWwindow* window, float deltaTime) {
    if (!window) return;

    auto timeView = registry.view<TimeComponent>();
    TimeComponent* timeComp = nullptr;
    if (!timeView.isEmpty()) {
        EntityID timeEntity = *timeView.begin();
        timeComp = &registry.getComponent<TimeComponent>(timeEntity);
    }

    auto lightingView = registry.view<LightingStateComponent>();
    LightingStateComponent* lightingComp = nullptr;
    if (!lightingView.isEmpty()) {
        EntityID lightingEntity = *lightingView.begin();
        lightingComp = &registry.getComponent<LightingStateComponent>(lightingEntity);
    }

    // ===== CAMERA SWAP INPUT (F1) =====
    bool toggleCameraSwap = keyPressedOnce(window, GLFW_KEY_F1, prev_F1);
    auto receiverView = registry.view<InputReceiverComponent>();
    for (EntityID entity : receiverView) {
        auto& input = registry.getComponent<InputReceiverComponent>(entity);
        input.toggleCameraSwap = toggleCameraSwap;
    }

    if (keyPressedOnce(window, GLFW_KEY_G, prev_G)) {
        startFpsRecording = true; 
    }

    // FRUSTRUM TOGGLE 
    if (keyPressedOnce(window, GLFW_KEY_F2, prev_F2)) {
        useFrustumCulling = !useFrustumCulling;
        printf("Frustum Culling: %s\n", useFrustumCulling ? "ON" : "OFF");
    }

    // Occlusion TOGGLE
    if (keyPressedOnce(window, GLFW_KEY_Y, prev_Y)) {
        useOcclusionCulling = !useOcclusionCulling;
        printf("Occlusion Culling (Face-to-Face): %s\n", useOcclusionCulling ? "ON" : "OFF");
    }

    // ===== WIREFRAME TOGGLE (F3) =====
    if (keyPressedOnce(window, GLFW_KEY_F3, prev_F3)) {
        debugWireframe = !debugWireframe;
        printf("Wireframe mode: %s\n", debugWireframe ? "ON" : "OFF");
    }

    // ===== DAY/NIGHT CYCLE INPUT =====
    if (checkArrowWithDebounce(window, GLFW_KEY_UP, prev_UP, arrow_repeat_timer_UP, deltaTime)) {
        if (timeComp) {
            timeComp->daySpeed = glm::clamp(timeComp->daySpeed + INCREMENT_STEP, 0.0f, 1.0f);
            daySpeed = timeComp->daySpeed;
            printf("Day speed: %.3f\n", timeComp->daySpeed);
        }
    }

    if (checkArrowWithDebounce(window, GLFW_KEY_DOWN, prev_DOWN, arrow_repeat_timer_DOWN, deltaTime)) {
        if (timeComp) {
            timeComp->daySpeed = glm::clamp(timeComp->daySpeed - INCREMENT_STEP, 0.0f, 1.0f);
            daySpeed = timeComp->daySpeed;
            printf("Day speed: %.3f\n", timeComp->daySpeed);
        }
    }

    if (timeComp && timeComp->paused && checkArrowWithDebounce(window, GLFW_KEY_LEFT, prev_LEFT, arrow_repeat_timer_LEFT, deltaTime)) {
        timeComp->dayTime -= INCREMENT_STEP;
        if (timeComp->dayTime < 0.0f) timeComp->dayTime = 0.0f;
        dayTime = timeComp->dayTime;
        printf("Day time: %.3f\n", timeComp->dayTime);
    }

    if (timeComp && timeComp->paused && checkArrowWithDebounce(window, GLFW_KEY_RIGHT, prev_RIGHT, arrow_repeat_timer_RIGHT, deltaTime)) {
        timeComp->dayTime += INCREMENT_STEP;
        if (timeComp->dayTime > 1.0f) timeComp->dayTime = 1.0f;
        dayTime = timeComp->dayTime;
        printf("Day time: %.3f\n", timeComp->dayTime);
    }

    if (keyPressedOnce(window, GLFW_KEY_M, prev_M)) {
        if (timeComp) {
            timeComp->paused = !timeComp->paused;
            dayPaused = timeComp->paused;
            printf("Day cycle paused: %s\n", timeComp->paused ? "YES" : "NO");
        }
    }

    // ===== RENDER DEBUG TOGGLES (F-keys) =====
    if (keyPressedOnce(window, GLFW_KEY_F10, prev_F10)) {
        usePbrShader = !usePbrShader;
        printf("Mode rendu: %s\n", usePbrShader ? "PBR" : "BASIC");
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_F9, prev_F9)) {
        lightingComp->debugTBN = !lightingComp->debugTBN;
        debugTBN = lightingComp->debugTBN;
        printf("Debug TBN: %s\n", lightingComp->debugTBN ? "ON" : "OFF");
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_F8, prev_F8)) {
        lightingComp->useNormalMap = !lightingComp->useNormalMap;
        useNormalMap = lightingComp->useNormalMap;
        printf("Normal map: %s\n", lightingComp->useNormalMap ? "ON" : "OFF");
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_F7, prev_F7)) {
        lightingComp->debugDiffuseOnly = !lightingComp->debugDiffuseOnly;
        debugDiffuseOnly = lightingComp->debugDiffuseOnly;
        printf("Diffuse only: %s\n", lightingComp->debugDiffuseOnly ? "ON" : "OFF");
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_F4, prev_F4)) {
        lightingComp->useBakedAO = !lightingComp->useBakedAO;
        useBakedAO = lightingComp->useBakedAO;
        printf("Baked AO: %s\n", lightingComp->useBakedAO ? "ON" : "OFF");
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_F5, prev_F5)) {
        lightingComp->useHemisphericalAmbient = !lightingComp->useHemisphericalAmbient;
        useHemisphericalAmbient = lightingComp->useHemisphericalAmbient;
        printf("Hemispherical ambient: %s\n", lightingComp->useHemisphericalAmbient ? "ON" : "OFF");
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_F6, prev_F6)) {
        lightingComp->useReducedAmbient = !lightingComp->useReducedAmbient;
        useReducedAmbient = lightingComp->useReducedAmbient;
        printf("Reduced ambient: %s\n", lightingComp->useReducedAmbient ? "ON" : "OFF");
    }

    // ===== AO STRENGTH ADJUSTMENTS =====
    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_O, prev_O)) {
        lightingComp->aoStrength = glm::clamp(lightingComp->aoStrength - 0.05f, 0.0f, 1.0f);
        aoStrength = lightingComp->aoStrength;
        printf("AO strength: %.2f\n", lightingComp->aoStrength);
    }

    if (usePbrShader && lightingComp && keyPressedOnce(window, GLFW_KEY_P, prev_P)) {
        lightingComp->aoStrength = glm::clamp(lightingComp->aoStrength + 0.05f, 0.0f, 1.0f);
        aoStrength = lightingComp->aoStrength;
        printf("AO strength: %.2f\n", lightingComp->aoStrength);
    }
}