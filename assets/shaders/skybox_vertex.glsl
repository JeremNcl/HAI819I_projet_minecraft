#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;

out vec3 fragmentPos;

uniform mat4 view;
uniform mat4 projection;

void main(){
    // Use view matrix but remove translation (skybox is infinitely far away)
    mat4 viewNoTranslate = mat4(mat3(view));
    
    vec4 pos = projection * viewNoTranslate * vec4(vertexPosition_modelspace, 1.0);
    
    // Ensure skybox is always at far plane (z = w for depth = 1.0)
    gl_Position = pos.xyww;
    
    fragmentPos = vertexPosition_modelspace;
}
