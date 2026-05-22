#include "debugSystem.hpp"

void DebugSystem::update(Registry& registry, GLFWwindow* _window, float deltaTime) {
    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    
    // 2. Mettre à jour ImGui IO
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayW, (float)displayH);
    
    ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug Info")) {
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
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Performance ===");
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("DeltaTime: %.4f ms", deltaTime * 1000.0f);
    }
    ImGui::End();

    ImGui::Render();
}