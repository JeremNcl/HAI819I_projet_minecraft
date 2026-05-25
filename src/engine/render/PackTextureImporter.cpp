#include "PackTextureImporter.hpp"

#include "engine/io/stb_image.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>

namespace {
std::string joinPath(const std::string& directory, const std::string& name) {
    return directory + "/" + name + ".tga";
}

std::string readWholeFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
} // namespace

bool PackTextureImporter::readFile(const std::string& path, std::string& contents) {
    contents = readWholeFile(path);
    return !contents.empty();
}

bool PackTextureImporter::extractStringField(const std::string& contents,
                                             const std::string& fieldName,
                                             std::string& value) {
    const std::regex pattern("\"" + fieldName + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (!std::regex_search(contents, match, pattern) || match.size() < 2) {
        return false;
    }

    value = match[1].str();
    return true;
}

float PackTextureImporter::estimateNormalStrength(const std::string& normalTexturePath) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(normalTexturePath.c_str(), &width, &height, &channels, 4);
    if (!data || width <= 0 || height <= 0) {
        if (data) {
            stbi_image_free(data);
        }
        return 0.28f;
    }

    double deviationSum = 0.0;
    const int pixelCount = width * height;

    for (int pixel = 0; pixel < pixelCount; ++pixel) {
        const int offset = pixel * 4;
        const float nx = (static_cast<float>(data[offset + 0]) / 255.0f) * 2.0f - 1.0f;
        const float ny = (static_cast<float>(data[offset + 1]) / 255.0f) * 2.0f - 1.0f;
        deviationSum += std::sqrt(nx * nx + ny * ny);
    }

    stbi_image_free(data);

    const float averageDeviation = static_cast<float>(deviationSum / static_cast<double>(pixelCount));
    const float boostedDeviation = averageDeviation * 1.18f;
    const float strength = 0.25f + boostedDeviation * 0.72f;
    return std::clamp(strength, 0.25f, 0.83f);
}

bool PackTextureImporter::importTextureSet(const std::string& textureSetPath,
                                           const std::string& localTextureDirectory,
                                           ImportedTextureSet& outTextureSet) {
    std::string contents;
    if (!readFile(textureSetPath, contents)) {
        return false;
    }

    if (!extractStringField(contents, "color", outTextureSet.colorName)) {
        return false;
    }
    if (!extractStringField(contents, "metalness_emissive_roughness", outTextureSet.merName)) {
        return false;
    }
    if (!extractStringField(contents, "normal", outTextureSet.normalName)) {
        return false;
    }

    outTextureSet.sourceTextureSetPath = textureSetPath;
    outTextureSet.colorPath = joinPath(localTextureDirectory, outTextureSet.colorName);
    outTextureSet.merPath = joinPath(localTextureDirectory, outTextureSet.merName);
    outTextureSet.normalPath = joinPath(localTextureDirectory, outTextureSet.normalName);
    outTextureSet.normalStrength = estimateNormalStrength(outTextureSet.normalPath);
    return true;
}