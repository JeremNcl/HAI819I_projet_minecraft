#version 330 core

// === INPUT FROM VERTEX SHADER ===
in VS_OUT {
    vec3 FragPos;
    vec3 TexCoords;
    vec3 Normal;
    mat3 TBN;
} fs_in;

// === OUTPUT ===
out vec4 FragColor;

// === TEXTURE SAMPLERS ===
uniform sampler2DArray colorMap;
uniform sampler2DArray normalMap;
uniform sampler2DArray metallicMap;

// === PBR PARAMETERS ===
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// === PBR CONSTANTS ===
const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, 0.0000001);
}

// Fresnel-Schlick Approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

void main() {
    // Sample textures using XYZ coordinates (Z is the texture index)
    vec3 uvw = fs_in.TexCoords;
    vec3 albedo = texture(colorMap, uvw).rgb;
    vec3 normal = texture(normalMap, uvw).rgb;
    vec3 metallicSample = texture(metallicMap, uvw).rgb;

    // On applique le colorant de biome AVANT la correction Gamma
    if (uvw.z >= 1.9) {
        // Vrai vert Minecraft (sRGB : 124, 189, 75)
        vec3 biomeColor = vec3(124.0/255.0, 189.0/255.0, 75.0/255.0);
        albedo *= biomeColor;
    }

    // Correction de l'albedo (sRGB to Linear)
    albedo = pow(albedo, vec3(2.2));
    
    // Convert normal map from [0,1] to [-1,1]
    normal = normalize(normal * 2.0 - 1.0);
    // Transform normal to world space using TBN
    normal = normalize(fs_in.TBN * normal);
    
    // DECODAGE FORMAT BEDROCK RTX (MER) :
    // R = Metallic, G = Emission, B = Roughness
    
    // Forçons un seuil très bas pour le métal pour éviter les points noirs de la dirt et autres matériaux non métalliques
    float metallic = metallicSample.r > 0.5 ? 1.0 : 0.0;
    
    float emission = metallicSample.g;
    
    // On lit la Roughness directement, SANS l'inverser (Standard Vanilla RTX strict)
    float roughness = metallicSample.b;
    
    // Clamp roughness to avoid artifacts
    roughness = clamp(roughness, 0.05, 1.0);
    
    // View direction
    vec3 V = normalize(viewPos - fs_in.FragPos);
    vec3 N = normal;
    
    // Calculate F0 (base reflectivity)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Lighting calculations
    vec3 Lo = vec3(0.0);
    
    // === SINGLE DIRECTIONAL LIGHT ===
    {
        vec3 L = normalize(lightPos); // La lumière vient de cette direction à l'infini
        vec3 H = normalize(V + L);
        vec3 radiance = lightColor;   // Pas d'atténuation basée sur la distance pour un soleil
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Ambient lighting (simple)
    vec3 ambient = vec3(0.03) * albedo; // modif temporaire
    
    vec3 color = ambient + Lo;
    
    // Ajout de l'émission (pour les blocs lumineux)
    color += albedo * emission * 5.0;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    // AFFICHER LE TBN POUR DEBUG :
    // R = Tangente (Axe X de la texture)
    // G = Bitangente (Axe Y de la texture)
    // B = Normale (Axe Z de la texture)
    FragColor = vec4(normalize(fs_in.TBN[0]) * 0.5 + 0.5, 1.0); // Affiche la Tangente
    // FragColor = vec4(normalize(fs_in.TBN[1]) * 0.5 + 0.5, 1.0); // Affiche la Bitangente
    // FragColor = vec4(normalize(fs_in.TBN[2]) * 0.5 + 0.5, 1.0); // Affiche la Normale
    
    //FragColor = vec4(color, 1.0);
}

