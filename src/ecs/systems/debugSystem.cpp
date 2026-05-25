#include "debugSystem.hpp"

bool DebugSystem::keyPressedOnce(GLFWwindow* _window, int _key){
    bool isPressed = (glfwGetKey(_window, _key) == GLFW_PRESS);
    bool triggered = isPressed && !m_previousState[_key];
    m_previousState[_key] = isPressed;
    return triggered;
}

void DebugSystem::update(Registry& _registry, GLFWwindow* _window, float _deltaTime, float _rawDeltaTime) {
    if (keyPressedOnce(_window, GLFW_KEY_F10)) {
        m_usePbrShader = !m_usePbrShader;
        std::cout << "Mode rendu: " << (m_usePbrShader ? "PBR" : "BASIC") << std::endl;
    }
    if (m_usePbrShader && keyPressedOnce(_window, GLFW_KEY_F9)) {
        m_debugTBN = !m_debugTBN;
        std::cout << "Debug TBN: " << (m_debugTBN ? "ON" : "OFF") << std::endl;
    }
    if (m_usePbrShader && keyPressedOnce(_window, GLFW_KEY_F8)) {
        m_useNormalMap = !m_useNormalMap;
        std::cout << "Normal map: " << (m_useNormalMap ? "ON" : "OFF") << std::endl;
    }
    if (m_usePbrShader && keyPressedOnce(_window, GLFW_KEY_F7)) {
        m_debugDiffuseOnly = !m_debugDiffuseOnly;
        std::cout << "Diffuse only: " << (m_debugDiffuseOnly ? "ON" : "OFF") << std::endl;
    }
    if (m_usePbrShader && keyPressedOnce(_window, GLFW_KEY_F6)) {
        m_useReducedAmbient = !m_useReducedAmbient;
        std::cout << "Ambient preset: " << (m_useReducedAmbient ? "CRISP" : "SOFT") << std::endl;
    }
    if (keyPressedOnce(_window, GLFW_KEY_F5)) {
        m_debugWireframe = !m_debugWireframe;
    }

    // 2. Mettre à jour ImGui IO
    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayW, (float)displayH);
    
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug Info")) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Camera Debug ===");
        auto view = _registry.view<CameraComponent, TransformComponent>();
        for (EntityID entity : view) {
            auto& cam = _registry.getComponent<CameraComponent>(entity);
            auto& transform = _registry.getComponent<TransformComponent>(entity);
            
            if (cam.isActive) {
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", transform.position.x, transform.position.y, transform.position.z);
                ImGui::Text("Front: (%.2f, %.2f, %.2f)", cam.front.x, cam.front.y, cam.front.z);
            }
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Performance ===");
        ImGui::Text("FPS: %.1f", 1.0f / _rawDeltaTime);
        ImGui::Text("DeltaTime: %.4f ms", _deltaTime * 1000.0f);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Render Settings ===");
        
        ImGui::Checkbox("Wireframe mode (F5)", &m_debugWireframe);
        ImGui::Checkbox("Use PBR Shader (F10)", &m_usePbrShader);
        
        if (m_usePbrShader) {
            ImGui::Indent(15.0f);
            ImGui::Checkbox("Show TBN Vectors (F9)", &m_debugTBN);
            ImGui::Checkbox("Enable Normal Map (F8)", &m_useNormalMap);
            ImGui::Checkbox("Debug Diffuse Only (F7)", &m_debugDiffuseOnly);
            ImGui::Checkbox("Reduced Ambient [CRISP] (F6)", &m_useReducedAmbient);
            ImGui::Unindent(15.0f);
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(PBR options disabled in Basic Mode)");
        }
    }
    ImGui::End();

    ImGui::Render();
}

void DebugSystem::renderLoadingScreen(GLFWwindow* _window, int _currentMeshesReady, int _totalExpectedMeshes) {

    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayW, (float)displayH);
    
    ImGui::NewFrame();
    
    ImGuiIO& current_io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(current_io.DisplaySize);
    ImGui::Begin("LoadingScreen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

    float progress = (_totalExpectedMeshes > 0) ? (float)_currentMeshesReady / _totalExpectedMeshes : 1.0f;

    ImGui::SetCursorPos(ImVec2(current_io.DisplaySize.x * 0.35f, current_io.DisplaySize.y * 0.45f));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "GENERATION DES MAILLAGES (MESHES)...");
    
    ImGui::SetCursorPos(ImVec2(current_io.DisplaySize.x * 0.35f, current_io.DisplaySize.y * 0.50f));
    ImGui::Text("Meshes prepares : %d / %d", _currentMeshesReady, _totalExpectedMeshes);
    
    ImGui::SetCursorPos(ImVec2(current_io.DisplaySize.x * 0.25f, current_io.DisplaySize.y * 0.55f));
    ImGui::ProgressBar(progress, ImVec2(current_io.DisplaySize.x * 0.5f, 30.0f));

    ImGui::End();
    ImGui::Render();
}