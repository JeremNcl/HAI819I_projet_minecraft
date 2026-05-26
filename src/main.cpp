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

static glm::vec3 computeLightColorFromTime(float t) {
    glm::vec3 dayLight    = useReducedAmbient ? glm::vec3(3.0f, 2.9f, 2.8f) : glm::vec3(2.5f, 2.4f, 2.3f);
    glm::vec3 sunsetLight = glm::vec3(2.5f, 1.2f, 0.4f); 
    glm::vec3 duskLight   = glm::vec3(0.8f, 0.2f, 0.2f); 
    glm::vec3 nightLight  = glm::vec3(0.15f, 0.20f, 0.35f); 

    glm::vec3 color;
    // On aligne les couleurs sur le lever (0.20) et coucher (0.80)
    if (t < 0.150f) color = nightLight;
    else if (t < 0.200f) color = glm::mix(nightLight, duskLight, inverseLerp(0.150f, 0.200f, t));
    else if (t < 0.250f) color = glm::mix(duskLight, dayLight, inverseLerp(0.200f, 0.250f, t));
    else if (t < 0.750f) color = dayLight;
    else if (t < 0.800f) color = glm::mix(dayLight, sunsetLight, inverseLerp(0.750f, 0.800f, t));
    else if (t < 0.850f) color = glm::mix(sunsetLight, nightLight, inverseLerp(0.800f, 0.850f, t));
    else color = nightLight;

    // --- L'ASTUCE ANTI-POP ---
    float angle = getSunAngle(t);
    float elevation = glm::sin(angle);
    
    // smoothstep crée un multiplicateur qui vaut 0.0 à l'horizon (elevation = 0) 
    // et monte à 1.0 dès que le soleil s'élève un peu (0.15).
    float horizonFade = glm::smoothstep(0.0f, 0.15f, glm::abs(elevation));
    
    return color * horizonFade;
}

static glm::vec3 computeLightDirectionFromTime(float t) {
    float angle = getSunAngle(t);
    
    float elevation = glm::sin(angle);  
    float azimuth = glm::cos(angle);    
    
    glm::vec3 direction = glm::normalize(glm::vec3(azimuth, elevation, 0.0f));
    
    // L'inversion se fait maintenant dans l'obscurité totale grâce au horizonFade !
    if (elevation < 0.0f) {
        direction = -direction;
    }
    
    direction.y = glm::max(direction.y, 0.05f);
    return glm::normalize(direction);
}

static float computeAmbientStrengthFromTime(float t) {
    float nightDarkness = 0.10f;
    float dayValue = useReducedAmbient ? kAmbientCrisp : kAmbientSoft;
    
    // Nuit
    if (t < 0.175f) return nightDarkness;
    // Aube (Fade in)
    if (t < 0.250f) return glm::mix(nightDarkness, dayValue, inverseLerp(0.175f, 0.250f, t));
    // Jour
    if (t < 0.750f) return dayValue;
    // Crépuscule (Fade out)
    if (t < 0.825f) return glm::mix(dayValue, nightDarkness, inverseLerp(0.750f, 0.825f, t));
    // Nuit
    return nightDarkness;
}

static glm::vec3 computeAmbientSkyColorFromTime(float t) {
    // --- 1. COULEURS (Standards PBR / Rayleigh) ---
    // Bleu Rayleigh (Plein jour : bleu azur réaliste)
    glm::vec3 daySky     = useReducedAmbient ? glm::vec3(0.15f, 0.35f, 0.75f) : glm::vec3(0.25f, 0.45f, 0.85f); 
    // Golden Hour (Soleil rasant, lumière douce)
    glm::vec3 goldenHour = glm::vec3(0.85f, 0.60f, 0.30f); 
    // Coucher de soleil (Orange soutenu par la dispersion)
    glm::vec3 sunset     = glm::vec3(0.90f, 0.35f, 0.15f); 
    // Crépuscule/Aube (Rouge profond avant la nuit noire)
    glm::vec3 redDusk    = glm::vec3(0.55f, 0.10f, 0.15f); 
    // Nuit claire (PBR : pas totalement noir, diffusion stellaire/lunaire)
    glm::vec3 nightSky   = glm::vec3(0.005f, 0.015f, 0.04f); 

    // --- 2. TIMINGS (Standard Voxel : 50% Jour, 35% Nuit, 15% Transitions) ---
    // t=0.5 est le zénith absolu (midi). 
    // Jour :        [0.250, 0.750] (50%)
    // Crépuscule :  [0.750, 0.825] (7.5%)
    // Nuit :        [0.825, 1.000] et [0.000, 0.175] (35%)
    // Aube :        [0.175, 0.250] (7.5%)

    // Nuit (Minuit -> Fin de nuit)
    if (t < 0.175f) return nightSky; 
    
    // Aube (Transition rapide de 7.5%)
    if (t < 0.200f) return glm::mix(nightSky, redDusk, inverseLerp(0.175f, 0.200f, t));
    if (t < 0.225f) return glm::mix(redDusk, goldenHour, inverseLerp(0.200f, 0.225f, t));
    if (t < 0.250f) return glm::mix(goldenHour, daySky, inverseLerp(0.225f, 0.250f, t));
    
    // Plein Jour (50% du cycle)
    if (t < 0.750f) return daySky; 
    
    // Crépuscule (Transition rapide de 7.5%)
    if (t < 0.775f) return glm::mix(daySky, goldenHour, inverseLerp(0.750f, 0.775f, t));
    if (t < 0.800f) return glm::mix(goldenHour, sunset, inverseLerp(0.775f, 0.800f, t));
    if (t < 0.825f) return glm::mix(sunset, redDusk, inverseLerp(0.800f, 0.825f, t));
    
    // Nuit (Début de nuit -> Fade vers noir profond)
    if (t <= 1.0f) return glm::mix(redDusk, nightSky, inverseLerp(0.825f, 0.850f, t)); // Le fade s'arrête à 0.85, le reste est nightSky
    
    return nightSky;
}

static glm::vec3 computeHorizonColorFromTime(float t) {
    // Teintes spécifiques pour la brume à l'horizon
    glm::vec3 dayHorizon     = useReducedAmbient ? glm::vec3(0.40f, 0.50f, 0.65f) : glm::vec3(0.65f, 0.75f, 0.85f); // Brume bleutée claire
    glm::vec3 goldenHorizon  = glm::vec3(0.95f, 0.75f, 0.45f); // Horizon lumineux et doré
    glm::vec3 sunsetHorizon  = glm::vec3(0.95f, 0.40f, 0.10f); // Horizon "en feu"
    glm::vec3 duskHorizon    = glm::vec3(0.40f, 0.15f, 0.20f); // Brume pourpre/rouge
    glm::vec3 nightHorizon   = glm::vec3(0.02f, 0.04f, 0.08f); // Nuit (légèrement plus clair que le zénith)

    // Les mêmes timings stricts que pour le Zénith
    if (t < 0.175f) return nightHorizon; 
    
    if (t < 0.200f) return glm::mix(nightHorizon, duskHorizon, inverseLerp(0.175f, 0.200f, t));
    if (t < 0.225f) return glm::mix(duskHorizon, goldenHorizon, inverseLerp(0.200f, 0.225f, t));
    if (t < 0.250f) return glm::mix(goldenHorizon, dayHorizon, inverseLerp(0.225f, 0.250f, t));
    
    if (t < 0.750f) return dayHorizon; 
    
    if (t < 0.775f) return glm::mix(dayHorizon, goldenHorizon, inverseLerp(0.750f, 0.775f, t));
    if (t < 0.800f) return glm::mix(goldenHorizon, sunsetHorizon, inverseLerp(0.775f, 0.800f, t));
    if (t < 0.825f) return glm::mix(sunsetHorizon, duskHorizon, inverseLerp(0.800f, 0.825f, t));
    
    if (t <= 1.0f) return glm::mix(duskHorizon, nightHorizon, inverseLerp(0.825f, 0.850f, t)); 
    
    return nightHorizon;
}

static glm::vec3 computeAmbientGroundColorFromTime(float t) {
    // Teintes du sol sous le monde (doit correspondre aux couleurs de l'air pour la skybox)
    glm::vec3 dayGround    = useReducedAmbient ? glm::vec3(0.08f, 0.07f, 0.05f) : glm::vec3(0.12f, 0.10f, 0.08f);
    glm::vec3 sunsetGround = glm::vec3(0.18f, 0.10f, 0.06f);
    glm::vec3 nightGround  = glm::vec3(0.01f, 0.015f, 0.02f);

    if (t < 0.175f) return nightGround;
    if (t < 0.250f) return glm::mix(nightGround, dayGround, inverseLerp(0.175f, 0.250f, t));
    if (t < 0.750f) return dayGround;
    if (t < 0.825f) return glm::mix(dayGround, sunsetGround, inverseLerp(0.750f, 0.825f, t));
    if (t <= 1.0f)  return glm::mix(sunsetGround, nightGround, inverseLerp(0.825f, 0.850f, t));
    
    return nightGround;
}

static float computeExposureFromTime(float t) {
    // Calcul de l'élévation du soleil (1.0 = zénith, 0.0 = horizon, -1.0 = minuit)
    float angle = (t - 0.25f) * 2.0f * glm::pi<float>();
    float elevation = glm::sin(angle);
    
    float noonExposure = 0.8f;      // Assombrit le plein jour pour éviter que le blanc grille
    float twilightExposure = 1.2f;  // Éclaircit légèrement au coucher du soleil
    float nightExposure = 1.8f;     // Pousse l'exposition à fond la nuit pour voir quelque chose
    
    if (elevation > 0.1f) {
        // Jour (le soleil est haut)
        return glm::mix(twilightExposure, noonExposure, inverseLerp(0.1f, 1.0f, elevation));
    } else if (elevation > -0.1f) {
        // Horizon (Aube / Crépuscule)
        return glm::mix(nightExposure, twilightExposure, inverseLerp(-0.1f, 0.1f, elevation));
    } else {
        // Nuit (le soleil est sous l'horizon)
        return glm::mix(nightExposure, 2.2f, inverseLerp(-0.1f, -1.0f, elevation));
    }
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
    auto lightingView = registry.view<LightingStateComponent>();
    if (!lightingView.isEmpty()) {
        EntityID lightingEntity = *lightingView.begin();
        LightingStateComponent& lightingComp = registry.getComponent<LightingStateComponent>(lightingEntity);
        debugTBN = lightingComp.debugTBN;
        useNormalMap = lightingComp.useNormalMap;
        debugDiffuseOnly = lightingComp.debugDiffuseOnly;
        useBakedAO = lightingComp.useBakedAO;
        useHemisphericalAmbient = lightingComp.useHemisphericalAmbient;
        useReducedAmbient = lightingComp.useReducedAmbient;
        aoStrength = lightingComp.aoStrength;
    }
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

    GLint exposureLoc = glGetUniformLocation(activeProgramID, "exposure");
    if (exposureLoc >= 0) {
        glUniform1f(exposureLoc, computeExposureFromTime(dayTime));
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
    SkyboxSystem skyboxSystem;
    TimeSystem timeSystem;
    LightingCalculationSystem lightingSystem;
    
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
                debugInputSystem.update(registry, window, deltaTime);

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
                debugInputSystem.update(registry, window, deltaTime);

                // Update time system (day/night cycle)
                timeSystem.update(registry, deltaTime);
                
                // Calculate lighting state from time
                lightingSystem.update(registry);
                
                // Sync components to globals for backward compatibility
                syncComponentsToGlobals(registry);

                GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
                BlockTextureManager::bindArrays(activeProgramID);

                glUseProgram(activeProgramID);
                applyActiveShaderUniforms(activeProgramID);

                // Render terrain
                renderSystem.update(registry, activeProgramID, computeLightColorFromTime(dayTime), computeLightDirectionFromTime(dayTime));
                
                // Render skybox
                skyboxSystem.update(registry, getRenderDebugState(registry));
                
                if (debugWireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                ImGui_ImplOpenGL3_NewFrame();
                debugSystem.update(registry, window, deltaTime, getRenderDebugState(registry));
                
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
            debugInputSystem.update(registry, window, deltaTime);

            // Update time system (day/night cycle)
            timeSystem.update(registry, deltaTime);
            
            // Calculate lighting state from time
            lightingSystem.update(registry);
            
            // Sync components to globals for backward compatibility
            syncComponentsToGlobals(registry);

            GLuint activeProgramID = usePbrShader ? pbrProgramID : basicProgramID;
            BlockTextureManager::bindArrays(activeProgramID);

            glUseProgram(activeProgramID);
            applyActiveShaderUniforms(activeProgramID);

            // Render terrain
            renderSystem.update(registry, activeProgramID, computeLightColorFromTime(dayTime), computeLightDirectionFromTime(dayTime));

            // Render skybox
            skyboxSystem.update(registry, getRenderDebugState(registry));
            
            if (debugWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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