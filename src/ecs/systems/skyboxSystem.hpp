#pragma once

#include <GL/glew.h>
#include "../../ecs/registry.hpp"
#include <glm/glm.hpp>

struct RenderDebugState;

class SkyboxSystem {
public:
    void initialize();
    void initializeSkyboxGeometry(Registry& registry, EntityID skyboxEntity);
    void update(Registry& registry, const RenderDebugState& renderState);
    void cleanup();

private:
    GLuint skyboxProgramID = 0;
};
