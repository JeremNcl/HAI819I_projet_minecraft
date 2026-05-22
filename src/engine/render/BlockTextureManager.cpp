#include "BlockTextureManager.hpp"
#include <iostream>

GLuint BlockTextureManager::colorArrayID = 0;
GLuint BlockTextureManager::normalArrayID = 0;
GLuint BlockTextureManager::metallicArrayID = 0;

void BlockTextureManager::initialize() {
    std::cout << "Initializing Block Texture Arrays..." << std::endl;

    // 1. Liste ordonnée des chemins pour l'Albedo
    std::vector<std::string> colorPaths = {
        "assets/textures/blocks/stone.png",            // STONE (0)
        "assets/textures/blocks/dirt.png",             // DIRT (1)
        "assets/textures/blocks/concrete_green.png"    // GRASS (2) (Temporaire)
    };

    // 2. Liste ordonnée des chemins pour les Normales
    std::vector<std::string> normalPaths = {
        "assets/textures/blocks/stone_normal.png",     // STONE (0)
        "assets/textures/blocks/dirt_normal.png",      // DIRT (1)
        "assets/textures/blocks/all_concrete_powder_normal.png" // GRASS (2) (Temporaire)
    };

    // 3. Liste ordonnée des chemins pour le Metallic/Roughness (MER)
    // R = Metallic, G = Roughness, B = Emission
    std::vector<std::string> metallicPaths = {
        "assets/textures/blocks/roughness100_mer.png", // STONE (0) - Gris par défaut
        "assets/textures/blocks/roughness100_mer.png", // DIRT (1) - Gris par défaut
        "assets/textures/blocks/all_concrete_powder_mer.png" // GRASS (2) (Temporaire)
    };

    // Chargement des 3 Texture Arrays
    colorArrayID = loadTextureArray(colorPaths);
    normalArrayID = loadTextureArray(normalPaths);
    metallicArrayID = loadTextureArray(metallicPaths);

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
