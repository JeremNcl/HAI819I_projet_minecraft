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
//#include "engine/scene/camera.hpp"

// ECS Includes
#include "ecs/registry.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/inputReceiver.hpp"
#include "ecs/systems/chunkMeshingSystem.hpp"
#include "ecs/systems/renderSystem.hpp"
#include "ecs/systems/inputSystem.hpp"
#include "ecs/systems/cameraSystem.hpp"
#include "ecs/systems/windowSystem.hpp"
#include "ecs/systems/debugSystem.hpp"

//void processInput(GLFWwindow *window, Camera& camera);
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
    
    // Remplissage manuel : couche pierre en bas, puis dirt, puis herbe
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            for (int y = 0; y < 256; ++y) {
                if (y < 5) {
                    chunkData.setVoxel(x, y, z, VoxelType::STONE);
                } else if (y < 10) {
                    chunkData.setVoxel(x, y, z, VoxelType::DIRT);
                } else if (y == 10) {
                    chunkData.setVoxel(x, y, z, VoxelType::GRASS);
                } else {
                    chunkData.setVoxel(x, y, z, VoxelType::AIR);
                }
            }
        }
    }
    printf("  → %u voxels créés (STONE: 0-5, DIRT: 5-10, GRASS: 10)\n", VOXEL_ARRAY_SIZE);
    chunkData.meshDirty = true;  // Marquer pour remaillage
    
    registry.addComponent(testChunkEntity, chunkData);
    registry.addComponent(testChunkEntity, MeshComponent());
    registry.addComponent(testChunkEntity, TransformComponent(glm::vec3(0.0f, 0.0f, 0.0f)));
    
    // Créer les systèmes
    ChunkMeshingSystem meshingSystem;
    RenderSystem renderSystem;
    InputSystem inputSystem(window);
    WindowSystem windowSystem;
    CameraSystem cameraSystem;
    DebugSystem debugSystem;

    EntityID camEntity = registry.createEntity();
    registry.addComponent(camEntity, TransformComponent{
        glm::vec3(0,0,3),
        glm::vec3(0,0,0)
    });
    registry.addComponent(camEntity, CameraComponent{ .isActive = true });
    registry.addComponent(camEntity, InputReceiverComponent{});

    //Setup curseur au demarrage
    inputSystem.setCursorMode(window, true);

    int major, minor, rev;
    glfwGetVersion(&major, &minor, &rev);
    printf("Version GLFW : %d.%d.%d\n", major, minor, rev);

    printf("Systèmes ECS créés (ChunkMeshingSystem, RenderSystem, InputSystem, CameraSystem).\n");
    printf("Caméra initialisée.\n");
    printf("Contrôles: ZQSD=mouvement XZ, Space/Ctrl=haut/bas, Souris=rotation\n");
    printf("\n=== BOUCLE DE RENDU COMMENCÉE ===\n\n");

    do {
        // Calcul du deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update ECS Systems
        meshingSystem.update(registry);
        renderSystem.update(registry, basicProgramID);
        inputSystem.update(registry, window);
        cameraSystem.update(registry, deltaTime);
        windowSystem.update(registry, window);        
        

        // === ImGui Debug UI ===
        ImGui_ImplOpenGL3_NewFrame();
        debugSystem.update(registry, window, deltaTime);
        
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

// Resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}