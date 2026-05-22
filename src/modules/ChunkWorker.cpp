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

        std::vector<BlockType> data = m_generator.GenerateChunk(task.first, task.second);
        
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back({task.first, task.second, std::move(data), true});
    }
}