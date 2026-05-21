// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <cmath>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// ImGui
#include "imgui.h"
#include "imgui_impl_opengl3.h"

// Inclusions de notre moteur (Nouvelle architecture ECS)
#include "engine/render/shader.hpp"
#include "engine/io/textureLoader.hpp"
#include "engine/scene/camera.hpp"
#include "modules/terrain_gen/TerrainGenerator.hpp"
#include "modules/pathfinding/PathFinder3D.hpp"

// ECS Includes
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/systems/chunkMeshingSystem.hpp"
#include "ecs/systems/renderSystem.hpp"

void processInput(GLFWwindow *window, Camera& camera);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

/*******************************************************************************/

int main( void ) {
    
    // Initialisation de GLFW
    if( !glfwInit() ) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow( 1024, 768, "Voxel Engine - Prototype", NULL, NULL);
    if( window == NULL ){
        fprintf(stderr, "Failed to open GLFW window.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLEW
    glewExperimental = true; 
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Couleur de fond (Ciel bleu typique)
    glClearColor(0.39f, 0.65f, 0.85f, 1.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Activation du Culling (Indispensable pour les Voxels)
    // Modification temporaire : désactivé pour debugger les faces visibles
    glDisable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Chargement du shader générique
    GLuint basicProgramID = LoadShaders("assets/shaders/vertex_shader.glsl", "assets/shaders/fragment_shader.glsl");
    glUseProgram(basicProgramID);
    
    // === INITIALISATION ImGui ===
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "build/imgui.ini";
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 150");
    
    // === INITIALISATION DU MONDE ECS ===
    
    printf("=== PREMIER TEST : AFFICHAGE D'UN CHUNK ===\n");
    printf("Initialisation de la Registry ECS...\n");
    
    // Créer la Registry ECS
    Registry registry;
    
    // Créer 1 chunk de test
    EntityID testChunkEntity = registry.createEntity();
    printf("Chunk créé (EntityID: %u)\n", testChunkEntity);
    
    // Remplir le chunk avec des données voxel (terrain manuel pour démo)
    ChunkComponent chunkData(glm::ivec3(0, 0, 0));
    printf("Génération du terrain manuel (16×256×16 voxels)...\n");
    
    printf("Génération du terrain procédural (10x10 Chunks)...\n");
    
    TerrainConfig config = LoadConfig("config.txt");
    TerrainGenerator generator(config);

    int numChunksX = 2;
    int numChunksZ = 2;
    int chunksGenerated = 0;

    for (int chunkX = 0; chunkX < numChunksX; ++chunkX) {
        for (int chunkZ = 0; chunkZ < numChunksZ; ++chunkZ) {
        
            EntityID currentChunkEntity = registry.createEntity();
            ChunkComponent chunkData(glm::ivec3(chunkX, 0, chunkZ));
            std::vector<BlockType> proceduralBlocks = generator.GenerateChunk(chunkX, chunkZ);
            for (int y = 0; y < TerrainGenerator::CHUNK_HEIGHT; ++y) {
                for (int z = 0; z < TerrainGenerator::CHUNK_DEPTH; ++z) {
                    for (int x = 0; x < TerrainGenerator::CHUNK_WIDTH; ++x) {
                        
                        int index = generator.GetIndex(x, y, z);
                        BlockType myBlock = proceduralBlocks[index];
                        VoxelType theirType = VoxelType::AIR;
                        
                        switch (myBlock) {
                            case BlockType::GRASS:   theirType = VoxelType::GRASS; break;
                            case BlockType::DIRT:    theirType = VoxelType::DIRT; break;
                            case BlockType::STONE:   theirType = VoxelType::STONE; break;
                            // case BlockType::WOOD:    theirType = VoxelType::WOOD; break;
                            // case BlockType::LEAVES:  theirType = VoxelType::LEAVES; break;
                            // case BlockType::COAL:    theirType = VoxelType::COAL; break;
                            // case BlockType::BEDROCK: theirType = VoxelType::BEDROCK; break;
                        }

                        chunkData.setVoxel(x, y, z, theirType);
                    }
                }
            }

            chunkData.meshDirty = true;
            
            registry.addComponent(currentChunkEntity, chunkData);
            registry.addComponent(currentChunkEntity, MeshComponent());
            
            float worldPosX = chunkX * TerrainGenerator::CHUNK_WIDTH;
            float worldPosZ = chunkZ * TerrainGenerator::CHUNK_DEPTH;
            registry.addComponent(currentChunkEntity, TransformComponent(glm::vec3(worldPosX, 0.0f, worldPosZ)));

            chunksGenerated++;
        }
    }
    printf("  → %d chunks générés et injectés avec succès !\n", chunksGenerated);

    chunkData.meshDirty = true;  // Marquer pour remaillage
    
    registry.addComponent(testChunkEntity, chunkData);
    registry.addComponent(testChunkEntity, MeshComponent());
    registry.addComponent(testChunkEntity, TransformComponent(glm::vec3(0.0f, 0.0f, 0.0f)));
    
    // Créer les systèmes
    ChunkMeshingSystem meshingSystem;
    RenderSystem renderSystem;
    
    printf("Systèmes ECS créés (ChunkMeshingSystem, RenderSystem).\n");
    printf("Caméra initialisée en mode FREE_CAMERA.\n");
    printf("Contrôles: WASD=mouvement XZ, Space/Ctrl=haut/bas, Souris=rotation\n");
    printf("\n=== BOUCLE DE RENDU COMMENCÉE ===\n\n");

    // Initialisation de la caméra (Mode Libre par défaut)
    Camera camera;
    camera.initialize(
        glm::vec3(8.0f, 20.0f, 8.0f),   // Position initiale en hauteur
        glm::vec3(8.0f, 0.0f, -8.0f),   // Regarde vers le bas
        glm::vec3(0.0f, 1.0f, 0.0f),    // Vecteur Up
        15.0f                           // Vitesse
    );
    camera.setMode(FREE_CAMERA, window);

    do {
        // Calcul du deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Inputs
        processInput(window, camera);

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update Caméra
        camera.update(window, deltaTime);
        
        // Calcul ViewProjection
        glm::mat4 viewMatrix = camera.getViewMatrix();
        glm::mat4 projMatrix = camera.getProjectionMatrix();

        // Update ECS Systems
        meshingSystem.update(registry);
        renderSystem.update(registry, basicProgramID, viewMatrix, projMatrix);

        // === ImGui Debug UI ===
        ImGuiIO& io = ImGui::GetIO();
        int displayW, displayH;
        glfwGetWindowSize(window, &displayW, &displayH);
        io.DisplaySize = ImVec2((float)displayW, (float)displayH);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Debug Info")) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Camera Debug ===");
            
            glm::vec3 camPos = camera.getPosition();
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
            
            glm::vec3 camFront = camera.getFront();
            ImGui::Text("Direction: (%.2f, %.2f, %.2f)", camFront.x, camFront.y, camFront.z);
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "=== Performance ===");
            ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
            ImGui::Text("DeltaTime: %.4f ms", deltaTime * 1000.0f);
        }
        ImGui::End();

        ImGui::Render();
        
        // Disable depth test for ImGui rendering (it's 2D overlay)
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glEnable(GL_DEPTH_TEST);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0) );

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    // Cleanup
    glDeleteProgram(basicProgramID);
    glDeleteVertexArrays(1, &VertexArrayID);

    glfwTerminate();
    return 0;
}

// Gestion des inputs
void processInput(GLFWwindow *window, Camera& camera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Fullscreen toggle with F11 (requires GLFW 3.2+, we have 3.1, so simplified)
    static bool f11_pressed_last = false;
    bool f11_pressed = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
    if (f11_pressed && !f11_pressed_last) {
        printf("[DEBUG] F11 pressed - fullscreen toggle not fully supported in GLFW 3.1\n");
    }
    f11_pressed_last = f11_pressed;
}

// Resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}