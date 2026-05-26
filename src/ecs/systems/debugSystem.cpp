#include "debugSystem.hpp"

extern bool debugWireframe;


static const char* format_block_name(VoxelType type) {
    switch(type) {
        case VoxelType::STONE:   return "Pierre";
        case VoxelType::DIRT:    return "Terre";
        case VoxelType::GRASS:   return "Herbe";
        case VoxelType::WOOD:    return "Bois";
        case VoxelType::LEAVES:  return "Feuilles";
        case VoxelType::BEDROCK: return "Bedrock";
        case VoxelType::COAL:    return "Minerai de Charbon";
        case VoxelType::IRON:    return "Minerai de Fer";
        case VoxelType::GOLD:    return "Minerai d'Or";
        case VoxelType::DIAMOND: return "Diamant";
        case VoxelType::SAND:    return "Sable";
        default:                 return "Bloc Inconnu";
    }
}
// ------------------------------------------------------------------

void DebugSystem::renderInventoryUI(Registry& registry, EntityID playerID, GLFWwindow* window) {
    auto& inv = registry.getComponent<InventoryComponent>(playerID);
    
    // Gestion de l'ouverture avec 'V'
    static bool prev_V = false;
    bool curr_V = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
    if (curr_V && !prev_V) {
        inv.isOpen = !inv.isOpen;
    }
    prev_V = curr_V;

    ImGuiIO& io = ImGui::GetIO();

    // === 1. LA HOTBAR (Toujours visible en bas) ===
    // On la place au centre en bas (Y = DisplaySize.y - 10)
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 10.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
    ImGui::Begin("Hotbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    
    // Style Minecraft avec un fond semi-transparent
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.7f));
    ImGui::BeginChild("HotbarBackground", ImVec2(200, 60), true);
    
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "> %s <", format_block_name(inv.selectedBlock));
    int count = inv.items.find(inv.selectedBlock) != inv.items.end() ? inv.items[inv.selectedBlock] : 0;
    ImGui::Text("Quantite : %d", count);
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::End();

    // === 2. LE SAC A DOS (Menu principal avec grille) ===
    if (inv.isOpen) {
        // Centrage parfait au milieu de l'écran
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        // On FORCE la taille avec ImGuiCond_Always pour écraser ta "petite fenêtre" buguée
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always); 
        
        // On enlève le titre et le redimensionnement pour faire un menu de jeu immersif
        ImGui::Begin("Inventaire", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar); 
        
        ImGui::TextDisabled("Appuyez sur 'V' pour fermer");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Inventaire du joueur");
        ImGui::Spacing();

        // Création d'une grille style Minecraft (9 colonnes)
        if (ImGui::BeginTable("InventoryGrid", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
            for (auto& [type, qty] : inv.items) {
                if (qty > 0) {
                    ImGui::TableNextColumn();
                    
                    bool isSelected = (type == inv.selectedBlock);
                    
                    // Si c'est le bloc sélectionné, on le met en vert fluo, sinon gris
                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
                    }

                    // Bouton carré (60x60 pixels)
                    std::string label = std::string(format_block_name(type)) + "\n" + std::to_string(qty);
                    if (ImGui::Button(label.c_str(), ImVec2(60, 60))) {
                        inv.selectedBlock = type; // On équipe ce bloc au clic !
                    }
                    
                    if (isSelected) {
                        ImGui::PopStyleColor(2);
                    }
                }
            }
            ImGui::EndTable();
        }
        
        ImGui::End();
    }
}

void DebugSystem::update(Registry& registry, GLFWwindow* _window, float deltaTime, const RenderDebugState& renderState) {
    int displayW, displayH;
    glfwGetFramebufferSize(_window, &displayW, &displayH);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)displayW, (float)displayH);
    
    //ImGui::NewFrame();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 450), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // ==========================================
        // CAMERA DEBUG
        // ==========================================
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Camera Debug ===");
        auto view = registry.view<CameraComponent, TransformComponent>();
        for (EntityID entity : view) {
            auto& cam = registry.getComponent<CameraComponent>(entity);
            auto& transform = registry.getComponent<TransformComponent>(entity);
            
            if (cam.isActive) {
                ImGui::Text("Active Camera ID: %d", entity);
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", transform.position.x, transform.position.y, transform.position.z);
                ImGui::Text("Front: (%.2f, %.2f, %.2f)", cam.front.x, cam.front.y, cam.front.z);
            }
        }
        
        // ==========================================
        // PERFORMANCE
        // ==========================================
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Performance (averaged over 0.5s) ===");
        
        frameCount++;
        frameTimeAccum += deltaTime;
        statsUpdateTimer += deltaTime;
        
        if (statsUpdateTimer >= STATS_UPDATE_INTERVAL) {
            lastDisplayedFps = (frameCount > 0) ? frameCount / frameTimeAccum : 60.0f;
            lastDisplayedDeltaTime = (frameCount > 0) ? frameTimeAccum / frameCount : 0.016f;
            frameCount = 0;
            frameTimeAccum = 0.0f;
            statsUpdateTimer = 0.0f;
        }
        
        ImGui::Text("FPS: %.1f", lastDisplayedFps);
        ImGui::Text("DeltaTime: %.4f ms", lastDisplayedDeltaTime * 1000.0f);

        // ==========================================
        // RENDER TOGGLES (Classés par touches Fx)
        // ==========================================
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "=== Render Toggles ===");
        
        ImGui::Text("F1 Camera Swap: (Press to switch active camera)");
        ImGui::Text("F3 Wireframe mode: %s", debugWireframe ? "ON" : "OFF");
        ImGui::Text("F4 Baked AO: %s", renderState.useBakedAO ? "ON" : "OFF");
        ImGui::Text("F5 Hemispherical ambient: %s", renderState.useHemisphericalAmbient ? "ON" : "OFF");
        ImGui::Text("F6 Preset: %s", renderState.useReducedAmbient ? "CRISP" : "SOFT");
        ImGui::Text("F7 Diffuse only: %s", renderState.debugDiffuseOnly ? "ON" : "OFF");
        ImGui::Text("F8 Normal map: %s", renderState.useNormalMap ? "ON" : "OFF");
        ImGui::Text("F9 Debug TBN: %s", renderState.debugTBN ? "ON" : "OFF");
        ImGui::Text("F10 PBR Shader mode: %s", renderState.usePbrShader ? "ON" : "OFF");

        // ==========================================
        // RENDER PARAMETERS (Nouveau titre jaune)
        // ==========================================
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "=== Render Parameters ===");
        
        ImGui::SliderFloat("AO Strength", &aoStrength, 0.0f, 1.0f);
        ImGui::Text("Ambient strength: %.3f", renderState.ambientStrength);
        ImGui::Text("Exposure: %.3f", renderState.exposure);
        ImGui::Text("Light color: (%.2f, %.2f, %.2f)", renderState.lightColor.x, renderState.lightColor.y, renderState.lightColor.z);
        ImGui::Text("Sky ambient: (%.2f, %.2f, %.2f)", renderState.ambientSkyColor.x, renderState.ambientSkyColor.y, renderState.ambientSkyColor.z);
        ImGui::Text("Ground ambient: (%.2f, %.2f, %.2f)", renderState.ambientGroundColor.x, renderState.ambientGroundColor.y, renderState.ambientGroundColor.z);

        // ==========================================
        // SHORTCUTS & CYCLE
        // ==========================================
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "=== Shortcuts ===");
        ImGui::Text(",: pause/resume day cycle    Left/Right: step when paused");
        ImGui::Text("Up/Down: increase/decrease day speed    O/P: AO strength");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "=== Day / Night Cycle ===");
        ImGui::Text("Day time: %.3f", dayTime);
        
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

    //ImGui::Render();
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