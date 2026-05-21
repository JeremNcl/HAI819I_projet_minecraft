#pragma once
#include <vector>
#include <cstdint>
#include <string>

enum class BlockType : std::uint8_t {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3,
    WOOD = 4,
    LEAVES = 5,
    COAL = 6,
    IRON = 7,
    GOLD = 8,
    DIAMOND = 9,
    LAVA = 10,
    SAND = 11,
    WATER = 12,
    BEDROCK = 13
};

enum class BiomeType { PLAINS, DESERT, MOUNTAIN, OCEAN };

struct MineralConfig {
    BlockType type;
    int probaValue;
    int minHeight;
    int maxHeight;
    int maxBlocksPerVein;
};

struct TerrainConfig {
    int seed = 12345;
    
    float terrainFreq = 0.005f;
    float caveFreq = 0.07f;
    float biomeFreq = 0.002f;
    
    int heightMountain = 50;
    int heightPlains = 30;
    int heightDesert = 27;
    int heightOcean = 25;
    float amplitude = 30.0f;
    
    float caveThreshold = -0.4f;
    int treeChance = 10;
};

TerrainConfig LoadConfig(const std::string& filename);

class TerrainGenerator {
public:
    static constexpr int CHUNK_WIDTH = 16;
    static constexpr int CHUNK_HEIGHT = 256;
    static constexpr int CHUNK_DEPTH = 16;

    TerrainGenerator(const TerrainConfig& config);
    
    int GetIndex(int x, int y, int z) const;
    std::vector<BlockType> GenerateChunk(int chunkX, int chunkZ);
    
private:
    int m_seed;
    TerrainConfig m_config;
    
    void GenerateTree(int startX, int startY, int startZ, std::vector<BlockType>& blocks) const;
    void GenerateMineralVein(BlockType mineral, int startX, int startY, int startZ, int maxBlock, std::vector<BlockType>& blocks) const;
    bool ShouldPlaceVein(int threshold, int x, int y, int z) const;
};
