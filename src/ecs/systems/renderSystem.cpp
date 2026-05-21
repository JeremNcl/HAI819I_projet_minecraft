#include "renderSystem.hpp"
#include "../components/camera.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void RenderSystem::renderMesh(GLuint shaderProgram,
                              const MeshComponent& mesh,
                              const glm::mat4& modelMatrix,
                              const glm::mat4& viewMatrix,
                              const glm::mat4& projectionMatrix) {
    if (mesh.VAO == 0 || mesh.indexCount == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    // Build MVP matrix
    glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;
    GLint locMVP = glGetUniformLocation(shaderProgram, "MVP");
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::update(Registry& registry, GLuint shaderProgram) {
    auto view = registry.view<MeshComponent, TransformComponent>();
    
    Registry::View cameraView = registry.view<CameraComponent>();

    if (cameraView.isEmpty()) return; // Aucune camera
    
    //TODO : Peut etre passer la cam courrante (active) en singleton pour eviter la boucle à chaque fois (STATIC)
    CameraComponent* camera = nullptr;

    for (EntityID cameraEntity : cameraView) {
        CameraComponent& testedCamera = registry.getComponent<CameraComponent>(cameraEntity);
        if (testedCamera.isActive){
            camera = &testedCamera;
            break;
        }
    }

    for (EntityID entity : view) {
        const auto& mesh = registry.getComponent<MeshComponent>(entity);
        const auto& transform = registry.getComponent<TransformComponent>(entity);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, transform.position);
        modelMatrix = glm::scale(modelMatrix, transform.scale);

        renderMesh(shaderProgram, mesh, modelMatrix, camera->viewMatrix, camera->projectionMatrix);
    }

    glUseProgram(0);
}
