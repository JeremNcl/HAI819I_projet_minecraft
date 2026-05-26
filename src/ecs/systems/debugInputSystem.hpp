#pragma once
#ifndef DEBUGINPUTSYSTEM_HPP
#define DEBUGINPUTSYSTEM_HPP

#include <GLFW/glfw3.h>
#include "../../ecs/registry.hpp"

// Global debug variables (controlled by this system)
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

class DebugInputSystem {
    private:
        // Key press tracking for one-time toggles (render settings)
        // These use prev_* to detect rising edges (press once)
        bool prev_F4 = false;
        bool prev_F5 = false;
        bool prev_F6 = false;
        bool prev_F7 = false;
        bool prev_F8 = false;
        bool prev_F9 = false;
        bool prev_F10 = false;
        bool prev_O = false;
        bool prev_P = false;
        bool prev_M = false;

        // Previous key states for debounce detection
        bool prev_UP = false;
        bool prev_DOWN = false;
        bool prev_LEFT = false;
        bool prev_RIGHT = false;

        // Repeat timers for held arrow keys (slow continuous adjustments)
        float arrow_repeat_timer_UP = 0.0f;
        float arrow_repeat_timer_DOWN = 0.0f;
        float arrow_repeat_timer_LEFT = 0.0f;
        float arrow_repeat_timer_RIGHT = 0.0f;
        static constexpr float REPEAT_DELAY = 0.2f;  // Initial delay before repeat starts
        static constexpr float REPEAT_INTERVAL = 0.08f;  // Interval between repeats
        static constexpr float INCREMENT_STEP = 0.02f;  // Amount to increment per press

        // Helper: Check if key is currently held
        bool checkKeyHeld(GLFWwindow* window, int key);

        // Helper: Check if key pressed exactly once (for toggles)
        bool keyPressedOnce(GLFWwindow* window, int key, bool& prev);

        // Helper: Check if arrow key triggered (initial press + debounced repeat)
        bool checkArrowWithDebounce(GLFWwindow* window, int key, bool& prevState, 
                                   float& repeatTimer, float deltaTime);

    public:
        DebugInputSystem() = default;

        // Update all debug inputs (needs registry for TimeComponent, LightingStateComponent)
        void update(Registry& registry, GLFWwindow* window, float deltaTime);
};

#endif
