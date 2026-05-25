#include "debugInputSystem.hpp"
#include <cstdio>
#include <glm/glm.hpp>

// External globals (defined in main.cpp)
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
extern float aoStrength;

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
    
    // Detect initial press (rising edge)
    if (isHeld && !wasHeld) {
        repeatTimer = 0.0f;
        return true;  // Trigger on initial press
    }
    
    // If still held, manage repeat with debounce timer
    if (isHeld) {
        repeatTimer += deltaTime;
        if (repeatTimer >= REPEAT_DELAY) {
            // Reset timer to trigger at intervals
            repeatTimer -= REPEAT_INTERVAL;
            return true;
        }
    }
    
    return false;
}

void DebugInputSystem::update(GLFWwindow* window, float deltaTime) {
    if (!window) return;

    // ===== DAY/NIGHT CYCLE INPUT =====
    // Up/Down: adjust speed with debounced repeat (press once + slow repeat)
    if (checkArrowWithDebounce(window, GLFW_KEY_UP, prev_UP, arrow_repeat_timer_UP, deltaTime)) {
        daySpeed = glm::clamp(daySpeed + INCREMENT_STEP, 0.0f, 1.0f);
        printf("Day speed: %.3f\n", daySpeed);
    }

    if (checkArrowWithDebounce(window, GLFW_KEY_DOWN, prev_DOWN, arrow_repeat_timer_DOWN, deltaTime)) {
        daySpeed = glm::clamp(daySpeed - INCREMENT_STEP, 0.0f, 1.0f);
        printf("Day speed: %.3f\n", daySpeed);
    }

    // Left/Right: manual time stepping when paused (with debounced repeat)
    if (dayPaused && checkArrowWithDebounce(window, GLFW_KEY_LEFT, prev_LEFT, arrow_repeat_timer_LEFT, deltaTime)) {
        dayTime -= INCREMENT_STEP;
        if (dayTime < 0.0f) dayTime = 0.0f;
        printf("Day time: %.3f\n", dayTime);
    }

    if (dayPaused && checkArrowWithDebounce(window, GLFW_KEY_RIGHT, prev_RIGHT, arrow_repeat_timer_RIGHT, deltaTime)) {
        dayTime += INCREMENT_STEP;
        if (dayTime > 1.0f) dayTime = 1.0f;
        printf("Day time: %.3f\n", dayTime);
    }

    // M: toggle pause
    if (keyPressedOnce(window, GLFW_KEY_M, prev_M)) {
        dayPaused = !dayPaused;
        printf("Day cycle paused: %s\n", dayPaused ? "YES" : "NO");
    }

    // ===== RENDER DEBUG TOGGLES (F-keys) =====
    if (keyPressedOnce(window, GLFW_KEY_F10, prev_F10)) {
        usePbrShader = !usePbrShader;
        printf("Mode rendu: %s\n", usePbrShader ? "PBR" : "BASIC");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F9, prev_F9)) {
        debugTBN = !debugTBN;
        printf("Debug TBN: %s\n", debugTBN ? "ON" : "OFF");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F8, prev_F8)) {
        useNormalMap = !useNormalMap;
        printf("Normal map: %s\n", useNormalMap ? "ON" : "OFF");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F7, prev_F7)) {
        debugDiffuseOnly = !debugDiffuseOnly;
        printf("Diffuse only: %s\n", debugDiffuseOnly ? "ON" : "OFF");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F4, prev_F4)) {
        useBakedAO = !useBakedAO;
        printf("Baked AO: %s\n", useBakedAO ? "ON" : "OFF");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F5, prev_F5)) {
        useHemisphericalAmbient = !useHemisphericalAmbient;
        printf("Hemispherical ambient: %s\n", useHemisphericalAmbient ? "ON" : "OFF");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F6, prev_F6)) {
        useReducedAmbient = !useReducedAmbient;
        printf("Reduced ambient: %s\n", useReducedAmbient ? "ON" : "OFF");
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_O, prev_O)) {
        // TODO: Define what O key does in your engine
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_P, prev_P)) {
        // TODO: Define what P key does in your engine
    }

    // ===== AO STRENGTH ADJUSTMENTS =====
    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_O, prev_O)) {
        aoStrength = glm::clamp(aoStrength - 0.05f, 0.0f, 1.0f);
        printf("AO strength: %.2f\n", aoStrength);
    }

    if (usePbrShader && keyPressedOnce(window, GLFW_KEY_P, prev_P)) {
        aoStrength = glm::clamp(aoStrength + 0.05f, 0.0f, 1.0f);
        printf("AO strength: %.2f\n", aoStrength);
    }
}
