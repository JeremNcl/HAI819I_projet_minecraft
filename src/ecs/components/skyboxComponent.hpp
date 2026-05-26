#pragma once

#include "component.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>

struct SkyboxComponent : public Component {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint indexCount = 36; // 6 faces * 6 indices each for cube
};
