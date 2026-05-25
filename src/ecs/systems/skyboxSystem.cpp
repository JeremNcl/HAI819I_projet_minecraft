#include "skyboxSystem.hpp"
#include "../components/skyboxComponent.hpp"
#include "../components/transform.hpp"
#include "../components/camera.hpp"
#include "../../engine/render/shader.hpp"
#include "debugSystem.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Create a simple cube geometry for skybox
static void createSkyboxCube(GLuint& VAO, GLuint& VBO, GLuint& EBO) {
    // Cube vertices (unit cube centered at origin)
    float vertices[] = {
        // Front face
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        // Back face
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
    };

    unsigned int indices[] = {
        // Front
        0, 1, 2, 2, 3, 0,
        // Right
        1, 5, 6, 6, 2, 1,
        // Back
        5, 4, 7, 7, 6, 5,
        // Left
        4, 0, 3, 3, 7, 4,
        // Top
        3, 2, 6, 6, 7, 3,
        // Bottom
        4, 5, 1, 1, 0, 4
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SkyboxSystem::initialize() {
    skyboxProgramID = LoadShaders("assets/shaders/skybox_vertex.glsl", "assets/shaders/skybox_fragment.glsl");
    if (!skyboxProgramID) {
        std::cerr << "Failed to load skybox shader program" << std::endl;
    }
}

void SkyboxSystem::initializeSkyboxGeometry(Registry& registry, EntityID skyboxEntity) {
    if (!registry.hasComponent<SkyboxComponent>(skyboxEntity)) {
        std::cerr << "Skybox entity has no SkyboxComponent" << std::endl;
        return;
    }

    auto& skybox = registry.getComponent<SkyboxComponent>(skyboxEntity);
    
    // Create cube geometry
    createSkyboxCube(skybox.VAO, skybox.VBO, skybox.EBO);
}

void SkyboxSystem::update(Registry& registry, const RenderDebugState& renderState) {
    if (!skyboxProgramID) return;

    // Find skybox entity
    auto skyboxView = registry.view<SkyboxComponent>();
    
    if (skyboxView.isEmpty()) {
        return;
    }

    EntityID skyboxEntity = *skyboxView.begin();
    auto& skybox = registry.getComponent<SkyboxComponent>(skyboxEntity);

    // Find active camera
    auto cameraView = registry.view<CameraComponent>();
    CameraComponent* camera = nullptr;
    
    for (EntityID camEntity : cameraView) {
        if (registry.hasComponent<CameraComponent>(camEntity)) {
            CameraComponent& cam = registry.getComponent<CameraComponent>(camEntity);
            if (cam.isActive) {
                camera = &cam;
                break;
            }
        }
    }
    
    if (!camera) {
        return;
    }

    // Use skybox shader
    glUseProgram(skyboxProgramID);

    // Disable depth writing (skybox is always rendered behind)
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    // Désactiver le culling car on est à l'intérieur du cube
    glDisable(GL_CULL_FACE);

    // Set uniforms
    GLint viewLoc = glGetUniformLocation(skyboxProgramID, "view");
    GLint projLoc = glGetUniformLocation(skyboxProgramID, "projection");
    GLint skyColorLoc = glGetUniformLocation(skyboxProgramID, "ambientSkyColor");
    GLint groundColorLoc = glGetUniformLocation(skyboxProgramID, "ambientGroundColor");
    GLint exposureLoc = glGetUniformLocation(skyboxProgramID, "exposure");
GLint horizonColorLoc = glGetUniformLocation(skyboxProgramID, "horizonColor");

    if (viewLoc >= 0) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
    }
    if (projLoc >= 0) {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
    }
    if (skyColorLoc >= 0) {
        glUniform3fv(skyColorLoc, 1, glm::value_ptr(renderState.ambientSkyColor));
    }
    if (groundColorLoc >= 0) {
        glUniform3fv(groundColorLoc, 1, glm::value_ptr(renderState.ambientGroundColor));
    }
    if (exposureLoc >= 0) {
        glUniform1f(exposureLoc, renderState.exposure);
    }
    if (horizonColorLoc >= 0) {
        glUniform3fv(horizonColorLoc, 1, glm::value_ptr(renderState.horizonColor));
    }

    // Render skybox
    glBindVertexArray(skybox.VAO);
    glDrawElements(GL_TRIANGLES, skybox.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore depth settings
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

void SkyboxSystem::cleanup() {
    if (skyboxProgramID) {
        glDeleteProgram(skyboxProgramID);
    }
}
