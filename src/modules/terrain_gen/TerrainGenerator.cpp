#include "TerrainGenerator.hpp"
#include "../../../external/FastNoiseLite.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <array>
#include <glm/glm.hpp>

namespace {
glm::vec3 computeBiomeTint(float desertWeight, float plainsWeight, float mountainWeight) {
    const glm::vec3 desertTint(0.76f, 0.68f, 0.40f);
    const glm::vec3 plainsTint(0.48f, 0.74f, 0.42f);
    const glm::vec3 mountainTint(0.36f, 0.56f, 0.31f);

    float weightSum = std::max(0.0001f, desertWeight + plainsWeight + mountainWeight);
    float normalizedDesert = desertWeight / weightSum;
    float normalizedPlains = plainsWeight / weightSum;
    float normalizedMountain = mountainWeight / weightSum;

    return normalizedDesert * desertTint + normalizedPlains * plainsTint + normalizedMountain * mountainTint;
}
} // namespace

TerrainConfig LoadConfig(const std::string& filename) {
    TerrainConfig config;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Attention: Impossible d'ouvrir " << filename << ". Utilisation des valeurs par defaut.\n";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '\r') continue;

        std::istringstream iss(line);
        std::string key, equals;
        float value;

        if (iss >> key >> equals >> value && equals == "=") {
            if (key == "seed") config.seed = static_cast<int>(value);
            else if (key == "terrain_freq") config.terrainFreq = value;
            else if (key == "cave_freq") config.caveFreq = value;
            else if (key == "biome_freq") config.biomeFreq = value;
            else if (key == "height_mountain") config.heightMountain = static_cast<int>(value);
            else if (key == "height_plains") config.heightPlains = static_cast<int>(value);
            else if (key == "height_desert") config.heightDesert = static_cast<int>(value);
            else if (key == "height_ocean") config.heightOcean = static_cast<int>(value);
            else if (key == "amplitude") config.amplitude = value;
            else if (key == "cave_threshold") config.caveThreshold = value;
            else if (key == "tree_chance") config.treeChance = static_cast<int>(value);
        }
    }
    
    std::cout << "Configuration chargee depuis " << filename << " avec succes.\n";
    return config;
}

TerrainGenerator::TerrainGenerator(const TerrainConfig& config) {
    m_config = config;
    m_seed = config.seed;
}

int TerrainGenerator::GetIndex(int x, int y, int z) const {
    return x + (z * CHUNK_WIDTH) + (y * CHUNK_DEPTH * CHUNK_WIDTH);
}

std::array<std::vector<BlockType>, 16> TerrainGenerator::GenerateChunk(int chunkX, int chunkZ, std::array<glm::vec3, CHUNK_WIDTH * CHUNK_DEPTH>* outBiomeColors) {
    std::vector<BlockType> blocks(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH, BlockType::AIR);
    std::vector<glm::ivec3> treesToGenerate;

    FastNoiseLite terrainNoise;
    terrainNoise.SetSeed(m_seed);
    terrainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    terrainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    terrainNoise.SetFractalOctaves(4);
    terrainNoise.SetFrequency(m_config.terrainFreq);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(m_seed);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFrequency(m_config.biomeFreq);

    FastNoiseLite densityNoise;
    densityNoise.SetSeed(m_seed + 123); 
    densityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    densityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    densityNoise.SetFractalOctaves(3); 
    densityNoise.SetFrequency(0.015f); 

    if (outBiomeColors) {
        outBiomeColors->fill(glm::vec3(1.0f));
    }

    constexpr int STEP_X = 4;
    constexpr int STEP_Y = 8;
    constexpr int STEP_Z = 4;
    
    constexpr int GRID_X = (CHUNK_WIDTH / STEP_X) + 1;
    constexpr int GRID_Y = (CHUNK_HEIGHT / STEP_Y) + 1;
    constexpr int GRID_Z = (CHUNK_DEPTH / STEP_Z) + 1;

    float coarseNoise[GRID_X][GRID_Y][GRID_Z];

    for (int gx = 0; gx < GRID_X; ++gx) {
        for (int gy = 0; gy < GRID_Y; ++gy) {
            for (int gz = 0; gz < GRID_Z; ++gz) {
                float globalX = (chunkX * CHUNK_WIDTH) + (gx * STEP_X);
                float globalY = gy * STEP_Y;
                float globalZ = (chunkZ * CHUNK_DEPTH) + (gz * STEP_Z);
                
                coarseNoise[gx][gy][gz] = densityNoise.GetNoise(globalX, globalY * 1.5f, globalZ);
            }
        }
    }

    auto lerp = [](float a, float b, float t) {
        return a + t * (b - a);
    };

    for (int z = 0; z < CHUNK_DEPTH; ++z) {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            
            float globalX = (chunkX * CHUNK_WIDTH) + x;
            float globalZ = (chunkZ * CHUNK_DEPTH) + z;

            float biomeValue = biomeNoise.GetNoise(globalX, globalZ);
            float dW = std::max(0.0f, 1.0f - std::abs(biomeValue + 0.5f) / 0.5f);
            float mW = std::max(0.0f, 1.0f - std::abs(biomeValue - 0.5f) / 0.5f);
            float pW = 1.0f - dW - mW;

            if (outBiomeColors) {
                (*outBiomeColors)[x + z * CHUNK_WIDTH] = computeBiomeTint(dW, pW, mW);
            }

            float noise2D = terrainNoise.GetNoise(globalX, globalZ);
            float baseHeight = (dW * m_config.heightDesert + 
                                pW * m_config.heightPlains + 
                                mW * m_config.heightMountain) + (noise2D * 15.0f);

            enum class Biome { DESERT, PLAINS, MOUNTAIN };
            Biome currentBiome = Biome::PLAINS;
            if (biomeValue < -0.2f) currentBiome = Biome::DESERT;
            else if (biomeValue > 0.3f) currentBiome = Biome::MOUNTAIN;

            const int WATER_LEVEL = 50;
            int depth = 0; 
            int highestGrassY = -1;

            int gx0 = x / STEP_X;
            int gx1 = gx0 + 1;
            float tx = static_cast<float>(x % STEP_X) / STEP_X;

            int gz0 = z / STEP_Z;
            int gz1 = gz0 + 1;
            float tz = static_cast<float>(z % STEP_Z) / STEP_Z;

            for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
                int index = GetIndex(x, y, z);
                
                if (y == 0) {
                    blocks[index] = BlockType::BEDROCK;
                    continue;
                }

                int gy0 = y / STEP_Y;
                int gy1 = gy0 + 1;
                float ty = static_cast<float>(y % STEP_Y) / STEP_Y;

                float c000 = coarseNoise[gx0][gy0][gz0];
                float c100 = coarseNoise[gx1][gy0][gz0];
                float c010 = coarseNoise[gx0][gy1][gz0];
                float c110 = coarseNoise[gx1][gy1][gz0];
                float c001 = coarseNoise[gx0][gy0][gz1];
                float c101 = coarseNoise[gx1][gy0][gz1];
                float c011 = coarseNoise[gx0][gy1][gz1];
                float c111 = coarseNoise[gx1][gy1][gz1];

                float v00 = lerp(c000, c100, tx);
                float v01 = lerp(c001, c101, tx);
                float v10 = lerp(c010, c110, tx);
                float v11 = lerp(c011, c111, tx);

                float v0 = lerp(v00, v10, ty);
                float v1 = lerp(v01, v11, ty);

                float noise3D = lerp(v0, v1, tz);

                float globalY = static_cast<float>(y);
                float falloff = (baseHeight - globalY) * 0.12f; 
                float finalDensity = falloff + noise3D;

                if (finalDensity > 0.0f) {
                    if (depth == 0) {
                        if (currentBiome == Biome::DESERT) {
                            blocks[index] = BlockType::SAND;
                        } else if (currentBiome == Biome::MOUNTAIN && y > 100) {
                            blocks[index] = BlockType::STONE;
                        } else {
                            blocks[index] = (y <= WATER_LEVEL + 1) ? BlockType::SAND : BlockType::GRASS;
                            if (highestGrassY == -1 && blocks[index] == BlockType::GRASS) highestGrassY = y;
                        }
                    } else if (depth < 4) {
                        if (currentBiome == Biome::DESERT || y <= WATER_LEVEL) {
                            blocks[index] = BlockType::SAND;
                        } else {
                            blocks[index] = BlockType::DIRT;
                        }
                    } else {
                        blocks[index] = BlockType::STONE;
                    }
                    depth++; 
                } else {
                    if (y <= WATER_LEVEL) {
                        blocks[index] = BlockType::WATER;
                    } else {
                        blocks[index] = BlockType::AIR;
                    }
                    depth = 0; 
                }
            }

            bool isAwayFromEdge = (x >= 2 && x < CHUNK_WIDTH - 2 && z >= 2 && z < CHUNK_DEPTH - 2);

            if (isAwayFromEdge && highestGrassY > WATER_LEVEL && currentBiome == Biome::PLAINS) {
                if ((std::rand() % 1000) < m_config.treeChance) {
                    treesToGenerate.push_back(glm::ivec3(x, highestGrassY + 1, z));
                }
            }
        }
    }
    
    for (const auto& pos : treesToGenerate) {
        GenerateTree(pos.x, pos.y, pos.z, blocks);
    }

    const std::vector<MineralConfig> mineralRules = {
        { BlockType::COAL,     80, 5, 45,  10 },
        { BlockType::IRON,     25, 5, 30,  10 },
        { BlockType::GOLD,      5, 5, 20,   8 },
        { BlockType::DIAMOND,   1, 2, 10,   6 },
        { BlockType::LAVA,      2, 5, 15,  20 }
    };

    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                int globalX = chunkX * CHUNK_WIDTH + x;
                int globalY = y;
                int globalZ = chunkZ * CHUNK_DEPTH + z;

                for (const auto& rule : mineralRules) {
                    if (y >= rule.minHeight && y <= rule.maxHeight) {
                        if (ShouldPlaceVein(rule.probaValue, globalX, globalY, globalZ)) {
                            GenerateMineralVein(rule.type, x, y, z, rule.maxBlocksPerVein, blocks);
                            break; 
                        }
                    }
                }
            }
        }
    }

    std::array<std::vector<BlockType>, 16> subChunks;

    for (int i = 0; i<16; ++i) {
        subChunks[i].resize(CHUNK_WIDTH * 16 * CHUNK_DEPTH);
    }
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        int subY = y / 16;
        int localY = y % 16;

        for (int z = 0; z < CHUNK_DEPTH; ++z) {
            for (int x = 0; x < CHUNK_WIDTH; ++x) {
                int flatIndex = GetIndex(x,y,z);
                int subIndex = x + (z * CHUNK_WIDTH) + (localY * CHUNK_WIDTH * CHUNK_DEPTH);

                subChunks[subY][subIndex] = blocks[flatIndex];
            }
        }
    }

    return subChunks;
}
void TerrainGenerator::GenerateTree(int startX, int startY, int startZ, std::vector<BlockType>& blocks) const {
    int trunkHeight = 4 + (std::rand() % 3);

    for (int y = 0; y < trunkHeight; ++y) {
        int currentY = startY + y;
        if (currentY >= CHUNK_HEIGHT) break; 
        
        int index = GetIndex(startX, currentY, startZ);
        blocks[index] = BlockType::WOOD;
    }

    int leafStartY = startY + 2;
    int leafEndY = startY + trunkHeight;

    for (int y = leafStartY; y <= leafEndY; ++y) {
        int currentRadius = 2;
        if (y >= leafEndY - 1) {
            currentRadius = 1;
        }
        for (int x = startX - currentRadius; x <= startX + currentRadius; ++x) {
            for (int z = startZ - currentRadius; z <= startZ + currentRadius; ++z) {
                
                if (x >= 0 && x < CHUNK_WIDTH &&
                    y >= 0 && y < CHUNK_HEIGHT &&
                    z >= 0 && z < CHUNK_DEPTH) {
                    
                    if (currentRadius == 2 && std::abs(x - startX) == 2 && std::abs(z - startZ) == 2) {
                        if ((std::rand() % 100) < 50) continue;
                    }
                    int index = GetIndex(x, y, z);
                    if (blocks[index] == BlockType::AIR) {
                        blocks[index] = BlockType::LEAVES;
                    }
                }
            }
        }
    }
}

void TerrainGenerator::GenerateMineralVein(BlockType mineral, int startX, int startY, int startZ, int maxBlock, std::vector<BlockType>& blocks) const {
    int currentX = startX;
    int currentY = startY;
    int currentZ = startZ;

    for (int i = 0; i < maxBlock; ++i) {
        if (currentX >= 0 && currentX < CHUNK_WIDTH && 
            currentY >= 0 && currentY < CHUNK_HEIGHT && 
            currentZ >= 0 && currentZ < CHUNK_DEPTH) {
            int index = GetIndex(currentX, currentY, currentZ);
            if (blocks[index] == BlockType::STONE) {
                blocks[index] = mineral;
            }
        }
        int direction = std::rand() % 6;
        if (direction == 0) currentX--;
        else if (direction == 1) currentX++;
        else if (direction == 2) currentY--;
        else if (direction == 3) currentY++;
        else if (direction == 4) currentZ--;
        else if (direction == 5) currentZ++;
    }
}

bool TerrainGenerator::ShouldPlaceVein(int threshold, int x, int y, int z) const {
    int hash = m_seed + x * 73856093 ^ y * 19349663 ^ z * 83492791;
    int value = std::abs(hash % 10000); 
    return value < threshold;
}
