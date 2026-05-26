#include "BlockTextureManager.hpp"
#include "PackTextureImporter.hpp"

#include <iostream>
#include <array>
#include <vector>

GLuint BlockTextureManager::colorArrayID = 0;
GLuint BlockTextureManager::normalArrayID = 0;
GLuint BlockTextureManager::metallicArrayID = 0;
int BlockTextureManager::sliceCount = 0;
std::array<float, 5> BlockTextureManager::normalStrengths = {0.28f, 0.28f, 0.28f, 0.28f, 0.28f};

namespace {
constexpr const char* kSourceTextureRoot = "/home/jerem/Documents/M1_IMAGINE_2025-2026/Semestre8/Moteur_De_Jeux/Projet/Vanilla-RTX-Opus-1.26.13/textures/blocks";
constexpr const char* kLocalTextureRoot = "assets/textures/blocks";

bool appendTextureSet(const std::string& textureSetPath,
                      std::vector<std::string>& colorPaths,
                      std::vector<std::string>& normalPaths,
                      std::vector<std::string>& metallicPaths,
                      std::array<float, 5>& strengths,
                      size_t targetIndex) {
    ImportedTextureSet importedSet;
    if (!PackTextureImporter::importTextureSet(textureSetPath, kLocalTextureRoot, importedSet)) {
        return false;
    }

    if (targetIndex >= colorPaths.size() || targetIndex >= normalPaths.size() || targetIndex >= metallicPaths.size()) {
        return false;
    }

    colorPaths[targetIndex] = importedSet.colorPath;
    normalPaths[targetIndex] = importedSet.normalPath;
    metallicPaths[targetIndex] = importedSet.merPath;
    strengths[targetIndex] = importedSet.normalStrength;
    return true;
}
} // namespace

void BlockTextureManager::initialize() {
    std::cout << "Initializing Block Texture Arrays..." << std::endl;

    BlockDefinitionRegistry::initialize();

    std::vector<std::string> colorPaths(5);
    std::vector<std::string> normalPaths(5);
    std::vector<std::string> metallicPaths(5);
    normalStrengths = {0.28f, 0.28f, 0.28f, 0.28f, 0.28f};

    const bool importedStone = appendTextureSet(
        std::string(kSourceTextureRoot) + "/stone.texture_set.json",
        colorPaths,
        normalPaths,
        metallicPaths,
        normalStrengths,
        0);

    const bool importedDirt = appendTextureSet(
        std::string(kSourceTextureRoot) + "/dirt.texture_set.json",
        colorPaths,
        normalPaths,
        metallicPaths,
        normalStrengths,
        1);

    const bool importedGrassTop = appendTextureSet(
        std::string(kSourceTextureRoot) + "/grass_top.texture_set.json",
        colorPaths,
        normalPaths,
        metallicPaths,
        normalStrengths,
        2);

    const bool importedGrassSide = appendTextureSet(
        std::string(kSourceTextureRoot) + "/grass_side.texture_set.json",
        colorPaths,
        normalPaths,
        metallicPaths,
        normalStrengths,
        3);

    if (!importedStone || !importedDirt || !importedGrassTop || !importedGrassSide) {
        std::cerr << "Pack import failed for the first block set, falling back to local assets." << std::endl;
        colorPaths = {
            "assets/textures/blocks/stone.tga",
            "assets/textures/blocks/dirt.tga",
            "assets/textures/blocks/grass_top.tga",
            "assets/textures/blocks/grass_side.tga",
            ""
        };
        normalPaths = {
            "assets/textures/blocks/stone_normal.tga",
            "assets/textures/blocks/dirt_normal.tga",
            "assets/textures/blocks/grass_top_normal.tga",
            "assets/textures/blocks/grass_side_normal.tga",
            ""
        };
        metallicPaths = {
            "assets/textures/blocks/stone_mer.tga",
            "assets/textures/blocks/dirt_mer.tga",
            "assets/textures/blocks/grass_top_mer.tga",
            "assets/textures/blocks/grass_side_mer.tga",
            ""
        };
        normalStrengths = {0.22f, 0.45f, 0.42f, 0.38f, 0.28f};
    }

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

    GLint normalStrengthsLoc = glGetUniformLocation(shaderProgram, "normalStrengths");
    if (normalStrengthsLoc >= 0) {
        // Apply a small global reduction to normal map influence for overall softer normals
        constexpr float kNormalGlobalFactor = 0.85f; // 85% of original
        std::array<float, 5> scaled = normalStrengths;
        for (size_t i = 0; i < scaled.size(); ++i) scaled[i] *= kNormalGlobalFactor;
        glUniform1fv(normalStrengthsLoc, static_cast<GLsizei>(scaled.size()), scaled.data());
    }
    
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
