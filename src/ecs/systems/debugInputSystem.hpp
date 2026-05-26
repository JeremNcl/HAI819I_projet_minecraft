#pragma once
#ifndef DEBUGINPUTSYSTEM_HPP
#define DEBUGINPUTSYSTEM_HPP

#include <GLFW/glfw3.h>
#include "ecs/registry.hpp"

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
extern bool debugWireframe;

class DebugInputSystem {
    private:
        // Key press tracking for one-time toggles
        bool prev_F1 = false;
        bool prev_F3 = false;
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

        // Repeat timers for held arrow keys
        float arrow_repeat_timer_UP = 0.0f;
        float arrow_repeat_timer_DOWN = 0.0f;
        float arrow_repeat_timer_LEFT = 0.0f;
        float arrow_repeat_timer_RIGHT = 0.0f;
        static constexpr float REPEAT_DELAY = 0.2f;
        static constexpr float REPEAT_INTERVAL = 0.08f;
        static constexpr float INCREMENT_STEP = 0.02f;

        bool checkKeyHeld(GLFWwindow* window, int key);
        bool keyPressedOnce(GLFWwindow* window, int key, bool& prev);
        bool checkArrowWithDebounce(GLFWwindow* window, int key, bool& prevState, 
                                   float& repeatTimer, float deltaTime);

    public:
        DebugInputSystem() = default;

        void update(Registry& registry, GLFWwindow* window, float deltaTime);
};

#endif