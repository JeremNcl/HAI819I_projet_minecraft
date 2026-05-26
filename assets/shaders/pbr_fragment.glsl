#version 330 core

// === INPUT FROM VERTEX SHADER ===
in VS_OUT {
    vec3 FragPos;
    vec3 TexCoords;
    vec3 Normal;
    vec3 BiomeColor;
    float AO;
    mat3 TBN;
} fs_in;

// === OUTPUT ===
out vec4 FragColor;

// === TEXTURE SAMPLERS ===
uniform sampler2DArray colorMap;
uniform sampler2DArray normalMap;
uniform sampler2DArray metallicMap;
uniform float normalStrengths[16];

// === PBR PARAMETERS ===
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform vec3 ambientSkyColor;
uniform vec3 ambientGroundColor;
uniform bool useHemisphericalAmbient;
uniform bool useBakedAO;
uniform float aoStrength; // 0.0 = no AO effect, 1.0 = full baked AO influence in mix
uniform bool debugTBN;
uniform bool useNormalMap;
uniform bool debugDiffuseOnly;
uniform float exposure; // Dynamic exposure for tone mapping

const float HEMISPHERE_BLEND = 0.35;

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

float getNormalStrength(float sliceIndex) {
    int slice = int(clamp(sliceIndex + 0.5, 0.0, 15.0));
    return normalStrengths[slice];
}

void main() {
    vec3 uvw = fs_in.TexCoords;
    vec4 colorSample = texture(colorMap, uvw);
    
    // ==========================================
    // ALPHA CUTOUT (Transparence ciblée)
    // ==========================================
    const float kLeavesSlice = 8.0;
    
    // Si nous sommes en train de dessiner des feuilles (index 8)
    if (abs(uvw.z - kLeavesSlice) < 0.5) {
        // Et que le pixel est transparent (alpha < 10%)
        if (colorSample.a < 0.1) {
            discard; // On détruit le pixel immédiatement
        }
    }
    // ==========================================

    vec3 albedo = colorSample.rgb;
    vec3 normal = texture(normalMap, uvw).rgb;
    vec3 metallicSample = texture(metallicMap, uvw).rgb;

    // Index décalés de +1 suite à l'introduction du slot d'erreur en position 0
    const float kDirtSlice = 2.0;
    const float kGrassTopSlice = 3.0;
    const float kGrassSideSlice = 4.0;

    if (abs(uvw.z - kGrassTopSlice) < 0.5) {
        albedo = colorSample.rgb * fs_in.BiomeColor;
    } else if (abs(uvw.z - kGrassSideSlice) < 0.5) {
        vec3 dirtAlbedo = texture(colorMap, vec3(uvw.xy, kDirtSlice)).rgb;
        float grassMask = colorSample.a;
        vec3 grassyAlbedo = colorSample.rgb * fs_in.BiomeColor;
        albedo = mix(dirtAlbedo, grassyAlbedo, grassMask);
    } else if (abs(uvw.z - kLeavesSlice) < 0.5) {
        albedo = colorSample.rgb * fs_in.BiomeColor;
    }

    // Correction de l'albedo (sRGB to Linear)
    albedo = pow(albedo, vec3(2.2));

    if (useNormalMap) {
        // Convert normal map from [0,1] to [-1,1]
        vec3 tangentNormal = normal * 2.0 - 1.0;
        float normalStrength = getNormalStrength(uvw.z);
        tangentNormal = normalize(mix(vec3(0.0, 0.0, 1.0), tangentNormal, normalStrength));
        // Transform normal to world space using TBN
        normal = normalize(fs_in.TBN * tangentNormal);
    } else {
        normal = normalize(fs_in.Normal);
    }
    
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
        vec3 L = normalize(lightPos);
        // La lumière vient de cette direction à l'infini
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

    if (debugDiffuseOnly) {
        vec3 debugColor = albedo * (0.15 + 0.85 * max(dot(N, normalize(lightPos)), 0.0));
        FragColor = vec4(pow(debugColor, vec3(1.0/2.2)), 1.0);
        return;
    }
    
    // Ambient lighting
    vec3 ambient = vec3(ambientStrength) * albedo;
    if (useHemisphericalAmbient) {
        vec3 up = vec3(0.0, 1.0, 0.0);
        float ndotUp = clamp(dot(N, up), -1.0, 1.0);
        float hemiT = ndotUp * 0.5 + 0.5; // remap [-1,1] -> [0,1]
        vec3 hemiColor = mix(ambientGroundColor, ambientSkyColor, hemiT);
        vec3 hemiAmbient = hemiColor * ambientStrength * albedo;
        ambient = mix(ambient, hemiAmbient, HEMISPHERE_BLEND);
    }
    
    // Apply baked AO only to ambient lighting (contact shadows), not to direct lighting Lo.
    float aoFactor = 1.0;
    if (useBakedAO) {
        aoFactor = mix(1.0, fs_in.AO, aoStrength);
    }

    vec3 ambientWithAO = ambient * aoFactor;
    vec3 color = ambientWithAO + Lo;
    
    // Ajout de l'émission (pour les blocs lumineux)
    color += albedo * emission * 5.0;

    // Apply dynamic exposure before tone mapping
    color *= exposure;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    if (debugTBN) {
        // R = Tangente, G = Bitangente, B = Normale
        FragColor = vec4(normalize(fs_in.TBN[0]) * 0.5 + 0.5, 1.0);
    } else {
        FragColor = vec4(color, 1.0);
    }
}