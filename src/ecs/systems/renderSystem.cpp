#include "renderSystem.hpp"
#include "ecs/components/camera.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_map>
#include <unordered_set>
#include <queue>


#ifndef GLM_VEC3_HASH_DEFINED
#define GLM_VEC3_HASH_DEFINED
struct GLMVec3Hash {
    std::size_t operator()(const glm::ivec3& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
    }
};
#endif


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

void RenderSystem::renderMesh(GLuint shaderProgram,
                              GLint locMVP,
                              const MeshComponent& mesh,
                              const glm::mat4& modelMatrix,
                              const glm::mat4& viewMatrix,
                              const glm::mat4& projectionMatrix,
                              const glm::vec3& lightColor,
                              const glm::vec3& lightDirection) {
    if (mesh.VAO == 0 || mesh.indexCount == 0) return;

    glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;
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
    glUniform3fv(locLightPos, 1, glm::value_ptr(lightDirection));
    glUniform3fv(locLightColor, 1, glm::value_ptr(lightColor));

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void RenderSystem::update(Registry& registry, GLuint shaderProgram, const glm::vec3& lightColor, const glm::vec3& lightDirection) {    
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
    glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(vpMatrix));

    std::array<glm::vec4, 6> frustumPlanes;
    extractFrustumPlanes(vpMatrix, frustumPlanes);

    struct RenderNode {
        EntityID entity;
        float distanceSq;
        const MeshComponent* mesh;
    };
    std::vector<RenderNode> visibleChunks;
    visibleChunks.reserve(2000);

    glm::vec3 camPos = registry.getComponent<TransformComponent>(lastActiveCamera).position;

    for (EntityID entity : view) {
        const auto& mesh = registry.getComponent<MeshComponent>(entity);
        if (mesh.indexCount == 0 || mesh.VAO == 0) continue;

        const auto& transform = registry.getComponent<TransformComponent>(entity);
        const auto& subChunk = registry.getComponent<SubChunkComponent>(entity);
        
        glm::vec3 minBounds = (glm::vec3(subChunk.subChunkPosition) * 16.0f) + transform.position;
        glm::vec3 maxBounds = minBounds + glm::vec3(16.0f, 16.0f, 16.0f);
        
        if (!isAABBInFrustum(minBounds, maxBounds, frustumPlanes)) continue;

        glm::vec3 diff = camPos - ((minBounds + maxBounds) * 0.5f);
        visibleChunks.push_back({entity, glm::dot(diff, diff), &mesh});
    }

    std::sort(visibleChunks.begin(), visibleChunks.end(), [](const RenderNode& a, const RenderNode& b) {
        return a.distanceSq < b.distanceSq; 
    });

    // 3. RENDU BATCHÉ
    for (const auto& node : visibleChunks) {
        renderMesh(shaderProgram, locMVP, *node.mesh, glm::mat4(1.0f), camera->viewMatrix, camera->projectionMatrix, lightColor, lightDirection);
    }
    
    glBindVertexArray(0);
    glUseProgram(0);
}
