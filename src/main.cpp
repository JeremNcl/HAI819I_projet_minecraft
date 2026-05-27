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
#include "ecs/systems/monsterInteractionSystem.hpp"
#include "ecs/systems/monsterMovementSystem.hpp"

//void processInput(GLFWwindow *window, Camera& camera);
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
bool useFrustumCulling = true;
bool useOcclusionCulling;

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

static float inverseLerp(float a, float b, float v) {
    return glm::clamp((v - a) / (b - a), 0.0f, 1.0f);
}

// Fonction utilitaire pour synchroniser parfaitement l'angle physique du soleil avec les couleurs de la skybox
static float getSunAngle(float t) {
    float sunrise = 0.20f; // Heure exacte du lever
    float sunset = 0.80f;  // Heure exacte du coucher
    float dayDuration = sunset - sunrise;
    float nightDuration = 1.0f - dayDuration;

    if (t >= sunrise && t <= sunset) {
        // Le jour, l'angle va de 0 à PI
        float t_mapped = (t - sunrise) / dayDuration;
        return t_mapped * glm::pi<float>();
    } else {
        // La nuit, l'angle va de PI à 2*PI
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
    // Synchronize TimeComponent to globals
    auto timeView = registry.view<TimeComponent>();
    if (!timeView.isEmpty()) {
        EntityID timeEntity = *timeView.begin();
        TimeComponent& timeComp = registry.getComponent<TimeComponent>(timeEntity);
        dayTime = timeComp.dayTime;
        daySpeed = timeComp.daySpeed;
        dayPaused = timeComp.paused;
        useReducedAmbient = (timeComp.ambientPreset == TimeComponent::AmbientPreset::CRISP);
    }
    
    // Synchronize LightingStateComponent to globals
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
    // Read from ECS components if available
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
    // Get lighting state from ECS
    LightingStateComponent lightingState = getLightingState(registry);
    
    GLint debugLoc = glGetUniformLocation(activeProgramID, "debugTBN");
    if (debugLoc >= 0) {
        glUniform1i(debugLoc, lightingState.debugTBN ? 1 : 0);
    }

    GLint useNormalMapLoc = glGetUniformLocation(activeProgramID, "useNormalMap");
    if (useNormalMapLoc >= 0) {
        glUniform1i(useNormalMapLoc, lightingState.useNormalMap ? 1 : 0);
    }

    GLint debugDiffuseOnlyLoc = glGetUniformLocation(activeProgramID, "debugDiffuseOnly");
    if (debugDiffuseOnlyLoc >= 0) {
        glUniform1i(debugDiffuseOnlyLoc, lightingState.debugDiffuseOnly ? 1 : 0);
    }

    GLint hemiAmbientLoc = glGetUniformLocation(activeProgramID, "useHemisphericalAmbient");
    if (hemiAmbientLoc >= 0) {
        glUniform1i(hemiAmbientLoc, lightingState.useHemisphericalAmbient ? 1 : 0);
    }

    GLint bakedAOLoc = glGetUniformLocation(activeProgramID, "useBakedAO");
    if (bakedAOLoc >= 0) {
        glUniform1i(bakedAOLoc, lightingState.useBakedAO ? 1 : 0);
    }
    GLint aoStrengthLoc = glGetUniformLocation(activeProgramID, "aoStrength");
    if (aoStrengthLoc >= 0) {
        glUniform1f(aoStrengthLoc, lightingState.aoStrength);
    }

    GLint ambientStrengthLoc = glGetUniformLocation(activeProgramID, "ambientStrength");
    if (ambientStrengthLoc >= 0) {
        glUniform1f(ambientStrengthLoc, lightingState.ambientStrength);
    }

    GLint ambientSkyLoc = glGetUniformLocation(activeProgramID, "ambientSkyColor");
    if (ambientSkyLoc >= 0) {
        glUniform3fv(ambientSkyLoc, 1, glm::value_ptr(lightingState.ambientSkyColor));
    }

    GLint ambientGroundLoc = glGetUniformLocation(activeProgramID, "ambientGroundColor");
    if (ambientGroundLoc >= 0) {
        glUniform3fv(ambientGroundLoc, 1, glm::value_ptr(lightingState.ambientGroundColor));
    }

    GLint exposureLoc = glGetUniformLocation(activeProgramID, "exposure");
    if (exposureLoc >= 0) {
        glUniform1f(exposureLoc, lightingState.exposure);
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
    


    auto deltaTimeView = registry.view<DeltaTimeComponent>();
    if (deltaTimeView.isEmpty()) {
        EntityID deltaTimeEntity = registry.createEntity();
        registry.addComponent(deltaTimeEntity, DeltaTimeComponent());
        deltaTimeView = registry.view<DeltaTimeComponent>();
    }
    DeltaTimeComponent& deltaTimeComponent = registry.getComponent<DeltaTimeComponent>(*deltaTimeView.begin());

    // Créer les systèmes
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
    MonsterInteractionSystem monsterInteractionSystem;
    MonsterMovementSystem monsterMovementSystem;

    // Systèmes terrain et pathfinding
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
        glm::vec3(50,100,50),
        glm::vec3(0,0,0)
    });
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
    registry.addComponent(spectatorCamera, TransformComponent{
        glm::vec3(50,100,50),
        glm::vec3(0,0,0)
    });
    registry.addComponent(spectatorCamera, InputReceiverComponent{});
    registry.addComponent(spectatorCamera, VelocityComponent{ .movementSpeed = 6.f}); //définit la speed camSpec ici si besoin

    //positionCameraForScene(registry, spectatorCamera, selectedScene);
    cameraSystem.initCamera(registry, spectatorCamera, 90, 0, glm::vec3(0,0,0));

    // Create skybox entity
    EntityID skyboxEntity = registry.createEntity();
    skyboxSystem.initialize();
    registry.addComponent(skyboxEntity, SkyboxComponent{ .VAO = 0, .VBO = 0, .EBO = 0, .indexCount = 36 });
    registry.addComponent(skyboxEntity, TransformComponent{
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 0)
    });
    skyboxSystem.initializeSkyboxGeometry(registry, skyboxEntity);

    // Create TimeManager entity
    EntityID timeManagerEntity = registry.createEntity();
    TimeComponent timeComp;
    timeComp.dayTime = dayTime;
    timeComp.daySpeed = daySpeed;
    timeComp.paused = dayPaused;
    timeComp.ambientPreset = useReducedAmbient ? TimeComponent::AmbientPreset::CRISP : TimeComponent::AmbientPreset::SOFT;
    registry.addComponent(timeManagerEntity, timeComp);

    // Create LightingManager entity
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
    const int TARGET_CHUNKS = useInfiniteTerrain ? 9 * 9 : 0; // (Rayon  * 2 + 1)^2 rayon = 14

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
        // Calcul du deltaTime

        deltaTimeSystem.update(deltaTimeComponent);
        float deltaTime = deltaTimeComponent.deltaTime;

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
                debugInputSystem.update(registry, window, deltaTime);

                ImGui_ImplOpenGL3_NewFrame();
                debugSystem.renderLoadingScreen(window, currentMeshesReady, totalExpectedMeshes);

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
                monsterInteractionSystem.update(registry, deltaTime);
                movementSystem.update(registry, deltaTime);
                monsterMovementSystem.update(registry, deltaTime);
                physicsSystem.update(registry, deltaTime);
                collisionSystem.update(registry, deltaTime);
                interactionSystem.update(registry, terrainSystem);
                
                // Handle debug inputs (day/night cycle, render toggles)
                debugInputSystem.update(registry, window, deltaTime);

                // Update time system (day/night cycle) (HEAD)
                timeSystem.update(registry, deltaTime);
                
                // Calculate lighting state from time (HEAD)
                lightingSystem.update(registry);
                
                // Sync components to globals for backward compatibility (HEAD)
                syncComponentsToGlobals(registry);

                GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
                BlockTextureManager::bindArrays(activeProgramID);

                glUseProgram(activeProgramID);
                applyActiveShaderUniforms(activeProgramID, registry);

                // Toggle Wireframe JUSTE pour le terrain
                if (debugSystem.isWireframe()) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }

                // Render terrain (Un seul appel suffit !)
                renderSystem.update(registry, activeProgramID);
                
                // Reset Fill mode pour la skybox
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                // Render skybox
                skyboxSystem.update(registry, getRenderDebugState(registry));

                ImGui_ImplOpenGL3_NewFrame();
                debugSystem.update(registry, window, deltaTime, getRenderDebugState(registry));
                
                glDisable(GL_DEPTH_TEST);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glEnable(GL_DEPTH_TEST);
            }
        } else {
            inputSystem.update(registry, window);
            if (windowSystem.update(registry, window)) {
                inputSystem.resetMouseTracking(window);
            }   

            pathFindingSystem.update(registry);
            monsterInteractionSystem.update(registry, deltaTime);
            movementSystem.update(registry, deltaTime);
            monsterMovementSystem.update(registry, deltaTime);
            physicsSystem.update(registry, deltaTime);
            collisionSystem.update(registry, deltaTime);
            interactionSystem.update(registry, terrainSystem);
            cameraSystem.update(registry, deltaTime); // (Seulement dans le else, comme dans leur code)

            // Handle debug inputs (day/night cycle, render toggles)
            debugInputSystem.update(registry, window, deltaTime);

            // Update time system (day/night cycle) (HEAD)
            timeSystem.update(registry, deltaTime);
            
            // Calculate lighting state from time (HEAD)
            lightingSystem.update(registry);
            
            // Sync components to globals for backward compatibility (HEAD)
            syncComponentsToGlobals(registry);

            GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
            BlockTextureManager::bindArrays(activeProgramID);

            glUseProgram(activeProgramID);
            applyActiveShaderUniforms(activeProgramID, registry);

            // Toggle Wireframe JUSTE pour le terrain
            if (debugSystem.isWireframe()) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            // Render terrain (Un seul appel suffit !)
            renderSystem.update(registry, activeProgramID);
            
            // Reset Fill mode pour la skybox
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            // Render skybox
            skyboxSystem.update(registry, getRenderDebugState(registry));

            ImGui_ImplOpenGL3_NewFrame();
            debugSystem.update(registry, window, deltaTime, getRenderDebugState(registry));
            
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