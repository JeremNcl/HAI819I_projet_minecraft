#include "renderSystem.hpp"
#include "../components/camera.hpp"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void RenderSystem::extractFrustumPlanes(const glm::mat4& vp, std::array<glm::vec4, 6>& planes) {
    planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
    planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
    planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
    planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
    planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
    planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(planes[i]));
        planes[i] /= length;
    }
}

bool RenderSystem::isAABBInFrustum(const glm::vec3& min, const glm::vec3& max, const std::array<glm::vec4, 6>& planes) {
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 extents = (max - min) * 0.5f;

    for (int i = 0; i < 6; i++) {
        glm::vec3 normal = glm::vec3(planes[i]);
        
        float r = extents.x * std::abs(normal.x) + 
                  extents.y * std::abs(normal.y) + 
                  extents.z * std::abs(normal.z);
        
        float d = glm::dot(normal, center) + planes[i].w;

        if (d < -r) {
            return false; 
        }
    }
    return true;
}

void RenderSystem::renderMesh(GLint locMVP,
                              const MeshComponent& mesh,
                              const glm::mat4& modelMatrix,
                              const glm::mat4& viewMatrix,
                              const glm::mat4& projectionMatrix) {
    if (mesh.VAO == 0 || mesh.indexCount == 0) return;

    glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::update(Registry& registry, GLuint shaderProgram) {    
    auto view = registry.view<MeshComponent, TransformComponent>();
    Registry::View cameraView = registry.view<CameraComponent>();

    if (cameraView.isEmpty()) return;
    
    static EntityID lastActiveCamera = -1;
    CameraComponent* camera = nullptr;

    if (lastActiveCamera != -1 && registry.hasComponent<CameraComponent>(lastActiveCamera)) {
        CameraComponent& cam = registry.getComponent<CameraComponent>(lastActiveCamera);
        if (cam.isActive) camera = &cam;
    }

    if (!camera) {
        for (EntityID cameraEntity : cameraView) {
            CameraComponent& testedCamera = registry.getComponent<CameraComponent>(cameraEntity);
            if (testedCamera.isActive){
                camera = &testedCamera;
                lastActiveCamera = cameraEntity;
                break;
            }
        }
    }

    if (!camera) return;

    glUseProgram(shaderProgram);
    GLint locMVP = glGetUniformLocation(shaderProgram, "MVP");
    glm::mat4 vpMatrix = camera->projectionMatrix * camera->viewMatrix;
    std::array<glm::vec4, 6> frustumPlanes;
    extractFrustumPlanes(vpMatrix, frustumPlanes);

    for (EntityID entity : view) {
        const auto& mesh = registry.getComponent<MeshComponent>(entity);
        const auto& transform = registry.getComponent<TransformComponent>(entity);

        if (registry.hasComponent<SubChunkComponent>(entity)) {
            const auto& subChunk = registry.getComponent<SubChunkComponent>(entity);

            glm::vec3 minBounds = (glm::vec3(subChunk.subChunkPosition) * 16.0f) + transform.position;
            glm::vec3 maxBounds = minBounds + (glm::vec3(16.0f, 16.0f, 16.0f) * transform.scale);

            if (!isAABBInFrustum(minBounds, maxBounds, frustumPlanes)){
                continue;
            }
        }
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, transform.position);
        modelMatrix = glm::scale(modelMatrix, transform.scale);
        renderMesh(locMVP, mesh, modelMatrix, camera->viewMatrix, camera->projectionMatrix);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

