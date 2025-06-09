#ifndef LIGHTSHADING_H
#define LIGHTSHADING_H

#define MAX_LIGHTS 10

#include "Vectors.h"

typedef enum {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT
} LightType;

typedef struct {
    LightType type;
    Vector3 position;
    Vector3 direction;
    Vector3 color;
    float intensity;
    float constant;    // For attenuation
    float linear;      // For attenuation
    float quadratic;   // For attenuation
    float cutOff;      // For spotlights
    float outerCutOff; // For spotlights
} Light;

extern Light lights[MAX_LIGHTS];
extern int lightCount;

void initLightingSystem();
void updateShaderLights();
void addLight(Light newLight);
void updateLight(int index, Light updatedLight);
void removeLight(int index);
Vector3 calculateLighting(Vector3 normal, Vector3 fragPos, Vector3 viewDir);
void createLight(Vector3 position, Vector3 direction, Vector3 color, float intensity, LightType type);

#endif 