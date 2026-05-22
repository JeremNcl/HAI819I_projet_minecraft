#ifndef TEXTURE_LOADER_HPP
#define TEXTURE_LOADER_HPP

#include <GL/glew.h>
#include <vector>
#include <string>

struct BlockTextures {
    GLuint colorMap;
    GLuint normalMap;
    GLuint metallicMap;
};

GLuint loadBMP_custom(const char * imagepath);

GLuint loadDDS(const char * imagepath);

GLuint loadTextureArray(const std::vector<std::string>& filepaths);

BlockTextures loadBlockTextures(const char* colorPath, 
                                 const char* normalPath, 
                                 const char* metallicPath);

#endif