#pragma once

#include "component.hpp"
#include <GL/glew.h>
#include <cstddef>

struct MeshComponent : public Component {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint IBO = 0;
    size_t indexCount = 0;
    bool isDirty = true;

    MeshComponent() = default;

    void cleanup() {
        if (IBO != 0) {
            glDeleteBuffers(1, &IBO);
            IBO = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        indexCount = 0;
    }

};
