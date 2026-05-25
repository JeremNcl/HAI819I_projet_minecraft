#version 330 core

in vec3 fragmentPos;
out vec4 FragColor;

uniform vec3 ambientSkyColor; 
uniform vec3 horizonColor;
uniform vec3 ambientGroundColor;

void main(){
    vec3 dir = normalize(fragmentPos);

    // 1. L'ÉPAISSEUR ATMOSPHÉRIQUE
    float a = clamp(dir.y, 0.0, 1.0);
    float atmosphericThickness = pow(1.0 - a, 4.0);
    
    // 2. LE CIEL
    vec3 finalColor = mix(ambientSkyColor, horizonColor, atmosphericThickness);

    // 3. LES ABYSSES (Sous la map)
    if (dir.y < 0.0) {
        float abyssFactor = smoothstep(0.0, -0.5, dir.y);
        vec3 deepAbyssColor = mix(horizonColor * 0.4, ambientGroundColor, 0.7);
        finalColor = mix(horizonColor, deepAbyssColor, abyssFactor);
    }

    FragColor = vec4(finalColor, 1.0);
}