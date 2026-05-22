#include "renderSystem.hpp"
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

    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

    GLint locModel = glGetUniformLocation(shaderProgram, "model");
    GLint locView = glGetUniformLocation(shaderProgram, "view");
    GLint locProjection = glGetUniformLocation(shaderProgram, "projection");
    GLint locNormalMatrix = glGetUniformLocation(shaderProgram, "normalMatrix");

    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix3fv(locNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    GLint locViewPos = glGetUniformLocation(shaderProgram, "viewPos");
    glm::vec3 viewPos = glm::vec3(glm::inverse(viewMatrix)[3]);
    glUniform3fv(locViewPos, 1, glm::value_ptr(viewPos));

    GLint locLightPos = glGetUniformLocation(shaderProgram, "lightPos");
    GLint locLightColor = glGetUniformLocation(shaderProgram, "lightColor");
    glm::vec3 lightPos(128.0f, 400.0f, 128.0f); // Un beau soleil de midi
    glm::vec3 lightColor(3.0f, 3.0f, 3.0f);   // Un peu plus fort
    glUniform3fv(locLightPos, 1, glm::value_ptr(lightPos));
    glUniform3fv(locLightColor, 1, glm::value_ptr(lightColor));

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::update(Registry& registry, GLuint shaderProgram,
                         const glm::mat4& viewMatrix,
                         const glm::mat4& projectionMatrix) {
    auto view = registry.view<MeshComponent, TransformComponent>();

    for (EntityID entity : view) {
        const auto& mesh = registry.getComponent<MeshComponent>(entity);
        const auto& transform = registry.getComponent<TransformComponent>(entity);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, transform.position);
        modelMatrix = glm::scale(modelMatrix, transform.scale);

        renderMesh(shaderProgram, mesh, modelMatrix, viewMatrix, projectionMatrix);
    }

    glUseProgram(0);
}

