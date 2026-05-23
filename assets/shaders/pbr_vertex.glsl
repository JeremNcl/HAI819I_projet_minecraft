#version 330 core

// === INPUT VERTEX DATA ===
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec3 vertices_normal;
layout(location = 2) in vec3 vertices_uv;
layout(location = 3) in vec3 vertices_tangent;
// layout(location = 4) in vec3 vertices_bitangent;

// === OUTPUT TO FRAGMENT SHADER ===
out VS_OUT {
    vec3 FragPos;
    vec3 TexCoords;
    vec3 Normal;
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
    
    // Transform normal to world space
    vs_out.Normal = normalize(normalMatrix * vertices_normal);
    
    // Compute TBN matrix for normal mapping
    vec3 N = vs_out.Normal;
    
    // On utilise la tangente fournie par le Meshing System
    vec3 T = normalize(normalMatrix * vertices_tangent);
    
    // Gram-Schmidt (Optionnel mais sécurisant pour garantir un angle de 90°)
    T = normalize(T - dot(T, N) * N);
    
    // On déduit la Bitangente (avec le bon sens, si l'éclairage est bizarre, on fera T = cross(T, N))
    vec3 B = cross(N, T);
    
    vs_out.TBN = mat3(T, B, N);
    
    // Transform to clip space
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
