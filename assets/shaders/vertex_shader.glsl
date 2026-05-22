#version 330 core

// Input vertex data
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec3 vertices_normal;
layout(location = 2) in vec3 texCoords;

// Output vertex data
out vec3 vTexCoords;

// Uniforms
uniform mat4 MVP;

void main(){
    gl_Position = MVP * vec4(vertices_position_modelspace, 1);
    vTexCoords = texCoords;
}
