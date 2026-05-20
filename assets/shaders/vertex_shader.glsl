#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 uv;

// Output vertex data
out vec2 vUV;
out float vHeight;  // Passer la hauteur au fragment shader

//TODO create uniform transformations matrices Model View Projection
// Values that stay constant for the whole mesh.
uniform mat4 MVP;

void main(){

        // TODO : Output position of the vertex, in clip space : MVP * position
        gl_Position = MVP * vec4(vertices_position_modelspace,1);
        vUV = uv;
        vHeight = vertices_position_modelspace.y; // Hauteur du vertex
}
