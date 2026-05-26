#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <atomic>
#include <glm/glm.hpp>
#include "terrain_gen/TerrainGenerator.hpp"
#include <array>
#include "ChunkSerializer.hpp"

struct ChunkTask {
    int x;
    int z;
    bool ready = false;
    ChunkDataDTO chunkData;
};

class ChunkWorker {
private:
    TerrainGenerator m_generator;
    std::thread workerThread;
    std::mutex queueMutex;
    std::queue<std::pair<int, int>> taskQueue;
    
    std::vector<ChunkTask> results;
    std::mutex resultsMutex;
    std::atomic<bool> running; 

    void processTasks();

public:
    ChunkWorker(const TerrainConfig& config);
    ~ChunkWorker();

    void requestChunk(int x, int z);
    
    bool popResult(ChunkTask& outTask);
};