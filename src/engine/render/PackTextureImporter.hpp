#pragma once

#include <string>

struct ImportedTextureSet {
    std::string sourceTextureSetPath;
    std::string colorName;
    std::string merName;
    std::string normalName;
    std::string colorPath;
    std::string merPath;
    std::string normalPath;
    float normalStrength = 0.28f;
};

class PackTextureImporter {
public:
    static bool importTextureSet(const std::string& textureSetPath,
                                 const std::string& localTextureDirectory,
                                 ImportedTextureSet& outTextureSet);

private:
    static bool readFile(const std::string& path, std::string& contents);
    static bool extractStringField(const std::string& contents,
                                   const std::string& fieldName,
                                   std::string& value);
    static float estimateNormalStrength(const std::string& normalTexturePath);
};