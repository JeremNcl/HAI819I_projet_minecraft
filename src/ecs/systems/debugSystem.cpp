#include "debugSystem.hpp"

void DebugSystem::update(Registry& registry, GLFWwindow* _window, float deltaTime, const RenderDebugState& renderState) {
    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayW, (float)displayH);
    
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Camera Debug ===");
        auto view = registry.view<CameraComponent, TransformComponent>();
        for (EntityID entity : view) {
            auto& cam = registry.getComponent<CameraComponent>(entity);
            auto& transform = registry.getComponent<TransformComponent>(entity);
            
            if (cam.isActive) {
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", transform.position.x, transform.position.y, transform.position.z);
                ImGui::Text("Front: (%.2f, %.2f, %.2f)", cam.front.x, cam.front.y, cam.front.z);
            }
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Performance (averaged over 0.5s) ===");
        
        // Accumulate stats
        frameCount++;
        frameTimeAccum += deltaTime;
        statsUpdateTimer += deltaTime;
        
        // Update displayed stats every STATS_UPDATE_INTERVAL
        if (statsUpdateTimer >= STATS_UPDATE_INTERVAL) {
            lastDisplayedFps = (frameCount > 0) ? frameCount / frameTimeAccum : 60.0f;
            lastDisplayedDeltaTime = (frameCount > 0) ? frameTimeAccum / frameCount : 0.016f;
            frameCount = 0;
            frameTimeAccum = 0.0f;
            statsUpdateTimer = 0.0f;
        }
        
        ImGui::Text("FPS: %.1f", lastDisplayedFps);
        ImGui::Text("DeltaTime: %.4f ms", lastDisplayedDeltaTime * 1000.0f);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "=== Render Toggles ===");
        
        // Checkbox Wireframe du main branch
        ImGui::Checkbox("Wireframe mode", &m_debugWireframe);
        
        // Variables PBR de ta branche
        ImGui::Text("PBR: %s", renderState.usePbrShader ? "ON" : "OFF");
        ImGui::Text("F5 Hemispherical ambient: %s", renderState.useHemisphericalAmbient ? "ON" : "OFF");
        ImGui::Text("F6 Preset: %s", renderState.useReducedAmbient ? "CRISP" : "SOFT");
        ImGui::Text("F4 Baked AO: %s", renderState.useBakedAO ? "ON" : "OFF");
        ImGui::Text("F7 Diffuse only: %s", renderState.debugDiffuseOnly ? "ON" : "OFF");
        ImGui::Text("F8 Normal map: %s", renderState.useNormalMap ? "ON" : "OFF");
        ImGui::Text("F9 Debug TBN: %s", renderState.debugTBN ? "ON" : "OFF");
        ImGui::SliderFloat("AO Strength", &aoStrength, 0.0f, 1.0f);
        ImGui::Text("Ambient strength: %.3f", renderState.ambientStrength);
        ImGui::Text("Exposure: %.3f", renderState.exposure);
        ImGui::Text("Light color: (%.2f, %.2f, %.2f)", renderState.lightColor.x, renderState.lightColor.y, renderState.lightColor.z);
        ImGui::Text("Sky ambient: (%.2f, %.2f, %.2f)", renderState.ambientSkyColor.x, renderState.ambientSkyColor.y, renderState.ambientSkyColor.z);
        ImGui::Text("Ground ambient: (%.2f, %.2f, %.2f)", renderState.ambientGroundColor.x, renderState.ambientGroundColor.y, renderState.ambientGroundColor.z);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f,0.7f,0.9f,1.0f), "=== Shortcuts ===");
        ImGui::Text(",: pause/resume day cycle    Left/Right: step when paused");
        ImGui::Text("Up/Down: increase/decrease day speed    O/P: AO strength");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "=== Day / Night Cycle ===");
        ImGui::Text("Day time: %.3f", dayTime);
        
        // Convert dayTime [0,1] to hours [0,24]
        int hours = static_cast<int>(dayTime * 24.0f);
        int minutes = static_cast<int>((dayTime * 24.0f - hours) * 60.0f);
        ImGui::Text("Time: %02d:%02d", hours, minutes);
        
        if (ImGui::SliderFloat("DayTime", &dayTime, 0.0f, 1.0f)) {}
        ImGui::Text("Speed: %.3f", daySpeed);
        if (ImGui::SliderFloat("Day Speed", &daySpeed, 0.0f, 1.0f)) {}
        ImGui::Text("Paused: %s", dayPaused ? "YES" : "NO");
        if (ImGui::Button(dayPaused ? "Resume" : "Pause")) {
            dayPaused = !dayPaused;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step -") && dayPaused) { dayTime = glm::clamp(dayTime - 0.01f, 0.0f, 1.0f); }
        ImGui::SameLine();
        if (ImGui::Button("Step +") && dayPaused) { dayTime = glm::clamp(dayTime + 0.01f, 0.0f, 1.0f); }
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