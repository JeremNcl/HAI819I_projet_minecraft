#pragma once

#include "../registry.hpp"
#include "../components/mesh.hpp"
#include "../components/transform.hpp"
#include <glm/glm.hpp>

class RenderSystem {
public:
    RenderSystem() = default;

    void update(Registry& registry, GLuint shaderProgram);

private:
    void renderMesh(GLuint shaderProgram,
                    const MeshComponent& mesh,
                    const glm::mat4& modelMatrix,
                    const glm::mat4& viewMatrix,
                    const glm::mat4& projectionMatrix);
};
