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
#include <glm/gtc/type_ptr.hpp>
#include <thread>

using namespace glm;

// ImGui
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

// Inclusions de notre moteur (Nouvelle architecture ECS)
#include "engine/render/shader.hpp"
#include "engine/io/textureLoader.hpp"
#include "engine/render/BlockTextureManager.hpp"
#include "modules/terrain_gen/TerrainGenerator.hpp"
#include "modules/pathfinding/PathFinder3D.hpp"
#include "game/testScenes.hpp"

// ECS Includes
#include "ecs/registry.hpp"
#include "ecs/components/inventory.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/mesh.hpp"
#include "ecs/components/chunk.hpp"
#include "ecs/components/camera.hpp"
#include "ecs/components/inputReceiver.hpp"
#include "ecs/components/skyboxComponent.hpp"
#include "ecs/components/timeComponent.hpp"
#include "ecs/components/lightingStateComponent.hpp"
#include "ecs/components/deltaTime.hpp"
#include "ecs/systems/chunkMeshingSystem.hpp"
#include "ecs/systems/renderSystem.hpp"
#include "ecs/systems/inputSystem.hpp"
#include "ecs/systems/cameraSystem.hpp"
#include "ecs/systems/windowSystem.hpp"
#include "ecs/systems/debugSystem.hpp"
#include "ecs/systems/debugInputSystem.hpp"
#include "ecs/systems/skyboxSystem.hpp"
#include "ecs/systems/timeSystem.hpp"
#include "ecs/systems/lightingCalculationSystem.hpp"
#include "ecs/systems/PathFindingSystem.hpp"
#include "ecs/systems/TerrainSystem.hpp"
#include "ecs/systems/playerMovementSystem.hpp"
#include "ecs/systems/physicsSystem.hpp"
#include "ecs/systems/collisionSystem.hpp"
#include "ecs/systems/deltaTimeSystem.hpp"
#include "ecs/systems/playerInteractionSystem.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void MeshingWorkerThread(Registry& registry, TerrainSystem& terrain, ChunkMeshingSystem& meshing);

// Debug flags PBR
bool debugWireframe = false;
bool usePbrShader = true;
bool debugTBN = false;
bool useNormalMap = true;
bool debugDiffuseOnly = false;
bool useHemisphericalAmbient = true;
bool useBakedAO = true;
float aoStrength = 0.50f;
bool useReducedAmbient = false;

// Day / Night cycle (managed by DebugInputSystem)
float dayTime = 0.0f;      
float daySpeed = 0.02f;    
bool dayPaused = false;

static constexpr float kAmbientSoft = 0.055f;
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

static float getSunAngle(float t) {
    float sunrise = 0.20f; 
    float sunset = 0.80f;  
    float dayDuration = sunset - sunrise;
    float nightDuration = 1.0f - dayDuration;

    if (t >= sunrise && t <= sunset) {
        float t_mapped = (t - sunrise) / dayDuration;
        return t_mapped * glm::pi<float>();
    } else {
        float t_mapped;
        if (t > sunset) t_mapped = (t - sunset) / nightDuration;
        else t_mapped = (t + 1.0f - sunset) / nightDuration;
        return glm::pi<float>() + t_mapped * glm::pi<float>();
    }
}

static LightingStateComponent getLightingState(Registry& registry) {
    auto lightingView = registry.view<LightingStateComponent>();
    if (!lightingView.isEmpty()) {
        EntityID lightingEntity = *lightingView.begin();
        return registry.getComponent<LightingStateComponent>(lightingEntity);
    }
    return LightingStateComponent{};
}

static void syncComponentsToGlobals(Registry& registry) {
    auto timeView = registry.view<TimeComponent>();
    if (!timeView.isEmpty()) {
        EntityID timeEntity = *timeView.begin();
        TimeComponent& timeComp = registry.getComponent<TimeComponent>(timeEntity);
        dayTime = timeComp.dayTime;
        daySpeed = timeComp.daySpeed;
        dayPaused = timeComp.paused;
        useReducedAmbient = (timeComp.ambientPreset == TimeComponent::AmbientPreset::CRISP);
    }
    
    LightingStateComponent lightingComp = getLightingState(registry);
    debugTBN = lightingComp.debugTBN;
    useNormalMap = lightingComp.useNormalMap;
    debugDiffuseOnly = lightingComp.debugDiffuseOnly;
    useBakedAO = lightingComp.useBakedAO;
    useHemisphericalAmbient = lightingComp.useHemisphericalAmbient;
    useReducedAmbient = lightingComp.useReducedAmbient;
    aoStrength = lightingComp.aoStrength;
}

static RenderDebugState getRenderDebugState(Registry& registry) {
    auto lightingView = registry.view<LightingStateComponent>();
    auto timeView = registry.view<TimeComponent>();
    
    LightingStateComponent lightingState;
    TimeComponent timeState;
    
    if (!lightingView.isEmpty()) {
        EntityID lightingEntity = *lightingView.begin();
        lightingState = registry.getComponent<LightingStateComponent>(lightingEntity);
    }
    if (!timeView.isEmpty()) {
        EntityID timeEntity = *timeView.begin();
        timeState = registry.getComponent<TimeComponent>(timeEntity);
    }
    
    return RenderDebugState{
        .usePbrShader = usePbrShader,
        .debugTBN = lightingState.debugTBN,
        .useNormalMap = lightingState.useNormalMap,
        .debugDiffuseOnly = lightingState.debugDiffuseOnly,
        .useBakedAO = lightingState.useBakedAO,
        .useHemisphericalAmbient = lightingState.useHemisphericalAmbient,
        .useReducedAmbient = lightingState.useReducedAmbient,
        .ambientStrength = lightingState.ambientStrength,
        .aoStrength = lightingState.aoStrength,
        .exposure = lightingState.exposure,
        .lightColor = lightingState.lightColor,
        .ambientSkyColor = lightingState.ambientSkyColor,
        .ambientGroundColor = lightingState.ambientGroundColor,
        .horizonColor = lightingState.horizonColor,
        .dayTime = timeState.dayTime,
        .daySpeed = timeState.daySpeed,
        .dayPaused = timeState.paused
    };
}

static void applyActiveShaderUniforms(GLuint activeProgramID, Registry& registry) {
    LightingStateComponent lightingState = getLightingState(registry);
    
    GLint debugLoc = glGetUniformLocation(activeProgramID, "debugTBN");
    if (debugLoc >= 0) glUniform1i(debugLoc, lightingState.debugTBN ? 1 : 0);

    GLint useNormalMapLoc = glGetUniformLocation(activeProgramID, "useNormalMap");
    if (useNormalMapLoc >= 0) glUniform1i(useNormalMapLoc, lightingState.useNormalMap ? 1 : 0);

    GLint debugDiffuseOnlyLoc = glGetUniformLocation(activeProgramID, "debugDiffuseOnly");
    if (debugDiffuseOnlyLoc >= 0) glUniform1i(debugDiffuseOnlyLoc, lightingState.debugDiffuseOnly ? 1 : 0);

    GLint hemiAmbientLoc = glGetUniformLocation(activeProgramID, "useHemisphericalAmbient");
    if (hemiAmbientLoc >= 0) glUniform1i(hemiAmbientLoc, lightingState.useHemisphericalAmbient ? 1 : 0);

    GLint bakedAOLoc = glGetUniformLocation(activeProgramID, "useBakedAO");
    if (bakedAOLoc >= 0) glUniform1i(bakedAOLoc, lightingState.useBakedAO ? 1 : 0);
    
    GLint aoStrengthLoc = glGetUniformLocation(activeProgramID, "aoStrength");
    if (aoStrengthLoc >= 0) glUniform1f(aoStrengthLoc, lightingState.aoStrength);

    GLint ambientStrengthLoc = glGetUniformLocation(activeProgramID, "ambientStrength");
    if (ambientStrengthLoc >= 0) glUniform1f(ambientStrengthLoc, lightingState.ambientStrength);

    GLint ambientSkyLoc = glGetUniformLocation(activeProgramID, "ambientSkyColor");
    if (ambientSkyLoc >= 0) glUniform3fv(ambientSkyLoc, 1, glm::value_ptr(lightingState.ambientSkyColor));

    GLint ambientGroundLoc = glGetUniformLocation(activeProgramID, "ambientGroundColor");
    if (ambientGroundLoc >= 0) glUniform3fv(ambientGroundLoc, 1, glm::value_ptr(lightingState.ambientGroundColor));

    GLint exposureLoc = glGetUniformLocation(activeProgramID, "exposure");
    if (exposureLoc >= 0) glUniform1f(exposureLoc, lightingState.exposure);
}

std::mutex ecsMutex;
std::atomic<bool> isGameRunning{true};

/*******************************************************************************/

int main(int argc, char** argv) {
    if( !glfwInit() ) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow( 1024, 720, "Voxel Engine - Prototype", NULL, NULL);
    if( window == NULL ){
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = true; 
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 720/2);

    glClearColor(0.39f, 0.65f, 0.85f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint basicProgramID = LoadShaders("assets/shaders/vertex_shader.glsl", "assets/shaders/fragment_shader.glsl");
    GLuint pbrProgramID = LoadShaders("assets/shaders/pbr_vertex.glsl", "assets/shaders/pbr_fragment.glsl");
    
    BlockTextureManager::initialize();
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "build/imgui.ini";
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    
    Registry registry;

    auto deltaTimeView = registry.view<DeltaTimeComponent>();
    if (deltaTimeView.isEmpty()) {
        EntityID deltaTimeEntity = registry.createEntity();
        registry.addComponent(deltaTimeEntity, DeltaTimeComponent());
        deltaTimeView = registry.view<DeltaTimeComponent>();
    }
    DeltaTimeComponent& deltaTimeComponent = registry.getComponent<DeltaTimeComponent>(*deltaTimeView.begin());

    DeltaTimeSystem deltaTimeSystem;
    ChunkMeshingSystem meshingSystem;
    RenderSystem renderSystem;
    InputSystem inputSystem(window);
    WindowSystem windowSystem;
    CameraSystem cameraSystem;
    DebugSystem debugSystem;
    DebugInputSystem debugInputSystem;
    SkyboxSystem skyboxSystem;
    TimeSystem timeSystem;
    LightingCalculationSystem lightingSystem;
    PlayerMovementSystem movementSystem;
    PhysicsSystem physicsSystem;
    CollisionSystem collisionSystem;
    PlayerInteractionSystem interactionSystem;

    TerrainConfig config = LoadConfig("config.txt");
    TerrainSystem terrainSystem(config);
    PathFindingSystem pathFindingSystem;

    const TestSceneMode selectedScene = parseSceneMode(argc, argv);
    const bool useInfiniteTerrain = (selectedScene == TestSceneMode::InfiniteTerrain);

    EntityID camEntity = registry.createEntity();
    registry.addComponent(camEntity, TransformComponent{
        glm::vec3(50,100,50),
        glm::vec3(0,0,0)
    });

    // === MODIFICATION : BLOCS DE DEPART POUR L'INVENTAIRE ===
    InventoryComponent initialInventory;
    initialInventory.items[VoxelType::GRASS] = 64;
    initialInventory.items[VoxelType::DIRT] = 64;
    initialInventory.items[VoxelType::STONE] = 64;
    initialInventory.items[VoxelType::WOOD] = 64;
    initialInventory.items[VoxelType::DIAMOND] = 5;
    registry.addComponent(camEntity, initialInventory);
    
    registry.addComponent(camEntity, CameraComponent{ .isActive = true});
    registry.addComponent(camEntity, InputReceiverComponent{});
    registry.addComponent(camEntity, RigidBodyComponent{});
    registry.addComponent(camEntity, VelocityComponent{ .movementSpeed = 10.f});
    registry.addComponent(camEntity, ColliderComponent{
        glm::vec3(.6f, 1.8f, .6f),
        glm::vec3(0.f, .9f, 0.f)
    });
    registry.addComponent(camEntity, PlayerComponent{});

    cameraSystem.initCamera(registry, camEntity, 90, 0, glm::vec3(0,1.8,0));
    
    EntityID spectatorCamera = registry.createEntity();
    registry.addComponent(spectatorCamera, CameraComponent{});
    registry.addComponent(spectatorCamera, TransformComponent{ glm::vec3(50,100,50), glm::vec3(0,0,0) });
    registry.addComponent(spectatorCamera, InputReceiverComponent{});
    registry.addComponent(spectatorCamera, VelocityComponent{ .movementSpeed = 6.f});
    cameraSystem.initCamera(registry, spectatorCamera, 90, 0, glm::vec3(0,0,0));

    EntityID skyboxEntity = registry.createEntity();
    skyboxSystem.initialize();
    registry.addComponent(skyboxEntity, SkyboxComponent{ .VAO = 0, .VBO = 0, .EBO = 0, .indexCount = 36 });
    registry.addComponent(skyboxEntity, TransformComponent{ glm::vec3(0), glm::vec3(0) });
    skyboxSystem.initializeSkyboxGeometry(registry, skyboxEntity);

    EntityID timeManagerEntity = registry.createEntity();
    TimeComponent timeComp;
    timeComp.dayTime = dayTime;
    timeComp.daySpeed = daySpeed;
    timeComp.paused = dayPaused;
    timeComp.ambientPreset = useReducedAmbient ? TimeComponent::AmbientPreset::CRISP : TimeComponent::AmbientPreset::SOFT;
    registry.addComponent(timeManagerEntity, timeComp);

    EntityID lightingManagerEntity = registry.createEntity();
    LightingStateComponent lightingComp;
    lightingComp.useHemisphericalAmbient = useHemisphericalAmbient;
    lightingComp.useBakedAO = useBakedAO;
    lightingComp.useReducedAmbient = useReducedAmbient;
    lightingComp.aoStrength = aoStrength;
    lightingComp.debugTBN = debugTBN;
    lightingComp.useNormalMap = useNormalMap;
    lightingComp.debugDiffuseOnly = debugDiffuseOnly;
    registry.addComponent(lightingManagerEntity, lightingComp);

    if (selectedScene == TestSceneMode::SimpleSubChunk) {
        TestScenes::createSimpleChunk(registry);
        buildStaticSceneMeshes(registry, meshingSystem);
    } else if (selectedScene == TestSceneMode::GeneratedChunk) {
        TestScenes::createGeneratedChunk(registry);
        buildStaticSceneMeshes(registry, meshingSystem);
    } else {
        TestScenes::createInfiniteTerrainScene(registry);
    }

    inputSystem.setCursorMode(window, true);

    bool isLoading = useInfiniteTerrain;
    const int TARGET_CHUNKS = useInfiniteTerrain ? 9 * 9 : 0; 

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; 

    std::vector<std::thread> workers;
    if (useInfiniteTerrain) {
        for (unsigned int i = 0; i < numThreads; ++i) {
            workers.emplace_back(MeshingWorkerThread, std::ref(registry), std::ref(terrainSystem), std::ref(meshingSystem));
        }
    }

    do {
        deltaTimeSystem.update(deltaTimeComponent);
        float deltaTime = deltaTimeComponent.deltaTime;

        glfwPollEvents();
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
                if (currentMeshesReady >= totalExpectedMeshes) isLoading = false;
            }

            if (isLoading) {
                if (windowSystem.update(registry, window)) inputSystem.resetMouseTracking(window);
                debugInputSystem.update(registry, window, deltaTime);

                ImGui_ImplOpenGL3_NewFrame();
                debugSystem.renderLoadingScreen(window, currentMeshesReady, totalExpectedMeshes);

                glDisable(GL_DEPTH_TEST);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glEnable(GL_DEPTH_TEST);

            } else {
                // === MODIFICATION : GESTION SOURIS ET BLOCAGE INPUTS (TERRAIN INFINI) ===
                auto& playerInv = registry.getComponent<InventoryComponent>(camEntity);
                static bool wasUIOpen = false;

                if (playerInv.isOpen != wasUIOpen) {
                    if (playerInv.isOpen) {
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    } else {
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        inputSystem.resetMouseTracking(window);
                    }
                    wasUIOpen = playerInv.isOpen;
                }

                if (!playerInv.isOpen) {
                    inputSystem.update(registry, window);
                    cameraSystem.update(registry, deltaTime);
                    interactionSystem.update(registry, terrainSystem);
                    if (windowSystem.update(registry, window)) inputSystem.resetMouseTracking(window);
                } else {
                    auto& input = registry.getComponent<InputReceiverComponent>(camEntity);
                    input.moveForward = false; input.moveBackward = false;
                    input.moveLeft = false;    input.moveRight = false;
                    input.jump = false;        input.leftClick = false; input.rightClick = false;
                }

                movementSystem.update(registry, deltaTime);
                physicsSystem.update(registry, deltaTime);
                collisionSystem.update(registry, deltaTime);
                pathFindingSystem.update(registry);
                debugInputSystem.update(registry, window, deltaTime);
                timeSystem.update(registry, deltaTime);
                lightingSystem.update(registry);
                syncComponentsToGlobals(registry);

                GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
                BlockTextureManager::bindArrays(activeProgramID);
                glUseProgram(activeProgramID);
                applyActiveShaderUniforms(activeProgramID, registry);

                if (debugSystem.isWireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                renderSystem.update(registry, activeProgramID);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                skyboxSystem.update(registry, getRenderDebugState(registry));

                // === MODIFICATION : REPARATION CYCLE DE RENDU IMGUI ===
                ImGui_ImplOpenGL3_NewFrame();
                ImGui::NewFrame(); 

                debugSystem.update(registry, window, deltaTime, getRenderDebugState(registry));
                debugSystem.renderInventoryUI(registry, camEntity, window); 

                ImGui::Render(); 
                
                glDisable(GL_DEPTH_TEST);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glEnable(GL_DEPTH_TEST);
            }
        } else {
            // === MODIFICATION : GESTION SOURIS ET BLOCAGE INPUTS (SCENE STATIQUE) ===
            auto& playerInv = registry.getComponent<InventoryComponent>(camEntity);
            static bool wasUIOpen = false;

            if (playerInv.isOpen != wasUIOpen) {
                if (playerInv.isOpen) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    inputSystem.resetMouseTracking(window);
                }
                wasUIOpen = playerInv.isOpen;
            }

            if (!playerInv.isOpen) {
                inputSystem.update(registry, window);
                cameraSystem.update(registry, deltaTime);
                interactionSystem.update(registry, terrainSystem);
                if (windowSystem.update(registry, window)) inputSystem.resetMouseTracking(window);
            } else {
                auto& input = registry.getComponent<InputReceiverComponent>(camEntity);
                input.moveForward = false; input.moveBackward = false;
                input.moveLeft = false;    input.moveRight = false;
                input.jump = false;        input.leftClick = false; input.rightClick = false;
            }

            movementSystem.update(registry, deltaTime);
            physicsSystem.update(registry, deltaTime);
            collisionSystem.update(registry, deltaTime);
            pathFindingSystem.update(registry);
            debugInputSystem.update(registry, window, deltaTime);
            timeSystem.update(registry, deltaTime);
            lightingSystem.update(registry);
            syncComponentsToGlobals(registry);

            GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
            BlockTextureManager::bindArrays(activeProgramID);
            glUseProgram(activeProgramID);
            applyActiveShaderUniforms(activeProgramID, registry);

            if (debugSystem.isWireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderSystem.update(registry, activeProgramID);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            skyboxSystem.update(registry, getRenderDebugState(registry));

            // === REPARATION CYCLE DE RENDU IMGUI (SCENE STATIQUE) ===
            ImGui_ImplOpenGL3_NewFrame();
            ImGui::NewFrame();

            debugSystem.update(registry, window, deltaTime, getRenderDebugState(registry));
            debugSystem.renderInventoryUI(registry, camEntity, window); 

            ImGui::Render();
            
            glDisable(GL_DEPTH_TEST);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
    } 
    while( (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0) );

    isGameRunning = false;
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(basicProgramID);
    glDeleteProgram(pbrProgramID);
    glDeleteVertexArrays(1, &VertexArrayID);
    BlockTextureManager::cleanup();

    glfwTerminate();
    return 0;
}

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
                    if (neighbor.solidBlockCount > 0) threadLocalChunks.push_back(neighbor);
                }
            }
        }

        subChunkCache localCache;
        localCache.push_back(&centerChunkCopy);
        for (auto& comp : threadLocalChunks) localCache.push_back(&comp);
        
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