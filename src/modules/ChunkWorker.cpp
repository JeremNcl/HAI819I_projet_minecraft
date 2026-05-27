#include "ChunkWorker.hpp"
#include <chrono>

ChunkWorker::ChunkWorker(const TerrainConfig& config) : m_generator(config), running(true) {
    workerThread = std::thread(&ChunkWorker::processTasks, this);
}

ChunkWorker::~ChunkWorker() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void ChunkWorker::requestChunk(int x, int z) {
    std::lock_guard<std::mutex> lock(queueMutex);
    taskQueue.push({x, z});
}

bool ChunkWorker::popResult(ChunkTask& outTask) {
    std::lock_guard<std::mutex> lock(resultsMutex);
    if (results.empty()) return false;
    
    outTask = results.front();
    results.erase(results.begin());
    return true;
}

void ChunkWorker::processTasks() {
    while(running) {
        std::pair<int, int> task;
        bool hasTask = false;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if(!taskQueue.empty()) {
                task = taskQueue.front();
                taskQueue.pop();
                hasTask = true;
            }
        }
        
        if (!hasTask) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        ChunkTask resultTask;
        resultTask.x = task.first;
        resultTask.z = task.second;

        bool loadedFromDisk = ChunkSerializer::LoadChunk(task.first, task.second, resultTask.chunkData);

        if (!loadedFromDisk) {
            std::array<glm::vec3, TerrainGenerator::CHUNK_WIDTH * TerrainGenerator::CHUNK_DEPTH> rawBiomeColors;
            
            // Génération brute des blocs et récupération des couleurs du biome
            auto generatedData = m_generator.GenerateChunk(task.first, task.second, &rawBiomeColors);
            
            resultTask.chunkData.x = task.first;
            resultTask.chunkData.z = task.second;
            resultTask.chunkData.biomeColors = rawBiomeColors; // <-- Assignation des couleurs générées
            resultTask.chunkData.subChunkMask = 0;

            for (int subY = 0; subY < 16; ++subY) {
                resultTask.chunkData.subChunksVoxels[subY].resize(4096, 0);
                bool subChunkHasBlocks = false;

                for (int i = 0; i < 4096; ++i) {
                    BlockType genBlock = generatedData[subY][i];
                    VoxelType t = VoxelType::AIR;
                    
                    switch(genBlock) {
                        case BlockType::STONE:   t = VoxelType::STONE; break;
                        case BlockType::DIRT:    t = VoxelType::DIRT; break;
                        case BlockType::GRASS:   t = VoxelType::GRASS; break;
                        case BlockType::WOOD:    t = VoxelType::WOOD; break;
                        case BlockType::LEAVES:  t = VoxelType::LEAVES; break;
                        case BlockType::BEDROCK: t = VoxelType::BEDROCK; break;
                        case BlockType::COAL:    t = VoxelType::COAL; break;
                        case BlockType::IRON:    t = VoxelType::IRON; break;
                        case BlockType::GOLD:    t = VoxelType::GOLD; break;
                        case BlockType::DIAMOND: t = VoxelType::DIAMOND; break;
                        case BlockType::LAVA:    t = VoxelType::LAVA; break;
                        case BlockType::SAND:    t = VoxelType::SAND; break;
                        case BlockType::WATER:   t = VoxelType::WATER; break;
                        default:                 t = VoxelType::AIR; break;
                    }

                    resultTask.chunkData.subChunksVoxels[subY][i] = static_cast<uint8_t>(t);
                    if (t != VoxelType::AIR) {
                        subChunkHasBlocks = true;
                    }
                }

                if (subChunkHasBlocks) {
                    resultTask.chunkData.subChunkMask |= (1 << subY);
                } else {
                    resultTask.chunkData.subChunksVoxels[subY].clear();
                }
            }
        } else {
            if (resultTask.chunkData.biomeColors[0] == glm::vec3(0.0f)) {
                resultTask.chunkData.biomeColors.fill(glm::vec3(1.0f));
            }
        }

        resultTask.ready = true;

        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back(std::move(resultTask));
        }
    }
}