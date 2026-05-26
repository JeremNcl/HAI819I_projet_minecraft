#pragma once
#include "chunk.hpp"
#include "component.hpp"
#include <map>


struct InventoryComponent : public Component {
    std::map<VoxelType, int> items;
    VoxelType selectedBlock = VoxelType::GRASS;
    bool isOpen = false;
    
    InventoryComponent() = default;
};