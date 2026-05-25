#ifndef SHADER_HPP
#define SHADER_HPP

struct ShaderUniforms {
    GLuint mvp;
    GLuint model;
    GLuint view;
    GLuint projection;
    GLuint normalMatrix;
    GLuint viewPos;
    GLuint lightPos;
    GLuint lightColor;
};

inline ShaderUniforms cacheUniformLocations(GLuint _program) {
    ShaderUniforms shaderUniforms;
    shaderUniforms.mvp = glGetUniformLocation(_program, "MVP");
    shaderUniforms.model = glGetUniformLocation(_program, "model");
    shaderUniforms.view = glGetUniformLocation(_program, "view");
    shaderUniforms.projection = glGetUniformLocation(_program, "projection");
    shaderUniforms.normalMatrix = glGetUniformLocation(_program, "normalMatrix");
    shaderUniforms.viewPos = glGetUniformLocation(_program, "viewPos");
    shaderUniforms.lightPos = glGetUniformLocation(_program, "lightPos");
    shaderUniforms.lightColor = glGetUniformLocation(_program, "lightColor");

    return shaderUniforms;
}

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);

#endif
