#include "TerrainGenerator.hpp"
#include "../../../external/FastNoiseLite.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>


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
            if (key == "seed") config.seed = (int)value;
            else if (key == "terrain_freq") config.terrainFreq = value;
            else if (key == "cave_freq") config.caveFreq = value;
            else if (key == "biome_freq") config.biomeFreq = value;
            else if (key == "height_mountain") config.heightMountain = (int)value;
            else if (key == "height_plains") config.heightPlains = (int)value;
            else if (key == "height_desert") config.heightDesert = (int)value;
            else if (key == "height_ocean") config.heightOcean = (int)value;
            else if (key == "amplitude") config.amplitude = value;
            else if (key == "cave_threshold") config.caveThreshold = value;
            else if (key == "tree_chance") config.treeChance = (int)value;
        }
    }
    
    std::cout << "Configuration chargee depuis " << filename << " avec succes.\n";
    return config;
}

//======================================================================================================================
//======================================================================================================================

TerrainGenerator::TerrainGenerator(const TerrainConfig& config) {
    m_config = config;
    m_seed = config.seed;
}

int TerrainGenerator::GetIndex(int x, int y, int z) const{
    return x + (z * CHUNK_WIDTH) + (y * CHUNK_DEPTH * CHUNK_WIDTH);
}

//======================================================================================================================
//======================================================================================================================

std::vector<BlockType> TerrainGenerator::GenerateChunk(int chunkX, int chunkZ) {
    std::vector<BlockType> blocks(CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH, BlockType::AIR);
    //const int BASE_HEIGHT = 30; 

    FastNoiseLite terrainNoise;
    terrainNoise.SetSeed(m_seed);
    terrainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite caveNoise;
    caveNoise.SetSeed(m_seed);
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(m_seed);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    terrainNoise.SetFrequency(m_config.terrainFreq);
    caveNoise.SetFrequency(m_config.caveFreq);
    biomeNoise.SetFrequency(m_config.biomeFreq);
    

    for (int z = 0; z < CHUNK_DEPTH; ++z) {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            
            float globalX = (chunkX * CHUNK_WIDTH) + x;
            float globalZ = (chunkZ * CHUNK_DEPTH) + z;

            float noiseValue = terrainNoise.GetNoise(globalX, globalZ);
            float biomeValue = biomeNoise.GetNoise(globalX, globalZ);

            float dW = std::max(0.0f, 1.0f - std::abs(biomeValue + 0.5f) / 0.5f);
            float mW = std::max(0.0f, 1.0f - std::abs(biomeValue - 0.5f) / 0.5f);
            float pW = 1.0f - dW - mW;

            int localHeight = (int)(dW * m_config.heightDesert + 
                                    pW * m_config.heightPlains + 
                                    mW * m_config.heightMountain);

            int terrainHeight = localHeight + static_cast<int>(noiseValue * m_config.amplitude);
            terrainHeight = std::max(0, std::min(terrainHeight, CHUNK_HEIGHT - 1));

            BlockType surfaceBlock = BlockType::GRASS;
            if (dW > 0.6f) surfaceBlock = BlockType::SAND;
            else if (mW > 0.6f) surfaceBlock = BlockType::STONE;

            for (int y = 0; y <= terrainHeight; ++y) {
                int index = GetIndex(x, y, z);

                int hash = m_seed + x * 9999 + z * 8888;
                int bedrockHeight = (hash % 2);

                if (y == bedrockHeight) {
                    blocks[index] = BlockType::BEDROCK;
                    continue;
                }

                float globalY = y;
                float density = caveNoise.GetNoise(globalX, globalY, globalZ);

                if (density < -0.4f && y < terrainHeight - 3) {
                    blocks[index] = BlockType::AIR;
                    continue; 
                }

                if (y == terrainHeight) {
                    blocks[index] = surfaceBlock;
                } 
                else if (y > terrainHeight - 4) {
                    blocks[index] = BlockType::DIRT;
                } 
                else {
                    blocks[index] = BlockType::STONE;
                }
            }
        }
    }

    int chunkSeed = m_seed + (chunkX * 12345) + (chunkZ * 67890);
    std::srand(chunkSeed);

    for (int z = 0; z < CHUNK_DEPTH; ++z) {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            int surfaceY = 0;
            for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
                if (blocks[GetIndex(x, y, z)] == BlockType::GRASS) {
                    surfaceY = y;
                    break;
                }
            }
            if (surfaceY > 0 && (std::rand() % 100) < 10) {
                GenerateTree(x, surfaceY + 1, z, blocks); 
            }
        }
    }
    const std::vector<MineralConfig> mineralRules = {
        { BlockType::COAL,     80,5, 45,  10 },
        { BlockType::IRON,    25, 5, 30,  10 },
        { BlockType::GOLD,      5, 5, 20,  8 },
        { BlockType::DIAMOND,    1,2, 10,  6 },
        { BlockType::LAVA,       2,5, 15,  20 }
    };

    for (int y = 0; y < CHUNK_HEIGHT; ++y){
        for (int z = 0; z < CHUNK_DEPTH; ++z){
            for (int x = 0; x < CHUNK_WIDTH; ++x){
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

    return blocks;
}

//======================================================================================================================
//======================================================================================================================

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

//======================================================================================================================
//======================================================================================================================

void TerrainGenerator::GenerateMineralVein(BlockType mineral, int startX, int startY, int startZ, int maxBlock, std::vector<BlockType> &blocks) const{
    int currentX = startX;
    int currentY = startY;
    int currentZ = startZ;

    for (int i = 0; i < maxBlock; ++i){
        if (currentX >= 0 && currentX < CHUNK_WIDTH && currentY >= 0 && currentY < CHUNK_HEIGHT && currentZ >= 0 && currentZ < CHUNK_DEPTH){
            int index = GetIndex(currentX, currentY, currentZ);
            if (blocks[index] == BlockType::STONE){
                blocks[index] = mineral;
            }
        }
        int direction = std::rand()%6;
        if (direction ==0) currentX--;
        else if (direction ==1) currentX++;
        else if (direction ==2) currentY--;
        else if (direction ==3) currentY++;
        else if (direction ==4) currentZ--;
        else if (direction ==5) currentZ++;
    }
    
}

//======================================================================================================================
//======================================================================================================================

bool TerrainGenerator::ShouldPlaceVein(int threshold, int x, int y, int z) const {

    int hash = m_seed + x * 73856093 ^ y * 19349663 ^ z * 83492791;
    int value = std::abs(hash % 10000); 
    
    return value < threshold;

}