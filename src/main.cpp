// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>

using namespace glm;

// ImGui
#include "imgui.h"
#include "imgui_impl_opengl3.h"

// Inclusions de notre moteur (Nouvelle architecture ECS)
#include "engine/render/shader.hpp"
#include "engine/io/textureLoader.hpp"
#include "engine/render/BlockTextureManager.hpp"
#include "modules/terrain_gen/TerrainGenerator.hpp"
#include "modules/pathfinding/PathFinder3D.hpp"
#include "game/testScenes.hpp"

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
#include "ecs/systems/PathFindingSystem.hpp"
#include "ecs/systems/TerrainSystem.hpp"

//void processInput(GLFWwindow *window, Camera& camera);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void MeshingWorkerThread(Registry& registry, TerrainSystem& terrain, ChunkMeshingSystem& meshing);
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Debug flags
bool debugWireframe = false;
bool usePbrShader = true;
bool debugTBN = false;
bool useNormalMap = true;
bool debugDiffuseOnly = false;
bool useReducedAmbient = false;

static constexpr float kAmbientSoft = 0.045f;
static constexpr float kAmbientCrisp = 0.035f;

enum class TestSceneMode {
    SimpleSubChunk = 1,
    GeneratedChunk = 2,
    InfiniteTerrain = 3
};

static const char* sceneName(TestSceneMode sceneMode) {
    switch (sceneMode) {
        case TestSceneMode::SimpleSubChunk: return "Simple subchunk test scene";
        case TestSceneMode::GeneratedChunk: return "Single generated chunk test scene";
        case TestSceneMode::InfiniteTerrain: return "Infinite terrain test scene";
    }
    return "Unknown test scene";
}

static TestSceneMode parseSceneMode(int argc, char** argv) {
    if (argc > 1) {
        int value = std::atoi(argv[1]);
        if (value == 1) return TestSceneMode::SimpleSubChunk;
        if (value == 2) return TestSceneMode::GeneratedChunk;
        if (value == 3) return TestSceneMode::InfiniteTerrain;
    }
    return TestSceneMode::InfiniteTerrain;
}

static void positionCameraForScene(Registry& registry, EntityID cameraEntity, TestSceneMode sceneMode) {
    auto& transform = registry.getComponent<TransformComponent>(cameraEntity);

    switch (sceneMode) {
        case TestSceneMode::SimpleSubChunk:
            transform.position = glm::vec3(8.0f, 24.0f, -24.0f);
            break;
        case TestSceneMode::GeneratedChunk:
            transform.position = glm::vec3(8.0f, 40.0f, -32.0f);
            break;
        case TestSceneMode::InfiniteTerrain:
            transform.position = glm::vec3(45.0f, 120.0f, -55.0f);
            break;
    }
}

static void buildStaticSceneMeshes(Registry& registry, ChunkMeshingSystem& meshingSystem) {
    std::vector<EntityID> subChunkEntities;
    subChunkCache cache;

    auto view = registry.view<SubChunkComponent>();
    for (EntityID entity : view) {
        subChunkEntities.push_back(entity);
        cache.push_back(&registry.getComponent<SubChunkComponent>(entity));
    }

    std::sort(cache.begin(), cache.end(), [](const SubChunkComponent* a, const SubChunkComponent* b) {
        if (a->subChunkPosition.x != b->subChunkPosition.x) return a->subChunkPosition.x < b->subChunkPosition.x;
        if (a->subChunkPosition.y != b->subChunkPosition.y) return a->subChunkPosition.y < b->subChunkPosition.y;
        return a->subChunkPosition.z < b->subChunkPosition.z;
    });

    for (EntityID entity : subChunkEntities) {
        auto& subChunk = registry.getComponent<SubChunkComponent>(entity);
        auto& mesh = registry.getComponent<MeshComponent>(entity);
        MeshData meshData = meshingSystem.calculateMeshData(registry, entity, subChunk, cache);
        meshingSystem.uploadMeshToGPU(mesh, meshData);
        subChunk.meshDirty = false;
    }
}

static bool keyPressedOnce(GLFWwindow* _window, int key) {
    static std::array<bool, GLFW_KEY_LAST + 1> previousState{};
    bool isPressed = (glfwGetKey(_window, key) == GLFW_PRESS);
    bool triggered = isPressed && !previousState[key];
    previousState[key] = isPressed;
    return triggered;
}

static float getAmbientStrength() {
    return useReducedAmbient ? kAmbientCrisp : kAmbientSoft;
}

static glm::vec3 getLightColor() {
    return useReducedAmbient
        ? glm::vec3(3.00f, 2.94f, 2.86f)
        : glm::vec3(2.85f, 2.78f, 2.70f);
}

static void applyActiveShaderUniforms(GLuint activeProgramID) {
    GLint debugLoc = glGetUniformLocation(activeProgramID, "debugTBN");
    if (debugLoc >= 0) {
        glUniform1i(debugLoc, debugTBN ? 1 : 0);
    }

    GLint useNormalMapLoc = glGetUniformLocation(activeProgramID, "useNormalMap");
    if (useNormalMapLoc >= 0) {
        glUniform1i(useNormalMapLoc, useNormalMap ? 1 : 0);
    }

    GLint debugDiffuseOnlyLoc = glGetUniformLocation(activeProgramID, "debugDiffuseOnly");
    if (debugDiffuseOnlyLoc >= 0) {
        glUniform1i(debugDiffuseOnlyLoc, debugDiffuseOnly ? 1 : 0);
    }

    GLint ambientStrengthLoc = glGetUniformLocation(activeProgramID, "ambientStrength");
    if (ambientStrengthLoc >= 0) {
        glUniform1f(ambientStrengthLoc, getAmbientStrength());
    }
}

std::mutex ecsMutex;
std::atomic<bool> isGameRunning{true};

/*******************************************************************************/

int main(int argc, char** argv) {
    
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

    window = glfwCreateWindow( 1024, 720, "Voxel Engine - Prototype", NULL, NULL);
    if( window == NULL ){
        fprintf(stderr, "Failed to open GLFW window.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
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
    glfwSetCursorPos(window, 1024/2, 720/2);

    // Couleur de fond (Ciel bleu typique)
    glClearColor(0.39f, 0.65f, 0.85f, 1.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable face culling for performance (CCW winding order)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Chargement du shader générique
    GLuint basicProgramID = LoadShaders("assets/shaders/vertex_shader.glsl", "assets/shaders/fragment_shader.glsl");
    glUseProgram(basicProgramID);
    
    // Chargement du shader PBR
    GLuint pbrProgramID = LoadShaders("assets/shaders/pbr_vertex.glsl", "assets/shaders/pbr_fragment.glsl");
    
    // Initialisation du BlockTextureManager
    BlockTextureManager::initialize();
    
    // === INITIALISATION ImGui ===
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "build/imgui.ini";
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 150");
    
    // === INITIALISATION DU MONDE ECS ===
    
    printf("=== ECS Monde Initialization ===\n");
    printf("Initialisation de la Registry ECS...\n");
    
    Registry registry;
    
    
    // Créer les systèmes
    ChunkMeshingSystem meshingSystem;
    RenderSystem renderSystem;
    InputSystem inputSystem(window);
    WindowSystem windowSystem;
    CameraSystem cameraSystem;
    DebugSystem debugSystem;
    
    // Systèmes du dev bonus
    TerrainConfig config = LoadConfig("config.txt");
    TerrainSystem terrainSystem(config);
    PathFindingSystem pathFindingSystem;

    const TestSceneMode selectedScene = parseSceneMode(argc, argv);
    const bool useInfiniteTerrain = (selectedScene == TestSceneMode::InfiniteTerrain);

    printf("Scene de test activee: %s\n", sceneName(selectedScene));
    printf("Ligne de commande: 1=simple subchunk, 2=chunk genere, 3=terrain infini\n");
    
    printf("Systèmes ECS créés (ChunkMeshingSystem, RenderSystem).\n");
    printf("Caméra initialisée en mode FREE_CAMERA.\n");
    printf("Contrôles: WASD=mouvement XZ, Space/Ctrl=haut/bas, Souris=rotation, F6=ambiance\n");
    printf("\n=== BOUCLE DE RENDU COMMENCÉE ===\n\n");

    EntityID camEntity = registry.createEntity();
    registry.addComponent(camEntity, TransformComponent{
        glm::vec3(45,120,-55),
        glm::vec3(0,0,0)
    });
    registry.addComponent(camEntity, CameraComponent{ .isActive = true});
    registry.addComponent(camEntity, InputReceiverComponent{});

    cameraSystem.initCamera(registry, camEntity, 90, 0);
    positionCameraForScene(registry, camEntity, selectedScene);

    if (selectedScene == TestSceneMode::SimpleSubChunk) {
        TestScenes::createSimpleChunk(registry);
        buildStaticSceneMeshes(registry, meshingSystem);
    } else if (selectedScene == TestSceneMode::GeneratedChunk) {
        TestScenes::createGeneratedChunk(registry);
        buildStaticSceneMeshes(registry, meshingSystem);
    } else {
        TestScenes::createInfiniteTerrainScene(registry);
    }

    //Setup curseur au demarrage
    inputSystem.setCursorMode(window, true);

    int major, minor, rev;
    glfwGetVersion(&major, &minor, &rev);
    printf("Version GLFW : %d.%d.%d\n", major, minor, rev);

    printf("Systèmes ECS créés (ChunkMeshingSystem, RenderSystem, InputSystem, CameraSystem).\n");
    printf("Caméra initialisée.\n");
    printf("Contrôles: ZQSD=mouvement XZ, Space/Ctrl=haut/bas, Souris=rotation, F6=ambiance\n");
    printf("\n=== BOUCLE DE RENDU COMMENCÉE ===\n\n");

    bool isLoading = useInfiniteTerrain;
    const int TARGET_CHUNKS = useInfiniteTerrain ? 29 * 29 : 0; // (Rayon  * 2 + 1)^2 rayon = 14

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Sécurité

    std::vector<std::thread> workers;
    if (useInfiniteTerrain) {
        printf("Lancement de %d threads de maillage en parallele !\n", numThreads);
        for (unsigned int i = 0; i < numThreads; ++i) {
            workers.emplace_back(MeshingWorkerThread, std::ref(registry), std::ref(terrainSystem), std::ref(meshingSystem));
        }
    }
    do {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f; 
        }
        lastFrame = currentFrame;
        

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (useInfiniteTerrain) {
            {
                std::lock_guard<std::mutex> lock(ecsMutex);
                terrainSystem.update(registry);
                meshingSystem.update(registry);
            }

            int totalExpectedMeshes = TARGET_CHUNKS * 16;
            int currentMeshesReady = 0;

            if (isLoading) {
                currentMeshesReady = meshingSystem.getCompletedMeshCount(registry);
                if (currentMeshesReady >= totalExpectedMeshes) {
                    isLoading = false;
                }
            }

            if (isLoading) {

                if (windowSystem.update(registry, window)) {
                    inputSystem.resetMouseTracking(window);
                }

                int displayW, displayH;
                glfwGetFramebufferSize(window, &displayW, &displayH);
                
                ImGuiIO& current_io = ImGui::GetIO();
                current_io.DisplaySize = ImVec2((float)displayW, (float)displayH);

                ImGui_ImplOpenGL3_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(current_io.DisplaySize);
                ImGui::Begin("LoadingScreen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

                float progress = (float)currentMeshesReady / totalExpectedMeshes;
                // ----------------------------

                ImGui::SetCursorPos(ImVec2(current_io.DisplaySize.x * 0.35f, current_io.DisplaySize.y * 0.45f));
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "GENERATION DES MAILLAGES (MESHES)...");
                
                ImGui::SetCursorPos(ImVec2(current_io.DisplaySize.x * 0.35f, current_io.DisplaySize.y * 0.50f));
                ImGui::Text("Meshes prepares : %d / %d", currentMeshesReady, totalExpectedMeshes);
                
                ImGui::SetCursorPos(ImVec2(current_io.DisplaySize.x * 0.25f, current_io.DisplaySize.y * 0.55f));
                ImGui::ProgressBar(progress, ImVec2(current_io.DisplaySize.x * 0.5f, 30.0f));

                ImGui::End();
                ImGui::Render();

                glDisable(GL_DEPTH_TEST);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glEnable(GL_DEPTH_TEST);

            } else {

                inputSystem.update(registry, window);
                cameraSystem.update(registry, deltaTime);
                if (windowSystem.update(registry, window)) {
                    inputSystem.resetMouseTracking(window);
                }
                pathFindingSystem.update(registry);

                if (keyPressedOnce(window, GLFW_KEY_F10)) {
                    usePbrShader = !usePbrShader;
                    printf("Mode rendu: %s\n", usePbrShader ? "PBR" : "BASIC");
                }

                if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F9)) {
                    debugTBN = !debugTBN;
                    printf("Debug TBN: %s\n", debugTBN ? "ON" : "OFF");
                }

                if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F8)) {
                    useNormalMap = !useNormalMap;
                    printf("Normal map: %s\n", useNormalMap ? "ON" : "OFF");
                }

                if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F7)) {
                    debugDiffuseOnly = !debugDiffuseOnly;
                    printf("Diffuse only: %s\n", debugDiffuseOnly ? "ON" : "OFF");
                }

                if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F6)) {
                    useReducedAmbient = !useReducedAmbient;
                    printf("Ambient preset: %s (ambient=%.3f, lightColor=%.2f %.2f %.2f)\n",
                           useReducedAmbient ? "CRISP" : "SOFT",
                           getAmbientStrength(),
                           getLightColor().r, getLightColor().g, getLightColor().b);
                }

                GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
                BlockTextureManager::bindArrays(activeProgramID);

                glUseProgram(activeProgramID);
                applyActiveShaderUniforms(activeProgramID);

                renderSystem.update(registry, activeProgramID, getLightColor());
                
                if (debugWireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                ImGui_ImplOpenGL3_NewFrame();
                debugSystem.update(registry, window, deltaTime);
                
                glDisable(GL_DEPTH_TEST);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glEnable(GL_DEPTH_TEST);
            }
        } else {
            inputSystem.update(registry, window);
            cameraSystem.update(registry, deltaTime);
            if (windowSystem.update(registry, window)) {
                inputSystem.resetMouseTracking(window);
            }

            if (keyPressedOnce(window, GLFW_KEY_F10)) {
                usePbrShader = !usePbrShader;
                printf("Mode rendu: %s\n", usePbrShader ? "PBR" : "BASIC");
            }

            if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F9)) {
                debugTBN = !debugTBN;
                printf("Debug TBN: %s\n", debugTBN ? "ON" : "OFF");
            }

            if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F8)) {
                useNormalMap = !useNormalMap;
                printf("Normal map: %s\n", useNormalMap ? "ON" : "OFF");
            }

            if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F7)) {
                debugDiffuseOnly = !debugDiffuseOnly;
                printf("Diffuse only: %s\n", debugDiffuseOnly ? "ON" : "OFF");
            }

            if (usePbrShader && keyPressedOnce(window, GLFW_KEY_F6)) {
                useReducedAmbient = !useReducedAmbient;
                printf("Ambient preset: %s (ambient=%.3f, lightColor=%.2f %.2f %.2f)\n",
                       useReducedAmbient ? "CRISP" : "SOFT",
                       getAmbientStrength(),
                       getLightColor().r, getLightColor().g, getLightColor().b);
            }

            GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
            BlockTextureManager::bindArrays(activeProgramID);

            glUseProgram(activeProgramID);
            applyActiveShaderUniforms(activeProgramID);

            renderSystem.update(registry, activeProgramID, getLightColor());
            
            if (debugWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            ImGui_ImplOpenGL3_NewFrame();
            debugSystem.update(registry, window, deltaTime);
            
            glDisable(GL_DEPTH_TEST);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    } 
    while( (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0) );

    isGameRunning = false;
    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    // Cleanup
    glDeleteProgram(basicProgramID);
    glDeleteProgram(pbrProgramID);
    glDeleteVertexArrays(1, &VertexArrayID);
    BlockTextureManager::cleanup();

    glfwTerminate();
    return 0;
}

// Resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void MeshingWorkerThread(Registry& registry, TerrainSystem& terrain, ChunkMeshingSystem& meshing) {
    while (isGameRunning) { 
        EntityID targetID;
        
        if (!terrain.popMeshingTask(targetID)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        std::vector<SubChunkComponent> threadLocalChunks;
        SubChunkComponent centerChunkCopy; 

        {
            std::lock_guard<std::mutex> ecsLock(ecsMutex);
            
            if (!registry.hasComponent<SubChunkComponent>(targetID)) continue;
            
            centerChunkCopy = registry.getComponent<SubChunkComponent>(targetID);
            
            if (centerChunkCopy.solidBlockCount == 0) {
                std::lock_guard<std::mutex> uploadLock(meshing.uploadMutex);
                meshing.uploadQueue.push({targetID, MeshData()});
                continue;
            }
            
            glm::ivec3 pos = centerChunkCopy.subChunkPosition;
            glm::ivec3 neededPos[6] = {
                pos + glm::ivec3(1,0,0), pos + glm::ivec3(-1,0,0),
                pos + glm::ivec3(0,1,0), pos + glm::ivec3(0,-1,0),
                pos + glm::ivec3(0,0,1), pos + glm::ivec3(0,0,-1)
            };

            for (int i=0; i<6; ++i) {
                EntityID nID = terrain.getSubChunkAt(neededPos[i].x, neededPos[i].y, neededPos[i].z, registry);
                if (nID != 0 && registry.hasComponent<SubChunkComponent>(nID)) {
                    auto& neighbor = registry.getComponent<SubChunkComponent>(nID);
                    if (neighbor.solidBlockCount > 0) {
                        threadLocalChunks.push_back(neighbor);
                    }
                }
            }
        }

        subChunkCache localCache;
        localCache.push_back(&centerChunkCopy);
        for (auto& comp : threadLocalChunks) {
            localCache.push_back(&comp);
        }
        
        std::sort(localCache.begin(), localCache.end(), [](const SubChunkComponent* a, const SubChunkComponent* b) {
            if (a->subChunkPosition.x != b->subChunkPosition.x) return a->subChunkPosition.x < b->subChunkPosition.x;
            if (a->subChunkPosition.y != b->subChunkPosition.y) return a->subChunkPosition.y < b->subChunkPosition.y;
            return a->subChunkPosition.z < b->subChunkPosition.z;
        });

        MeshData generatedData = meshing.calculateMeshData(registry, targetID, centerChunkCopy, localCache);

        std::lock_guard<std::mutex> uploadLock(meshing.uploadMutex);
        meshing.uploadQueue.push({targetID, std::move(generatedData)});
    }
}