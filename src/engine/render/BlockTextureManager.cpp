#include "BlockTextureManager.hpp"
#include <iostream>

GLuint BlockTextureManager::colorArrayID = 0;
GLuint BlockTextureManager::normalArrayID = 0;
GLuint BlockTextureManager::metallicArrayID = 0;
int BlockTextureManager::sliceCount = 0;

void BlockTextureManager::initialize() {
    std::cout << "Initializing Block Texture Arrays..." << std::endl;

    BlockDefinitionRegistry::initialize();

    // 1. Liste ordonnée des chemins pour l'Albedo
    std::vector<std::string> colorPaths = {
        "assets/textures/blocks/stone.tga",       // Index 0
        "assets/textures/blocks/dirt.tga",        // Index 1
        "assets/textures/blocks/grass_top.tga",   // Index 2
        "assets/textures/blocks/grass_side.tga",  // Index 3
        ""                                         // Index 4: fallback magenta
    };

    // 2. Liste ordonnée des chemins pour les Normales
    std::vector<std::string> normalPaths = {
        "assets/textures/blocks/stone_normal.tga",     // Index 0
        "assets/textures/blocks/dirt_normal.tga",      // Index 1
        "assets/textures/blocks/grass_top_normal.tga", // Index 2
        "assets/textures/blocks/grass_side_normal.tga",// Index 3
        ""                                             // Index 4: fallback magenta
    };

    // 3. Liste ordonnée des chemins pour le Metallic/Roughness (MER)
    // R = Metallic, G = Emission, B = Roughness
    std::vector<std::string> metallicPaths = {
        "assets/textures/blocks/stone_mer.tga",        // Index 0
        "assets/textures/blocks/dirt_mer.tga",         // Index 1
        "assets/textures/blocks/grass_top_mer.tga",    // Index 2
        "assets/textures/blocks/grass_side_mer.tga",   // Index 3
        ""                                              // Index 4: fallback magenta
    };

    // Chargement des 3 Texture Arrays
    colorArrayID = loadTextureArray(colorPaths);
    normalArrayID = loadTextureArray(normalPaths);
    metallicArrayID = loadTextureArray(metallicPaths);

    // Enregistrer le nombre de slices attendues pour diagnostics / mapping
    sliceCount = static_cast<int>(colorPaths.size());

    if (colorArrayID == 0 || normalArrayID == 0 || metallicArrayID == 0) {
        std::cerr << "ERREUR CRITIQUE: Echec de la création des Texture Arrays." << std::endl;
    } else {
        std::cout << "Texture Arrays générés avec succès !" << std::endl;
    }
}

void BlockTextureManager::bindArrays(GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    // Color Array sur l'Unité 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, colorArrayID);
    glUniform1i(glGetUniformLocation(shaderProgram, "textureSampler"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "colorMap"), 0);

    // Normal Array sur l'Unité 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, normalArrayID);
    glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 1);

    // Metallic Array sur l'Unité 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, metallicArrayID);
    glUniform1i(glGetUniformLocation(shaderProgram, "metallicMap"), 2);
    
    // Rétablir l'unité active par défaut
    glActiveTexture(GL_TEXTURE0);
}

void BlockTextureManager::cleanup() {
    std::cout << "Cleaning up Block Texture Arrays..." << std::endl;
    glDeleteTextures(1, &colorArrayID);
    glDeleteTextures(1, &normalArrayID);
    glDeleteTextures(1, &metallicArrayID);
}

int BlockTextureManager::getTextureSliceIndex(VoxelType type, int axis, bool isPositive) {
    int idx = BlockDefinitionRegistry::getTextureSliceIndex(type, axis, isPositive);
    if (sliceCount <= 0) return idx;
    if (idx >= sliceCount) {
        std::cerr << "Warning: requested texture slice " << idx << " out of range (" << sliceCount << "). Clamping to 0." << std::endl;
        return 0;
    }
    return idx;
}
