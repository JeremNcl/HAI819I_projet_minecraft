#version 330 core

// Input data
in vec2 vUV;

// Ouput data
out vec3 color;

// Uniforms
uniform bool hasTexture;
uniform sampler2D textureSampler;


void main(){
    if (hasTexture) {
        vec3 tex = texture(textureSampler, vUV).rgb;
        color = tex;
    } else {
        color = vec3(0.2, 0.4, 0.8);
    }
}
