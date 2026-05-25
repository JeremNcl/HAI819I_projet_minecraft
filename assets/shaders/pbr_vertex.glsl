#version 330 core

// === INPUT VERTEX DATA ===
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec3 vertices_normal;
layout(location = 2) in vec3 vertices_uv;
layout(location = 3) in vec3 vertices_tangent;
layout(location = 4) in vec3 vertices_bitangent;
layout(location = 5) in vec3 vertices_biomeColor;
layout(location = 6) in float vertices_ao;

// === OUTPUT TO FRAGMENT SHADER ===
out VS_OUT {
    vec3 FragPos;
    vec3 TexCoords;
    vec3 Normal;
    vec3 BiomeColor;
    float AO;
    mat3 TBN;
} vs_out;

// === UNIFORMS ===
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main(){
    // Transform position to world space
    vs_out.FragPos = vec3(model * vec4(vertices_position_modelspace, 1.0));
    
    // Pass texture coordinates (xy) and texture index (z)
    vs_out.TexCoords = vertices_uv;
    vs_out.BiomeColor = vertices_biomeColor;
    vs_out.AO = vertices_ao;
    
    // Transform normal to world space
    vs_out.Normal = normalize(normalMatrix * vertices_normal);
    
    // Canonical TBN: tangent-space basis comes from the mesh itself.
    vec3 N = normalize(normalMatrix * vertices_normal);
    vec3 T = normalize(normalMatrix * vertices_tangent);
    vec3 B = normalize(normalMatrix * vertices_bitangent);

    // Gram-Schmidt orthonormalization to stabilize interpolation artifacts.
    T = normalize(T - dot(T, N) * N);
    B = normalize(B - dot(B, N) * N);

    if (dot(cross(T, B), N) < 0.0) {
        B = -B;
    }
    
    vs_out.TBN = mat3(T, B, N);
    
    // Transform to clip space
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
