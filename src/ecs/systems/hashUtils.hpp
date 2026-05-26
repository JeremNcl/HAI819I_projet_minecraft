#pragma once
#ifndef GLM_VEC3_HASH_DEFINED
#define GLM_VEC3_HASH_DEFINED

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <vector> 

struct GLMVec3Hash {
    std::size_t operator()(const glm::ivec3& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
    }
};
#endif