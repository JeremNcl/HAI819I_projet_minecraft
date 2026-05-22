#version 330 core

// Input data
in vec3 vTexCoords;

// Output data
out vec4 color;

uniform sampler2DArray textureSampler;

void main(){
    color = texture(textureSampler, vTexCoords);
}
