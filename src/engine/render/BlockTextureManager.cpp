#include "BlockTextureManager.hpp"
#include "PackTextureImporter.hpp"

#include <iostream>
#include <array>
#include <vector>

GLuint BlockTextureManager::colorArrayID = 0;
GLuint BlockTextureManager::normalArrayID = 0;
GLuint BlockTextureManager::metallicArrayID = 0;
int BlockTextureManager::sliceCount = 0;
std::array<float, 16> BlockTextureManager::normalStrengths = {0.28f}; // Initialisé à 16

namespace {
constexpr const char* kSourceTextureRoot = "/home/jerem/Documents/M1_IMAGINE_2025-2026/Semestre8/Moteur_De_Jeux/Projet/Vanilla-RTX-Opus-1.26.13/textures/blocks";
constexpr const char* kLocalTextureRoot = "assets/textures/blocks";

bool appendTextureSet(const std::string& textureSetPath,
                      std::vector<std::string>& colorPaths,
                      std::vector<std::string>& normalPaths,
                      std::vector<std::string>& metallicPaths,
                      std::array<float, 16>& strengths,
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
    std::cout << "Initializing Block Texture Arrays (16 slots)..." << std::endl;

    BlockDefinitionRegistry::initialize();

    // Allocation de 16 slots pour anticiper tous les blocs de l'énumération
    std::vector<std::string> colorPaths(16, "");
    std::vector<std::string> normalPaths(16, "");
    std::vector<std::string> metallicPaths(16, "");
    normalStrengths.fill(0.28f);

    // ========================================================
    // INDEX 0 : TEXTURE D'ERREUR (FALLBACK ABSOLU)
    // ========================================================
    colorPaths[0]    = "assets/textures/blocks/error.tga";
    normalPaths[0]   = "assets/textures/blocks/error_normal.tga";
    metallicPaths[0] = "assets/textures/blocks/error_mer.tga";
    normalStrengths[0] = 0.0f; // Surface plate pour l'erreur

    // ========================================================
    // CHARGEMENT DU PACK PBR (DÉCALAGE DE +1 POUR LES ANCIENS)
    // ========================================================
    bool success = true;
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/stone.texture_set.json",     colorPaths, normalPaths, metallicPaths, normalStrengths, 1);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/dirt.texture_set.json",      colorPaths, normalPaths, metallicPaths, normalStrengths, 2);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/grass_top.texture_set.json",  colorPaths, normalPaths, metallicPaths, normalStrengths, 3);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/grass_side.texture_set.json", colorPaths, normalPaths, metallicPaths, normalStrengths, 4);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/sand.texture_set.json",       colorPaths, normalPaths, metallicPaths, normalStrengths, 5);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/log_oak_top.texture_set.json", colorPaths, normalPaths, metallicPaths, normalStrengths, 6);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/log_oak.texture_set.json",     colorPaths, normalPaths, metallicPaths, normalStrengths, 7); // Écorce latérale
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/leaves_oak.texture_set.json",  colorPaths, normalPaths, metallicPaths, normalStrengths, 8);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/bedrock.texture_set.json",      colorPaths, normalPaths, metallicPaths, normalStrengths, 9);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/coal_ore.texture_set.json",     colorPaths, normalPaths, metallicPaths, normalStrengths, 10);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/iron_ore.texture_set.json",     colorPaths, normalPaths, metallicPaths, normalStrengths, 11);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/gold_ore.texture_set.json",     colorPaths, normalPaths, metallicPaths, normalStrengths, 12);
    success &= appendTextureSet(std::string(kSourceTextureRoot) + "/diamond_ore.texture_set.json",  colorPaths, normalPaths, metallicPaths, normalStrengths, 13);

    if (!success) {
        std::cerr << "Pack PBR incomplet dans le dossier source, application du fallback local." << std::endl;
        // Remplir manuellement les index si le dossier absolu est introuvable
        colorPaths[1] = "assets/textures/blocks/stone.tga";
        colorPaths[2] = "assets/textures/blocks/dirt.tga";
        colorPaths[3] = "assets/textures/blocks/grass_top.tga";
        colorPaths[4] = "assets/textures/blocks/grass_side.tga";
        colorPaths[5] = "assets/textures/blocks/sand.tga";
        colorPaths[6] = "assets/textures/blocks/log_oak_top.tga";
        colorPaths[7] = "assets/textures/blocks/log_oak.tga";
        colorPaths[8] = "assets/textures/blocks/leaves_oak.tga";
        colorPaths[9] = "assets/textures/blocks/bedrock.tga";
        colorPaths[10] = "assets/textures/blocks/coal_ore.tga";
        colorPaths[11] = "assets/textures/blocks/iron_ore.tga";
        colorPaths[12] = "assets/textures/blocks/gold_ore.tga";
        colorPaths[13] = "assets/textures/blocks/diamond_ore.tga";
    }

    colorArrayID    = loadTextureArray(colorPaths);
    normalArrayID   = loadTextureArray(normalPaths);
    metallicArrayID = loadTextureArray(metallicPaths);

    sliceCount = static_cast<int>(colorPaths.size());
}

void BlockTextureManager::bindArrays(GLuint shaderProgram) {
    glUseProgram(shaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, colorArrayID);
    glUniform1i(glGetUniformLocation(shaderProgram, "colorMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, normalArrayID);
    glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, metallicArrayID);
    glUniform1i(glGetUniformLocation(shaderProgram, "metallicMap"), 2);

    GLint normalStrengthsLoc = glGetUniformLocation(shaderProgram, "normalStrengths");
    if (normalStrengthsLoc >= 0) {
        constexpr float kNormalGlobalFactor = 0.85f;
        std::array<float, 16> scaled = normalStrengths;
        for (size_t i = 0; i < scaled.size(); ++i) scaled[i] *= kNormalGlobalFactor;
        glUniform1fv(normalStrengthsLoc, static_cast<GLsizei>(scaled.size()), scaled.data());
    }
    
    glActiveTexture(GL_TEXTURE0);
}

void BlockTextureManager::cleanup() {
    glDeleteTextures(1, &colorArrayID);
    glDeleteTextures(1, &normalArrayID);
    glDeleteTextures(1, &metallicArrayID);
}

int BlockTextureManager::getTextureSliceIndex(VoxelType type, int axis, bool isPositive) {
    int idx = BlockDefinitionRegistry::getTextureSliceIndex(type, axis, isPositive);
    if (sliceCount <= 0) return idx;
    if (idx >= sliceCount) {
        return 0; // Sécurité : retourne l'erreur si hors limites
    }
    return idx;
}