#pragma once

#include "engine/io/textureLoader.hpp"
#include "engine/render/BlockDefinitionRegistry.hpp"
#include <GL/glew.h>

class BlockTextureManager {
private:
    static GLuint colorArrayID;
    static GLuint normalArrayID;
    static GLuint metallicArrayID;
    static int sliceCount;

public:
    static void initialize();
    static void bindArrays(GLuint shaderProgram);
    static void cleanup();
    // Délègue le mapping voxel/face au registre de définitions.
    static int getTextureSliceIndex(VoxelType type, int axis, bool isPositive);
    
    static GLuint getColorArrayID() { return colorArrayID; }
    static GLuint getNormalArrayID() { return normalArrayID; }
    static GLuint getMetallicArrayID() { return metallicArrayID; }
};
