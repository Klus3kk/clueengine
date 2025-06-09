#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 vertexColor;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    int type; // 0=directional, 1=point, 2=spot
};

uniform Light lights[10];
uniform int lightCount;
uniform sampler2D texture1;
uniform vec3 viewPos;
uniform bool useTexture;
uniform bool useLighting;
uniform bool useColor;
uniform bool noShading;
uniform bool usePBR;

// PBR uniforms
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// Shadow mapping uniforms
uniform bool enableShadows;
uniform float shadowBias;
uniform sampler2D shadowMap[8];
uniform samplerCube pointShadowMaps[8];
uniform mat4 lightSpaceMatrix[8];
uniform vec3 pointLightPositions[8];
uniform float pointLightFarPlane[8];

// Shadow calculation functions
float ShadowCalculation(vec4 fragPosLightSpace, sampler2D shadowMap)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if fragment is outside light's frustum
    if(projCoords.z > 1.0)
        return 0.0;
    
    // Get current depth
    float currentDepth = projCoords.z;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Calculate bias (reduce shadow acne)
    vec3 normal = normalize(Normal);
    vec3 lightColor = lights[0].color; // Assuming first light for simplicity
    vec3 lightDir = normalize(lights[0].position - FragPos);
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias/10.0);
    
    // PCF (Percentage-Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

float PointShadowCalculation(vec3 fragPos, vec3 lightPos, samplerCube depthMap, float farPlane)
{
    // Get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    
    // Get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    
    // Sample the depth from the cube map
    float closestDepth = texture(depthMap, fragToLight).r;
    
    // Convert back to original depth value
    closestDepth *= farPlane;
    
    // Calculate bias
    float bias = shadowBias;
    
    // Simple shadow test
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

vec3 calculateLighting(vec3 norm, vec3 viewDir, vec3 albedo, float metallic, float roughness, float ao) {
    vec3 ambientLightIntensity = vec3(0.3, 0.3, 0.3);
    vec3 result = vec3(0.0);
    norm = normalize(norm);
    viewDir = normalize(viewDir);

    for (int i = 0; i < lightCount; i++) {
        Light light = lights[i];
        vec3 lightDir;
        float attenuation = 1.0;
        vec3 diffuse;
        vec3 specular;
        float shadow = 0.0;

        // Calculate light direction and attenuation based on light type
        if (light.type == 0) { // Directional light
            lightDir = normalize(-light.direction);
            attenuation = 1.0;
            
            // Calculate shadow for directional light
            if (enableShadows && i < 8) {
                vec4 fragPosLightSpace = lightSpaceMatrix[i] * vec4(FragPos, 1.0);
                shadow = ShadowCalculation(fragPosLightSpace, shadowMap[i]);
            }
        }
        else if (light.type == 1) { // Point light
            lightDir = normalize(light.position - FragPos);
            float distance = length(light.position - FragPos);
            attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            
            // Calculate shadow for point light
            if (enableShadows && i < 8) {
                shadow = PointShadowCalculation(FragPos, light.position, pointShadowMaps[i], pointLightFarPlane[i]);
            }
        }
        else if (light.type == 2) { // Spot light
            lightDir = normalize(light.position - FragPos);
            float distance = length(light.position - FragPos);
            
            // Spotlight intensity calculation
            float theta = dot(lightDir, normalize(-light.direction));
            float epsilon = light.cutOff - light.outerCutOff;
            float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
            
            attenuation = intensity / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            
            // Calculate shadow for spot light
            if (enableShadows && i < 8) {
                vec4 fragPosLightSpace = lightSpaceMatrix[i] * vec4(FragPos, 1.0);
                shadow = ShadowCalculation(fragPosLightSpace, shadowMap[i]);
            }
        }

        // Calculate diffuse component
        float diff = max(dot(norm, lightDir), 0.0);
        diffuse = diff * light.color * albedo;

        // Calculate specular component
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 2.0 / (roughness + 0.0001));
        float kSpecular = (metallic + (1.0 - metallic) * pow(1.0 - max(dot(viewDir, halfwayDir), 0.0), 5.0));
        specular = spec * light.color * kSpecular;

        // Apply shadow factor
        vec3 lighting = (diffuse + specular) * attenuation * light.intensity;
        lighting *= (1.0 - shadow);

        result += lighting;
    }

    // Add ambient light once
    result += ambientLightIntensity * albedo * ao;

    return result;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 baseColor = vec3(1.0);

    if (usePBR) {
        baseColor = texture(albedoMap, TexCoord).rgb;
        norm = normalize(texture(normalMap, TexCoord).rgb * 2.0 - 1.0);
        float metallic = texture(metallicMap, TexCoord).r;
        float roughness = texture(roughnessMap, TexCoord).r;
        float ao = texture(aoMap, TexCoord).r;
        
        vec3 lightingResult = calculateLighting(norm, viewDir, baseColor, metallic, roughness, ao);
        FragColor = vec4(lightingResult, 1.0);
    } else if (useTexture) {
        baseColor = texture(texture1, TexCoord).rgb;
        
        if (useLighting && !noShading) {
            vec3 lightingResult = calculateLighting(norm, viewDir, baseColor, 0.0, 1.0, 1.0);
            FragColor = vec4(lightingResult, 1.0);
        } else {
            FragColor = vec4(baseColor, 1.0);
        }
    } else {
        if (useColor) {
            baseColor = vertexColor.rgb;
        }
        
        if (useLighting && !noShading) {
            vec3 lightingResult = calculateLighting(norm, viewDir, baseColor, 0.0, 1.0, 1.0);
            FragColor = vec4(lightingResult, vertexColor.a);
        } else {
            FragColor = vec4(baseColor, vertexColor.a);
        }
    }
}