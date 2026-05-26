#pragma once
#ifndef CHUNKSERIALIZER_HPP
#define CHUNKSERIALIZER_HPP

#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include "ecs/components/chunk.hpp"

enum class VoxelType : uint8_t; 

struct ChunkDataDTO {
    int x = 0;
    int z = 0;
    uint16_t subChunkMask = 0;
    std::array<std::vector<uint8_t>, 16> subChunksVoxels;
    std::array<glm::vec3, 16 * 16> biomeColors;
};

class ChunkSerializer {
    public:
        static std::string getFilePath(int _x, int _z) {
            std::filesystem::create_directories("saves");
            return "saves/chunk_" + std::to_string(_x) + "_" + std::to_string(_z) + ".bin";
        }

        static void SaveChunk(const ChunkDataDTO& _chunk) {
            std::ofstream file(getFilePath(_chunk.x, _chunk.z), std::ios::binary);
            if (!file.is_open()) return;

            // 1. Écriture de l'en-tête (Position et Masque)
            file.write(reinterpret_cast<const char*>(&_chunk.x), sizeof(_chunk.x));
            file.write(reinterpret_cast<const char*>(&_chunk.z), sizeof(_chunk.z));
            file.write(reinterpret_cast<const char*>(&_chunk.subChunkMask), sizeof(_chunk.subChunkMask));

            // 2. Écriture des biomes (256 * vec3 = 3072 octets)
            file.write(reinterpret_cast<const char*>(_chunk.biomeColors.data()), _chunk.biomeColors.size() * sizeof(glm::vec3));

            // 3. Écriture unique des sous-chunks actifs
            for (int i = 0; i < 16; ++i) {
                if (_chunk.subChunkMask & (1 << i)) {
                    // Sauvegarde brute des 4096 octets du sous-chunk
                    file.write(reinterpret_cast<const char*>(_chunk.subChunksVoxels[i].data()), 4096);
                }
            }
            file.close();
        }

        static bool LoadChunk(int _x, int _z, ChunkDataDTO& _outChunk) {
        std::ifstream file(getFilePath(_x, _z), std::ios::binary);
        if (!file.is_open()) return false;

        _outChunk.x = _x;
        _outChunk.z = _z;

        file.read(reinterpret_cast<char*>(&_outChunk.x), sizeof(_outChunk.x));
        file.read(reinterpret_cast<char*>(&_outChunk.z), sizeof(_outChunk.z));
        file.read(reinterpret_cast<char*>(&_outChunk.subChunkMask), sizeof(_outChunk.subChunkMask));

        file.read(reinterpret_cast<char*>(_outChunk.biomeColors.data()), _outChunk.biomeColors.size() * sizeof(glm::vec3));

        for (int i = 0; i < 16; ++i) {
            if (_outChunk.subChunkMask & (1 << i)) {
                _outChunk.subChunksVoxels[i].resize(4096, 0);
                file.read(reinterpret_cast<char*>(_outChunk.subChunksVoxels[i].data()), 4096);
            } else {
                _outChunk.subChunksVoxels[i].clear();
            }
        }

        file.close();
        return true;
    }
};

#endif