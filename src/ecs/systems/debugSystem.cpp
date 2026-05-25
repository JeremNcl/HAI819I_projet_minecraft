#include "debugSystem.hpp"

void DebugSystem::update(Registry& registry, GLFWwindow* _window, float deltaTime, const RenderDebugState& renderState) {
    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    
    // 2. Mettre à jour ImGui IO
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayW, (float)displayH);
    
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Camera Debug ===");
        
        // Recuperation de la camera
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
        ImGui::Text("PBR: %s", renderState.usePbrShader ? "ON" : "OFF");
        ImGui::Text("F5 Hemispherical ambient: %s", renderState.useHemisphericalAmbient ? "ON" : "OFF");
        ImGui::Text("F6 Preset: %s", renderState.useReducedAmbient ? "CRISP" : "SOFT");
        ImGui::Text("F4 Baked AO: %s", renderState.useBakedAO ? "ON" : "OFF");
        ImGui::Text("F7 Diffuse only: %s", renderState.debugDiffuseOnly ? "ON" : "OFF");
        ImGui::Text("F8 Normal map: %s", renderState.useNormalMap ? "ON" : "OFF");
        ImGui::Text("F9 Debug TBN: %s", renderState.debugTBN ? "ON" : "OFF");
        ImGui::SliderFloat("AO Strength", &aoStrength, 0.0f, 1.0f);
        ImGui::Text("Ambient strength: %.3f", renderState.ambientStrength);
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
        
        if (ImGui::SliderFloat("DayTime", &dayTime, 0.0f, 1.0f)) {
            // dayTime modified by UI
        }
        ImGui::Text("Speed: %.3f", daySpeed);
        if (ImGui::SliderFloat("Day Speed", &daySpeed, 0.0f, 1.0f)) {
            // daySpeed modified by UI
        }
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