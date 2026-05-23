#pragma once

#include "../io/textureLoader.hpp"
#include <vector>
#include <string>
#include <GL/glew.h>

// L'ordre ici définit l'index (Z) dans le Texture Array
// 0 = STONE, 1 = DIRT, 2 = GRASS
enum class MaterialType {
    STONE = 0,
    DIRT = 1,
    GRASS = 2,
    COUNT // Permet de connaître le nombre de matériaux
};

class BlockTextureManager {
private:
    static GLuint colorArrayID;
    static GLuint normalArrayID;
    static GLuint metallicArrayID;

public:
    static void initialize();
    static void bindArrays(GLuint shaderProgram);
    static void cleanup();
    
    static GLuint getColorArrayID() { return colorArrayID; }
    static GLuint getNormalArrayID() { return normalArrayID; }
    static GLuint getMetallicArrayID() { return metallicArrayID; }
};
