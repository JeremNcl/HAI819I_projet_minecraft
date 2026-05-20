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

// Inclusions de notre moteur (Nouvelle architecture)
#include "engine/render/shader.hpp"
#include "engine/io/textureLoader.hpp"
#include "engine/scene/camera.hpp"
#include "engine/scene/sceneGraph.hpp"
#include "engine/scene/meshNode.hpp"

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
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Chargement du shader générique
    GLuint basicProgramID = LoadShaders("assets/shaders/vertex_shader.glsl", "assets/shaders/fragment_shader.glsl");
    glUseProgram(basicProgramID);
    GLuint MVP_ID = glGetUniformLocation(basicProgramID, "MVP");

    // Chargement d'une texture de test
    GLuint blockTexture = loadBMP_custom("assets/textures/grass.bmp");
    
    // === INITIALISATION DU MONDE ===
    
    SceneGraph sceneGraph;
    
    // TODO: Instancier ici le premier ChunkNode et l'ajouter au sceneGraph
    // auto chunkNode = std::make_shared<ChunkNode>(...);
    // sceneGraph.getRoot()->addChild(chunkNode);

    printf("Graphe de scène initialisé. Prêt pour l'ajout des Chunks.\n");

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
        glm::mat4 viewProjection = camera.getProjectionMatrix() * camera.getViewMatrix();

        // Update & Draw de la scène
        sceneGraph.update(deltaTime);
        sceneGraph.draw(viewProjection);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) && (glfwWindowShouldClose(window) == 0) );

    // Cleanup
    MeshNode::clearMeshCache();
    glDeleteProgram(basicProgramID);
    glDeleteVertexArrays(1, &VertexArrayID);

    glfwTerminate();
    return 0;
}

// Gestion des inputs
void processInput(GLFWwindow *window, Camera& camera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // On peut retirer le toggle de caméra isométrique car inutile en vue Voxel FPS.
}

// Resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}