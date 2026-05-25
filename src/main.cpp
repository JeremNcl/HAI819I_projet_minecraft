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
#include "ecs/systems/debugInputSystem.hpp"
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
bool useHemisphericalAmbient = true;
bool useBakedAO = true;
float aoStrength = 0.50f; // default blend between 1.0 and baked AO
bool useReducedAmbient = false;

// Day / Night cycle (managed by DebugInputSystem)
float dayTime = 0.0f;      // normalized [0,1]
float daySpeed = 0.02f;    // units per second (fraction of day per second)
bool dayPaused = false;

// Ambient presets: SOFT gives higher ambient to reduce overall contrast
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


// Derive day/night cycle parameters from normalized dayTime [0, 1]
// dayTime: 0.0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = night, 1.0 = end of day

static float computeAmbientStrengthFromTime(float t) {
    // At night (t ~ 0.75): ~0.10 (darker)
    // At day  (t ~ 0.25): base preset value
    float nightDarkness = 0.10f;
    float dayValue = useReducedAmbient ? kAmbientCrisp : kAmbientSoft;
    
    // Smoothly interpolate based on a sine curve (night is roughly t in [0.6, 1.0] and [0, 0.1])
    float sunHeight = glm::sin(glm::pi<float>() * t); // 0 at t=0/1, 1 at t=0.5
    return glm::mix(nightDarkness, dayValue, glm::clamp(sunHeight, 0.0f, 1.0f));
}

static glm::vec3 computeLightColorFromTime(float t) {
    // Warm orange at sunrise/sunset, white at noon, dim blue at night
    float sunHeight = glm::sin(glm::pi<float>() * t);
    
    // Sunset/sunrise colors (orange/red)
    glm::vec3 sunsetColor = glm::vec3(3.0f, 2.2f, 1.4f);
    // Noon color (slightly warm white)
    glm::vec3 noonColor = useReducedAmbient 
        ? glm::vec3(3.00f, 2.94f, 2.86f)
        : glm::vec3(2.40f, 2.35f, 2.30f);
    // Night color (very dim, bluish)
    glm::vec3 nightColor = glm::vec3(0.1f, 0.15f, 0.25f);
    
    glm::vec3 color;
    if (t < 0.25f) {
        // Sunrise: night -> sunset
        float fade = t / 0.25f;
        color = glm::mix(nightColor, sunsetColor, fade);
    } else if (t < 0.5f) {
        // Morning to noon: sunset -> noon
        float fade = (t - 0.25f) / 0.25f;
        color = glm::mix(sunsetColor, noonColor, fade);
    } else if (t < 0.75f) {
        // Afternoon to sunset: noon -> sunset
        float fade = (t - 0.5f) / 0.25f;
        color = glm::mix(noonColor, sunsetColor, fade);
    } else {
        // Night: sunset -> night
        float fade = (t - 0.75f) / 0.25f;
        color = glm::mix(sunsetColor, nightColor, fade);
    }
    
    // Modulate intensity by sun height (darker when below horizon)
    color *= (0.3f + 0.7f * glm::clamp(sunHeight, 0.0f, 1.0f));
    
    return color;
}

static glm::vec3 computeAmbientSkyColorFromTime(float t) {
    float sunHeight = glm::sin(glm::pi<float>() * t);
    
    // Day sky: blue
    glm::vec3 daySky = useReducedAmbient ? glm::vec3(0.50f, 0.60f, 0.75f) : glm::vec3(0.60f, 0.68f, 0.85f);
    // Sunset sky: orange/red
    glm::vec3 sunsetSky = glm::vec3(0.8f, 0.5f, 0.3f);
    // Night sky: dark blue/black
    glm::vec3 nightSky = glm::vec3(0.05f, 0.08f, 0.15f);
    
    glm::vec3 color;
    if (t < 0.25f) {
        float fade = t / 0.25f;
        color = glm::mix(nightSky, sunsetSky, fade);
    } else if (t < 0.5f) {
        float fade = (t - 0.25f) / 0.25f;
        color = glm::mix(sunsetSky, daySky, fade);
    } else if (t < 0.75f) {
        float fade = (t - 0.5f) / 0.25f;
        color = glm::mix(daySky, sunsetSky, fade);
    } else {
        float fade = (t - 0.75f) / 0.25f;
        color = glm::mix(sunsetSky, nightSky, fade);
    }
    
    return color;
}

static glm::vec3 computeAmbientGroundColorFromTime(float t) {
    float sunHeight = glm::sin(glm::pi<float>() * t);
    
    // Day ground: brownish
    glm::vec3 dayGround = useReducedAmbient ? glm::vec3(0.08f, 0.07f, 0.05f) : glm::vec3(0.12f, 0.10f, 0.08f);
    // Sunset ground: warm reddish
    glm::vec3 sunsetGround = glm::vec3(0.3f, 0.15f, 0.08f);
    // Night ground: very dark blue
    glm::vec3 nightGround = glm::vec3(0.02f, 0.02f, 0.05f);
    
    glm::vec3 color;
    if (t < 0.25f) {
        float fade = t / 0.25f;
        color = glm::mix(nightGround, sunsetGround, fade);
    } else if (t < 0.5f) {
        float fade = (t - 0.25f) / 0.25f;
        color = glm::mix(sunsetGround, dayGround, fade);
    } else if (t < 0.75f) {
        float fade = (t - 0.5f) / 0.25f;
        color = glm::mix(dayGround, sunsetGround, fade);
    } else {
        float fade = (t - 0.75f) / 0.25f;
        color = glm::mix(sunsetGround, nightGround, fade);
    }
    
    return color;
}

// Compute sun light direction from dayTime
// At t=0: sunrise east, t=0.25: noon overhead, t=0.5: sunset west, t=0.75: night below
static glm::vec3 computeLightDirectionFromTime(float t) {
    float angle = glm::pi<float>() * t; // 0 to 2*pi
    float elevation = glm::sin(angle);  // sun height: -1 (bottom) to 1 (overhead) to -1
    float azimuth = glm::cos(angle);    // horizontal position: 1 (east) to -1 (west)
    
    // Direction vector pointing from world toward sun
    // At night (elevation < 0), keep light below horizon for realistic night
    glm::vec3 direction = glm::normalize(glm::vec3(azimuth, elevation, 0.0f));
    
    // Ensure light direction is never pointing straight down (clamp elevation)
    direction.y = glm::clamp(direction.y, -0.3f, 1.0f);
    
    return glm::normalize(direction);
}


static RenderDebugState getRenderDebugState() {
    return RenderDebugState{
        .usePbrShader = usePbrShader,
        .debugTBN = debugTBN,
        .useNormalMap = useNormalMap,
        .debugDiffuseOnly = debugDiffuseOnly,
        .useBakedAO = useBakedAO,
        .useHemisphericalAmbient = useHemisphericalAmbient,
        .useReducedAmbient = useReducedAmbient,
        .ambientStrength = computeAmbientStrengthFromTime(dayTime),
        .aoStrength = aoStrength,
        .lightColor = computeLightColorFromTime(dayTime),
        .ambientSkyColor = computeAmbientSkyColorFromTime(dayTime),
        .ambientGroundColor = computeAmbientGroundColorFromTime(dayTime),
        .dayTime = dayTime,
        .daySpeed = daySpeed,
        .dayPaused = dayPaused
    };
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

    GLint hemiAmbientLoc = glGetUniformLocation(activeProgramID, "useHemisphericalAmbient");
    if (hemiAmbientLoc >= 0) {
        glUniform1i(hemiAmbientLoc, useHemisphericalAmbient ? 1 : 0);
    }

    GLint bakedAOLoc = glGetUniformLocation(activeProgramID, "useBakedAO");
    if (bakedAOLoc >= 0) {
        glUniform1i(bakedAOLoc, useBakedAO ? 1 : 0);
    }
    GLint aoStrengthLoc = glGetUniformLocation(activeProgramID, "aoStrength");
    if (aoStrengthLoc >= 0) {
        glUniform1f(aoStrengthLoc, aoStrength);
    }

    GLint ambientStrengthLoc = glGetUniformLocation(activeProgramID, "ambientStrength");
    if (ambientStrengthLoc >= 0) {
        glUniform1f(ambientStrengthLoc, computeAmbientStrengthFromTime(dayTime));
    }

    GLint ambientSkyLoc = glGetUniformLocation(activeProgramID, "ambientSkyColor");
    if (ambientSkyLoc >= 0) {
        glm::vec3 sky = computeAmbientSkyColorFromTime(dayTime);
        glUniform3fv(ambientSkyLoc, 1, glm::value_ptr(sky));
    }

    GLint ambientGroundLoc = glGetUniformLocation(activeProgramID, "ambientGroundColor");
    if (ambientGroundLoc >= 0) {
        glm::vec3 ground = computeAmbientGroundColorFromTime(dayTime);
        glUniform3fv(ambientGroundLoc, 1, glm::value_ptr(ground));
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
    // Note: Key callbacks removed - using polling via DebugInputSystem instead
    
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
    DebugInputSystem debugInputSystem;
    
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
    printf("Contrôles: WASD=mouvement XZ, Space/Ctrl=haut/bas, Souris=rotation, F4=AO, F5=hemi ambiant, F6=ambiance\n");
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
    printf("Contrôles: ZQSD=mouvement XZ, Space/Ctrl=haut/bas, Souris=rotation, F4=AO, F5=hemi ambiant, F6=ambiance\n");
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
        

        // Ensure GLFW processes events early so glfwGetKey states are fresh.
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
                if (currentMeshesReady >= totalExpectedMeshes) {
                    isLoading = false;
                }
            }

            if (isLoading) {

                if (windowSystem.update(registry, window)) {
                    inputSystem.resetMouseTracking(window);
                }
                
                // Handle debug inputs (day/night cycle, render toggles)
                debugInputSystem.update(window, deltaTime);

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
                
                // Handle debug inputs (day/night cycle, render toggles)
                debugInputSystem.update(window, deltaTime);

                if (!dayPaused) {
                    dayTime = std::fmod(dayTime + daySpeed * deltaTime, 1.0f);
                    if (dayTime < 0.0f) {
                        dayTime += 1.0f;
                    }
                }

                GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
                BlockTextureManager::bindArrays(activeProgramID);

                glUseProgram(activeProgramID);
                applyActiveShaderUniforms(activeProgramID);

                renderSystem.update(registry, activeProgramID, computeLightColorFromTime(dayTime), computeLightDirectionFromTime(dayTime));
                
                if (debugWireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                ImGui_ImplOpenGL3_NewFrame();
                debugSystem.update(registry, window, deltaTime, getRenderDebugState());
                
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

            // Handle debug inputs (day/night cycle, render toggles, AO adjustments)
            debugInputSystem.update(window, deltaTime);

            // Update day/night cycle
            if (!dayPaused) {
                dayTime = std::fmod(dayTime + daySpeed * deltaTime, 1.0f);
                if (dayTime < 0.0f) {
                    dayTime += 1.0f;
                }
            }

            GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
            BlockTextureManager::bindArrays(activeProgramID);

            glUseProgram(activeProgramID);
            applyActiveShaderUniforms(activeProgramID);

            renderSystem.update(registry, activeProgramID, computeLightColorFromTime(dayTime), computeLightDirectionFromTime(dayTime));
            
            if (debugWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            ImGui_ImplOpenGL3_NewFrame();
            debugSystem.update(registry, window, deltaTime, getRenderDebugState());
            
            glDisable(GL_DEPTH_TEST);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
        // NOTE: glfwPollEvents() already called at frame start (line 579)
        // Calling it again here resets key states and breaks checkKeyEdge tracking

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