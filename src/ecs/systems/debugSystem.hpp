#pragma once
#ifndef DEBUGSYSTEM_HPP
#define DEBUGSYSTEM_HPP

#include "ecs/registry.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/inputReceiver.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <array>
#include <iostream>

class DebugSystem {
    private:
        std::array<bool, GLFW_KEY_LAST + 1> m_previousState{};

        bool m_debugWireframe = false;
        bool m_usePbrShader = true;
        bool m_debugTBN = false;
        bool m_useNormalMap = true;
        bool m_debugDiffuseOnly = false;
        bool m_useReducedAmbient = false;

        bool keyPressedOnce(GLFWwindow* _window, int _key);

    public:
        void update(Registry& _registry, GLFWwindow* _window, float _deltaTime, float _rawDeltaTime);
        void renderLoadingScreen(GLFWwindow* _window, int _currentMeshesReady, int _totalExpectedMeshes);

        bool isWireframe() const { return m_debugWireframe; }
        bool isPbrShader() const { return m_usePbrShader; }
        bool isDebugTBN() const { return m_debugTBN; }
        bool isNormalMap() const { return m_useNormalMap; }
        bool isDiffuseOnly() const { return m_debugDiffuseOnly; }
        bool isReducedAmbient() const { return m_useReducedAmbient; }
};

#endif