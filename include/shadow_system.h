#ifndef SHADOW_SYSTEM_H
#define SHADOW_SYSTEM_H

#include <glad/glad.h>
#include <stdbool.h>
#include "Vectors.h"
#include "lightshading.h"

#define MAX_SHADOW_MAPS 8
#define SHADOW_MAP_SIZE 2048
#define CUBE_SHADOW_MAP_SIZE 1024

typedef enum {
    SHADOW_TYPE_DIRECTIONAL,
    SHADOW_TYPE_POINT,
    SHADOW_TYPE_SPOT
} ShadowType;

typedef struct {
    GLuint depthTexture;
    GLuint framebuffer;
    Matrix4x4 lightProjection;
    Matrix4x4 lightView;
    Matrix4x4 lightSpaceMatrix;
    int shadowMapSize;
    bool isActive;
    ShadowType type;
    int lightIndex;
} ShadowMap;

typedef struct {
    GLuint depthCubemap;
    GLuint framebuffer;
    Matrix4x4 lightProjection;
    Matrix4x4 lightViews[6]; // 6 faces of the cube
    Vector3 lightPosition;
    float farPlane;
    bool isActive;
    int lightIndex;
} CubeShadowMap;

typedef struct {
    ShadowMap* directionalShadows[MAX_SHADOW_MAPS];
    ShadowMap* spotShadows[MAX_SHADOW_MAPS];
    CubeShadowMap* pointShadows[MAX_SHADOW_MAPS];
    GLuint shadowShader;
    GLuint pointShadowShader;
    GLuint debugShader;
    int directionalCount;
    int spotCount;
    int pointCount;
    bool enableShadows;
    float shadowBias;
    int shadowQuality; // 0=Low, 1=Medium, 2=High
    bool showShadowMaps; // Debug visualization
} ShadowSystem;

// Make shadowSystem a POINTER
extern ShadowSystem* shadowSystem;

// Core shadow system functions
bool initShadowSystem();
void shutdownShadowSystem();
void updateShadowMaps();
void renderShadowMaps();

// Shadow map management
int createDirectionalShadowMap(int lightIndex);
int createPointShadowMap(int lightIndex);
int createSpotShadowMap(int lightIndex);
void destroyShadowMap(ShadowType type, int index);

// Shadow rendering
void renderDirectionalShadow(int shadowIndex);
void renderPointShadow(int shadowIndex);
void renderSpotShadow(int shadowIndex);
void renderSceneToShadowMap(const Matrix4x4* lightSpaceMatrix);
void renderSceneToCubeShadowMap(const Vector3* lightPos, float farPlane);

// Shadow matrix calculations
Matrix4x4 calculateDirectionalLightMatrix(const Light* light);
Matrix4x4 calculateSpotLightMatrix(const Light* light);
void calculatePointLightMatrices(const Light* light, Matrix4x4* matrices, float farPlane);

// Utility functions
void bindShadowMapsForRendering();
void setShadowUniforms(GLuint shader);
void toggleShadowQuality();
void debugRenderShadowMaps();

// Shadow settings
void setShadowQuality(int quality);
void setShadowBias(float bias);
void enableShadows(bool enable);

// Helper functions
bool createShadowFramebuffer(GLuint* framebuffer, GLuint* depthTexture, int size);
bool createCubeShadowFramebuffer(GLuint* framebuffer, GLuint* depthCubemap, int size);
void cleanupShadowMap(ShadowMap* shadowMap);
void cleanupCubeShadowMap(CubeShadowMap* cubeShadowMap);

#endif