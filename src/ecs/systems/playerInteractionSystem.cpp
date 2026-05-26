#include "playerInteractionSystem.hpp"

RaycastResult PlayerInteractionSystem::raycast(Registry& _registry, TerrainSystem& _terrain, const glm::vec3& _start, const glm::vec3& _direction, float _reach) const {
    RaycastResult result;
    glm::vec3 direction = glm::normalize(_direction);

    int x = std::floor(_start.x);
    int y = std::floor(_start.y);
    int z = std::floor(_start.z);

    int stepX = (direction.x > 0) ? 1 : ((direction.x < 0) ? -1 : 0);
    int stepY = (direction.y > 0) ? 1 : ((direction.y < 0) ? -1 : 0);
    int stepZ = (direction.z > 0) ? 1 : ((direction.z < 0) ? -1 : 0);

    float tMaxX = (stepX != 0) ? (x + (stepX > 0 ? 1 : 0) - _start.x) / direction.x : std::numeric_limits<float>::infinity();
    float tMaxY = (stepY != 0) ? (y + (stepY > 0 ? 1 : 0) - _start.y) / direction.y : std::numeric_limits<float>::infinity();
    float tMaxZ = (stepZ != 0) ? (z + (stepZ > 0 ? 1 : 0) - _start.z) / direction.z : std::numeric_limits<float>::infinity();

    float tDeltaX = (stepX != 0) ? std::abs(1.0f / direction.x) : std::numeric_limits<float>::infinity();
    float tDeltaY = (stepY != 0) ? std::abs(1.0f / direction.y) : std::numeric_limits<float>::infinity();
    float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / direction.z) : std::numeric_limits<float>::infinity();

    glm::ivec3 normal(0);

    while (true) {
        int chunkX = std::floor(static_cast<float>(x) / 16.0f);
        int chunkY = std::floor(static_cast<float>(y) / 16.0f);
        int chunkZ = std::floor(static_cast<float>(z) / 16.0f);
        
        EntityID chunkEntity = _terrain.getSubChunkAt(chunkX, chunkY, chunkZ, _registry);
        
        if (chunkEntity != 0 && _registry.hasComponent<SubChunkComponent>(chunkEntity)) {
            const auto& chunk = _registry.getComponent<SubChunkComponent>(chunkEntity);
            int localX = (x % 16 + 16) % 16;
            int localY = (y % 16 + 16) % 16;
            int localZ = (z % 16 + 16) % 16;
            
            VoxelType type = chunk.getVoxel(localX, localY, localZ);
            
            if (type != VoxelType::AIR && type != VoxelType::WATER && type != VoxelType::LAVA) {
                result.hit = true;
                result.hitVoxelPos = glm::ivec3(x, y, z);
                result.normal = normal;
                result.chunkEntity = chunkEntity;
                return result;
            }
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                if (tMaxX > _reach) break;
                x += stepX;
                tMaxX += tDeltaX;
                normal = glm::ivec3(-stepX, 0, 0);
            } else {
                if (tMaxZ > _reach) break;
                z += stepZ;
                tMaxZ += tDeltaZ;
                normal = glm::ivec3(0, 0, -stepZ);
            }
        } else {
            if (tMaxY < tMaxZ) {
                if (tMaxY > _reach) break;
                y += stepY;
                tMaxY += tDeltaY;
                normal = glm::ivec3(0, -stepY, 0);
            } else {
                if (tMaxZ > _reach) break;
                z += stepZ;
                tMaxZ += tDeltaZ;
                normal = glm::ivec3(0, 0, -stepZ);
            }
        }
    }
    return result;
}

void PlayerInteractionSystem::update(Registry& _registry, TerrainSystem& _terrain) {
    auto view = _registry.view<PlayerComponent, TransformComponent, InputReceiverComponent>();

    for (EntityID entity : view) {

        if (_registry.hasComponent<CameraComponent>(entity)) {
            auto& player = _registry.getComponent<PlayerComponent>(entity);
            auto& camera = _registry.getComponent<CameraComponent>(entity);
            auto& transform = _registry.getComponent<TransformComponent>(entity);
            auto& input = _registry.getComponent<InputReceiverComponent>(entity);

            if (camera.isActive) {
                if (input.leftClick && player.canBreakBlocks) {
                    RaycastResult result = raycast(_registry, _terrain, transform.position + camera.offset, camera.front, player.reach);

                    if (result.hit) {
                        _terrain.setBlock(_registry, result.hitVoxelPos.x, result.hitVoxelPos.y, result.hitVoxelPos.z, VoxelType::AIR);
                    }  

                    input.leftClick = false;
                }
                if (input.rightClick) {
                    RaycastResult result = raycast(_registry, _terrain, transform.position + camera.offset, camera.front, player.reach);
                    if (result.hit) {
                        glm::ivec3 placePos = result.hitVoxelPos + result.normal;

                        _terrain.setBlock(_registry, placePos.x, placePos.y, placePos.z, player.currentBloc);
                    }

                    input.rightClick = false;
                }
            }
        }
    }
}