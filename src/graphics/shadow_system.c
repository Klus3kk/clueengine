#include "shadow_system.h"
#include "shaders.h"
#include "ObjectManager.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

ShadowSystem* shadowSystem = NULL;

// Shadow quality settings
static int shadowMapSizes[] = { 512, 1024, 2048 }; // Low, Medium, High
static int cubeShadowMapSizes[] = { 256, 512, 1024 };

bool initShadowSystem() {
    if (shadowSystem) {
        printf("Shadow system already initialized\n");
        return true;
    }

    shadowSystem = (ShadowSystem*)malloc(sizeof(ShadowSystem));
    if (!shadowSystem) {
        printf("Failed to allocate shadow system\n");
        return false;
    }

    // Initialize arrays to NULL
    memset(shadowSystem->directionalShadows, 0, sizeof(shadowSystem->directionalShadows));
    memset(shadowSystem->spotShadows, 0, sizeof(shadowSystem->spotShadows));
    memset(shadowSystem->pointShadows, 0, sizeof(shadowSystem->pointShadows));

    // Initialize counters
    shadowSystem->directionalCount = 0;
    shadowSystem->spotCount = 0;
    shadowSystem->pointCount = 0;

    // Load shadow shaders
    shadowSystem->shadowShader = loadShader("shaders/shadows/shadow_vertex.glsl", "shaders/shadows/shadow_fragment.glsl");
    shadowSystem->pointShadowShader = loadShader("shaders/shadows/point_shadow_vertex.glsl", "shaders/shadows/point_shadow_fragment.glsl");
    shadowSystem->debugShader = loadShader("shaders/shadows/debug_vertex.glsl", "shaders/shadows/debug_fragment.glsl");

    if (shadowSystem->shadowShader == 0 || shadowSystem->pointShadowShader == 0) {
        printf("Warning: Failed to load shadow shaders, shadows will be disabled\n");
        shadowSystem->enableShadows = false;
    } else {
        shadowSystem->enableShadows = true;
        printf("Shadow system initialized successfully\n");
    }

    // Set default values
    shadowSystem->shadowBias = 0.005f;
    shadowSystem->shadowQuality = 1; // Medium quality by default
    shadowSystem->showShadowMaps = false;

    return true;
}

void shutdownShadowSystem() {
    if (!shadowSystem) return;

    // Clean up directional shadows
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->directionalShadows[i]) {
            cleanupShadowMap(shadowSystem->directionalShadows[i]);
            free(shadowSystem->directionalShadows[i]);
        }
    }

    // Clean up spot shadows
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->spotShadows[i]) {
            cleanupShadowMap(shadowSystem->spotShadows[i]);
            free(shadowSystem->spotShadows[i]);
        }
    }

    // Clean up point shadows
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->pointShadows[i]) {
            cleanupCubeShadowMap(shadowSystem->pointShadows[i]);
            free(shadowSystem->pointShadows[i]);
        }
    }

    // Clean up shaders
    if (shadowSystem->shadowShader) glDeleteProgram(shadowSystem->shadowShader);
    if (shadowSystem->pointShadowShader) glDeleteProgram(shadowSystem->pointShadowShader);
    if (shadowSystem->debugShader) glDeleteProgram(shadowSystem->debugShader);

    free(shadowSystem);
    shadowSystem = NULL;
    printf("Shadow system shutdown complete\n");
}

bool createShadowFramebuffer(GLuint* framebuffer, GLuint* depthTexture, int size) {
    // Create framebuffer
    glGenFramebuffers(1, framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);

    // Create depth texture
    glGenTextures(1, depthTexture);
    glBindTexture(GL_TEXTURE_2D, *depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // Set border color to white (1.0) for areas outside shadow map
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach depth texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *depthTexture, 0);
    
    // No color buffer needed for shadow mapping
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Shadow framebuffer is not complete!\n");
        glDeleteFramebuffers(1, framebuffer);
        glDeleteTextures(1, depthTexture);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool createCubeShadowFramebuffer(GLuint* framebuffer, GLuint* depthCubemap, int size) {
    // Create framebuffer
    glGenFramebuffers(1, framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);

    // Create cube depth texture
    glGenTextures(1, depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *depthCubemap);
    
    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, 
                     size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Attach depth cubemap to framebuffer
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *depthCubemap, 0);
    
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Cube shadow framebuffer is not complete!\n");
        glDeleteFramebuffers(1, framebuffer);
        glDeleteTextures(1, depthCubemap);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

int createDirectionalShadowMap(int lightIndex) {
    if (!shadowSystem || !shadowSystem->enableShadows) return -1;
    if (shadowSystem->directionalCount >= MAX_SHADOW_MAPS) return -1;

    // Find available slot
    int index = -1;
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (!shadowSystem->directionalShadows[i]) {
            index = i;
            break;
        }
    }

    if (index == -1) return -1;

    ShadowMap* shadowMap = (ShadowMap*)malloc(sizeof(ShadowMap));
    if (!shadowMap) return -1;

    shadowMap->shadowMapSize = shadowMapSizes[shadowSystem->shadowQuality];
    shadowMap->type = SHADOW_TYPE_DIRECTIONAL;
    shadowMap->lightIndex = lightIndex;
    shadowMap->isActive = true;

    if (!createShadowFramebuffer(&shadowMap->framebuffer, &shadowMap->depthTexture, shadowMap->shadowMapSize)) {
        free(shadowMap);
        return -1;
    }

    shadowSystem->directionalShadows[index] = shadowMap;
    shadowSystem->directionalCount++;

    printf("Created directional shadow map %d for light %d\n", index, lightIndex);
    return index;
}

int createPointShadowMap(int lightIndex) {
    if (!shadowSystem || !shadowSystem->enableShadows) return -1;
    if (shadowSystem->pointCount >= MAX_SHADOW_MAPS) return -1;

    // Find available slot
    int index = -1;
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (!shadowSystem->pointShadows[i]) {
            index = i;
            break;
        }
    }

    if (index == -1) return -1;

    CubeShadowMap* cubeShadowMap = (CubeShadowMap*)malloc(sizeof(CubeShadowMap));
    if (!cubeShadowMap) return -1;

    int size = cubeShadowMapSizes[shadowSystem->shadowQuality];
    cubeShadowMap->lightIndex = lightIndex;
    cubeShadowMap->isActive = true;
    cubeShadowMap->farPlane = 25.0f; // Default far plane

    if (!createCubeShadowFramebuffer(&cubeShadowMap->framebuffer, &cubeShadowMap->depthCubemap, size)) {
        free(cubeShadowMap);
        return -1;
    }

    shadowSystem->pointShadows[index] = cubeShadowMap;
    shadowSystem->pointCount++;

    printf("Created point shadow map %d for light %d\n", index, lightIndex);
    return index;
}

int createSpotShadowMap(int lightIndex) {
    if (!shadowSystem || !shadowSystem->enableShadows) return -1;
    if (shadowSystem->spotCount >= MAX_SHADOW_MAPS) return -1;

    // Find available slot
    int index = -1;
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (!shadowSystem->spotShadows[i]) {
            index = i;
            break;
        }
    }

    if (index == -1) return -1;

    ShadowMap* shadowMap = (ShadowMap*)malloc(sizeof(ShadowMap));
    if (!shadowMap) return -1;

    shadowMap->shadowMapSize = shadowMapSizes[shadowSystem->shadowQuality];
    shadowMap->type = SHADOW_TYPE_SPOT;
    shadowMap->lightIndex = lightIndex;
    shadowMap->isActive = true;

    if (!createShadowFramebuffer(&shadowMap->framebuffer, &shadowMap->depthTexture, shadowMap->shadowMapSize)) {
        free(shadowMap);
        return -1;
    }

    shadowSystem->spotShadows[index] = shadowMap;
    shadowSystem->spotCount++;

    printf("Created spot shadow map %d for light %d\n", index, lightIndex);
    return index;
}

void destroyShadowMap(ShadowType type, int index) {
    if (!shadowSystem || index < 0 || index >= MAX_SHADOW_MAPS) return;

    switch (type) {
        case SHADOW_TYPE_DIRECTIONAL:
            if (shadowSystem->directionalShadows[index]) {
                cleanupShadowMap(shadowSystem->directionalShadows[index]);
                free(shadowSystem->directionalShadows[index]);
                shadowSystem->directionalShadows[index] = NULL;
                shadowSystem->directionalCount--;
            }
            break;
        case SHADOW_TYPE_SPOT:
            if (shadowSystem->spotShadows[index]) {
                cleanupShadowMap(shadowSystem->spotShadows[index]);
                free(shadowSystem->spotShadows[index]);
                shadowSystem->spotShadows[index] = NULL;
                shadowSystem->spotCount--;
            }
            break;
        case SHADOW_TYPE_POINT:
            if (shadowSystem->pointShadows[index]) {
                cleanupCubeShadowMap(shadowSystem->pointShadows[index]);
                free(shadowSystem->pointShadows[index]);
                shadowSystem->pointShadows[index] = NULL;
                shadowSystem->pointCount--;
            }
            break;
    }
}

Matrix4x4 calculateDirectionalLightMatrix(const Light* light) {
    // For directional lights, we create an orthographic projection
    float near_plane = 1.0f, far_plane = 7.5f;
    float orthoSize = 10.0f;
    
    // Initialize matrix properly
    Matrix4x4 lightProjection = {0}; // Initialize to zero first
    
    // Set matrix values manually
    lightProjection.data[0][0] = 2.0f/orthoSize;
    lightProjection.data[0][1] = 0.0f;
    lightProjection.data[0][2] = 0.0f;
    lightProjection.data[0][3] = 0.0f;
    
    lightProjection.data[1][0] = 0.0f;
    lightProjection.data[1][1] = 2.0f/orthoSize;
    lightProjection.data[1][2] = 0.0f;
    lightProjection.data[1][3] = 0.0f;
    
    lightProjection.data[2][0] = 0.0f;
    lightProjection.data[2][1] = 0.0f;
    lightProjection.data[2][2] = -2.0f/(far_plane-near_plane);
    lightProjection.data[2][3] = -(far_plane+near_plane)/(far_plane-near_plane);
    
    lightProjection.data[3][0] = 0.0f;
    lightProjection.data[3][1] = 0.0f;
    lightProjection.data[3][2] = 0.0f;
    lightProjection.data[3][3] = 1.0f;

    // Calculate light view matrix
    Vector3 lightTarget = vector_add(light->position, light->direction);
    Vector3 up = vector(0.0f, 1.0f, 0.0f);
    Matrix4x4 lightView = lookAt(light->position, lightTarget, up);

    return matrixMultiply(lightProjection, lightView);
}

Matrix4x4 calculateSpotLightMatrix(const Light* light) {
    // Create perspective projection for spotlight
    float fov = acosf(light->cutOff) * 2.0f; // Convert cutoff to full FOV
    float aspect = 1.0f; // Square shadow map
    float near_plane = 1.0f, far_plane = 25.0f;
    
    Matrix4x4 lightProjection = perspective(fov, aspect, near_plane, far_plane);
    
    Vector3 lightTarget = vector_add(light->position, light->direction);
    Vector3 up = vector(0.0f, 1.0f, 0.0f);
    Matrix4x4 lightView = lookAt(light->position, lightTarget, up);

    return matrixMultiply(lightProjection, lightView);
}

void calculatePointLightMatrices(const Light* light, Matrix4x4* matrices, float farPlane) {
    Matrix4x4 shadowProjection = perspective(M_PI / 2.0f, 1.0f, 1.0f, farPlane);
    
    Vector3 targets[6] = {
        vector_add(light->position, vector(1.0f, 0.0f, 0.0f)),   // +X
        vector_add(light->position, vector(-1.0f, 0.0f, 0.0f)),  // -X
        vector_add(light->position, vector(0.0f, 1.0f, 0.0f)),   // +Y
        vector_add(light->position, vector(0.0f, -1.0f, 0.0f)),  // -Y
        vector_add(light->position, vector(0.0f, 0.0f, 1.0f)),   // +Z
        vector_add(light->position, vector(0.0f, 0.0f, -1.0f))   // -Z
    };
    
    Vector3 ups[6] = {
        vector(0.0f, -1.0f, 0.0f),  // +X
        vector(0.0f, -1.0f, 0.0f),  // -X
        vector(0.0f, 0.0f, 1.0f),   // +Y
        vector(0.0f, 0.0f, -1.0f),  // -Y
        vector(0.0f, -1.0f, 0.0f),  // +Z
        vector(0.0f, -1.0f, 0.0f)   // -Z
    };

    for (int i = 0; i < 6; i++) {
        Matrix4x4 shadowView = lookAt(light->position, targets[i], ups[i]);
        matrices[i] = matrixMultiply(shadowProjection, shadowView);
    }
}

void renderDirectionalShadow(int shadowIndex) {
    if (!shadowSystem || shadowIndex < 0 || shadowIndex >= MAX_SHADOW_MAPS) return;
    if (!shadowSystem->directionalShadows[shadowIndex]) return;

    ShadowMap* shadowMap = shadowSystem->directionalShadows[shadowIndex];
    Light* light = &lights[shadowMap->lightIndex];

    // Calculate light space matrix
    shadowMap->lightSpaceMatrix = calculateDirectionalLightMatrix(light);

    // Render to shadow map
    glViewport(0, 0, shadowMap->shadowMapSize, shadowMap->shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap->framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render scene from light's perspective
    renderSceneToShadowMap(&shadowMap->lightSpaceMatrix);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderPointShadow(int shadowIndex) {
    if (!shadowSystem || shadowIndex < 0 || shadowIndex >= MAX_SHADOW_MAPS) return;
    if (!shadowSystem->pointShadows[shadowIndex]) return;

    CubeShadowMap* cubeShadowMap = shadowSystem->pointShadows[shadowIndex];
    Light* light = &lights[cubeShadowMap->lightIndex];

    cubeShadowMap->lightPosition = light->position;
    
    // Calculate view matrices for all 6 faces
    calculatePointLightMatrices(light, cubeShadowMap->lightViews, cubeShadowMap->farPlane);

    // Render to cube shadow map
    int size = cubeShadowMapSizes[shadowSystem->shadowQuality];
    glViewport(0, 0, size, size);
    glBindFramebuffer(GL_FRAMEBUFFER, cubeShadowMap->framebuffer);

    // Render each face of the cubemap
    for (int face = 0; face < 6; face++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 
                               cubeShadowMap->depthCubemap, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        renderSceneToCubeShadowMap(&cubeShadowMap->lightPosition, cubeShadowMap->farPlane);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderSpotShadow(int shadowIndex) {
    if (!shadowSystem || shadowIndex < 0 || shadowIndex >= MAX_SHADOW_MAPS) return;
    if (!shadowSystem->spotShadows[shadowIndex]) return;

    ShadowMap* shadowMap = shadowSystem->spotShadows[shadowIndex];
    Light* light = &lights[shadowMap->lightIndex];

    // Calculate light space matrix
    shadowMap->lightSpaceMatrix = calculateSpotLightMatrix(light);

    // Render to shadow map
    glViewport(0, 0, shadowMap->shadowMapSize, shadowMap->shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap->framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render scene from light's perspective
    renderSceneToShadowMap(&shadowMap->lightSpaceMatrix);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderSceneToShadowMap(const Matrix4x4* lightSpaceMatrix) {
    glUseProgram(shadowSystem->shadowShader);
    
    GLint lightSpaceMatrixLoc = glGetUniformLocation(shadowSystem->shadowShader, "lightSpaceMatrix");
    glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, &lightSpaceMatrix->data[0][0]);

    // Enable back face culling to reduce peter panning
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    // Render all objects in the scene
    for (int i = 0; i < objectManager.count; i++) {
        SceneObject* obj = &objectManager.objects[i];
        
        // Calculate model matrix
        Matrix4x4 modelMatrix = translateMatrix(obj->position);
        modelMatrix = matrixMultiply(modelMatrix, rotateMatrix(obj->rotation.x, vector(1.0f, 0.0f, 0.0f)));
        modelMatrix = matrixMultiply(modelMatrix, rotateMatrix(obj->rotation.y, vector(0.0f, 1.0f, 0.0f)));
        modelMatrix = matrixMultiply(modelMatrix, rotateMatrix(obj->rotation.z, vector(0.0f, 0.0f, 1.0f)));
        modelMatrix = matrixMultiply(modelMatrix, scaleMatrix(obj->scale));

        GLint modelLoc = glGetUniformLocation(shadowSystem->shadowShader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix.data[0][0]);

        // Draw the object (simplified - just geometry, no textures)
        switch (obj->object.type) {
            case OBJ_CUBE:
                glBindVertexArray(obj->object.data.cube.vao);
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_SPHERE:
                glBindVertexArray(obj->object.data.sphere.vao);
                glDrawElements(GL_TRIANGLES, obj->object.data.sphere.numIndices, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_PYRAMID:
                glBindVertexArray(obj->object.data.pyramid.vao);
                glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_CYLINDER:
                glBindVertexArray(obj->object.data.cylinder.vao);
                glDrawElements(GL_TRIANGLES, obj->object.data.cylinder.sectorCount * 12, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_PLANE:
                glBindVertexArray(obj->object.data.plane.vao);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_MODEL:
                for (unsigned int j = 0; j < obj->object.data.model.meshCount; j++) {
                    glBindVertexArray(obj->object.data.model.meshes[j].VAO);
                    glDrawElements(GL_TRIANGLES, obj->object.data.model.meshes[j].numIndices, GL_UNSIGNED_INT, 0);
                }
                break;
        }
    }

    glDisable(GL_CULL_FACE);
    glBindVertexArray(0);
}

void renderSceneToCubeShadowMap(const Vector3* lightPos, float farPlane) {
    glUseProgram(shadowSystem->pointShadowShader);
    
    GLint lightPosLoc = glGetUniformLocation(shadowSystem->pointShadowShader, "lightPos");
    GLint farPlaneLoc = glGetUniformLocation(shadowSystem->pointShadowShader, "far_plane");
    
    glUniform3f(lightPosLoc, lightPos->x, lightPos->y, lightPos->z);
    glUniform1f(farPlaneLoc, farPlane);

    // Render scene (similar to renderSceneToShadowMap but for point lights)
    for (int i = 0; i < objectManager.count; i++) {
        SceneObject* obj = &objectManager.objects[i];
        
        Matrix4x4 modelMatrix = translateMatrix(obj->position);
        modelMatrix = matrixMultiply(modelMatrix, rotateMatrix(obj->rotation.x, vector(1.0f, 0.0f, 0.0f)));
        modelMatrix = matrixMultiply(modelMatrix, rotateMatrix(obj->rotation.y, vector(0.0f, 1.0f, 0.0f)));
        modelMatrix = matrixMultiply(modelMatrix, rotateMatrix(obj->rotation.z, vector(0.0f, 0.0f, 1.0f)));
        modelMatrix = matrixMultiply(modelMatrix, scaleMatrix(obj->scale));

        GLint modelLoc = glGetUniformLocation(shadowSystem->pointShadowShader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix.data[0][0]);

        // Draw object
        switch (obj->object.type) {
            case OBJ_CUBE:
                glBindVertexArray(obj->object.data.cube.vao);
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_SPHERE:
                glBindVertexArray(obj->object.data.sphere.vao);
                glDrawElements(GL_TRIANGLES, obj->object.data.sphere.numIndices, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_PYRAMID:
                glBindVertexArray(obj->object.data.pyramid.vao);
                glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_CYLINDER:
                glBindVertexArray(obj->object.data.cylinder.vao);
                glDrawElements(GL_TRIANGLES, obj->object.data.cylinder.sectorCount * 12, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_PLANE:
                glBindVertexArray(obj->object.data.plane.vao);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                break;
            case OBJ_MODEL:
                for (unsigned int j = 0; j < obj->object.data.model.meshCount; j++) {
                    glBindVertexArray(obj->object.data.model.meshes[j].VAO);
                    glDrawElements(GL_TRIANGLES, obj->object.data.model.meshes[j].numIndices, GL_UNSIGNED_INT, 0);
                }
                break;
        }
    }

    glBindVertexArray(0);
}

void updateShadowMaps() {
    if (!shadowSystem || !shadowSystem->enableShadows) return;

    // Auto-create shadow maps for lights that don't have them
    for (int i = 0; i < lightCount; i++) {
        Light* light = &lights[i];
        bool hasShadowMap = false;

        // Check if this light already has a shadow map
        switch (light->type) {
            case LIGHT_DIRECTIONAL:
                for (int j = 0; j < MAX_SHADOW_MAPS; j++) {
                    if (shadowSystem->directionalShadows[j] && 
                        shadowSystem->directionalShadows[j]->lightIndex == i) {
                        hasShadowMap = true;
                        break;
                    }
                }
                if (!hasShadowMap && shadowSystem->directionalCount < MAX_SHADOW_MAPS) {
                    createDirectionalShadowMap(i);
                }
                break;

            case LIGHT_POINT:
                for (int j = 0; j < MAX_SHADOW_MAPS; j++) {
                    if (shadowSystem->pointShadows[j] && 
                        shadowSystem->pointShadows[j]->lightIndex == i) {
                        hasShadowMap = true;
                        break;
                    }
                }
                if (!hasShadowMap && shadowSystem->pointCount < MAX_SHADOW_MAPS) {
                    createPointShadowMap(i);
                }
                break;

            case LIGHT_SPOT:
                for (int j = 0; j < MAX_SHADOW_MAPS; j++) {
                    if (shadowSystem->spotShadows[j] && 
                        shadowSystem->spotShadows[j]->lightIndex == i) {
                        hasShadowMap = true;
                        break;
                    }
                }
                if (!hasShadowMap && shadowSystem->spotCount < MAX_SHADOW_MAPS) {
                    createSpotShadowMap(i);
                }
                break;
        }
    }
}

void renderShadowMaps() {
    if (!shadowSystem || !shadowSystem->enableShadows) return;

    // Store original viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Render directional shadow maps
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->directionalShadows[i] && shadowSystem->directionalShadows[i]->isActive) {
            renderDirectionalShadow(i);
        }
    }

    // Render point shadow maps
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->pointShadows[i] && shadowSystem->pointShadows[i]->isActive) {
            renderPointShadow(i);
        }
    }

    // Render spot shadow maps
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->spotShadows[i] && shadowSystem->spotShadows[i]->isActive) {
            renderSpotShadow(i);
        }
    }

    // Restore original viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void bindShadowMapsForRendering() {
    if (!shadowSystem || !shadowSystem->enableShadows) return;

    int textureUnit = 10; // Start from texture unit 10 for shadow maps

    // Bind directional shadow maps
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->directionalShadows[i] && shadowSystem->directionalShadows[i]->isActive) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, shadowSystem->directionalShadows[i]->depthTexture);
            textureUnit++;
        }
    }

    // Bind point shadow maps
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->pointShadows[i] && shadowSystem->pointShadows[i]->isActive) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowSystem->pointShadows[i]->depthCubemap);
            textureUnit++;
        }
    }

    // Bind spot shadow maps
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->spotShadows[i] && shadowSystem->spotShadows[i]->isActive) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, shadowSystem->spotShadows[i]->depthTexture);
            textureUnit++;
        }
    }
}

void setShadowUniforms(GLuint shader) {
    if (!shadowSystem || !shadowSystem->enableShadows) return;

    glUseProgram(shader);

    // Set shadow bias
    GLint shadowBiasLoc = glGetUniformLocation(shader, "shadowBias");
    if (shadowBiasLoc != -1) {
        glUniform1f(shadowBiasLoc, shadowSystem->shadowBias);
    }

    // Set enable shadows flag
    GLint enableShadowsLoc = glGetUniformLocation(shader, "enableShadows");
    if (enableShadowsLoc != -1) {
        glUniform1i(enableShadowsLoc, shadowSystem->enableShadows ? 1 : 0);
    }

    // Set light space matrices for directional/spot lights
    int shadowMapIndex = 0;
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->directionalShadows[i] && shadowSystem->directionalShadows[i]->isActive) {
            char uniformName[64];
            snprintf(uniformName, sizeof(uniformName), "lightSpaceMatrix[%d]", shadowMapIndex);
            GLint lightSpaceMatrixLoc = glGetUniformLocation(shader, uniformName);
            if (lightSpaceMatrixLoc != -1) {
                glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, 
                                 &shadowSystem->directionalShadows[i]->lightSpaceMatrix.data[0][0]);
            }

            snprintf(uniformName, sizeof(uniformName), "shadowMap[%d]", shadowMapIndex);
            GLint shadowMapLoc = glGetUniformLocation(shader, uniformName);
            if (shadowMapLoc != -1) {
                glUniform1i(shadowMapLoc, 10 + shadowMapIndex);
            }
            shadowMapIndex++;
        }
    }

    // Set point light shadow information
    int pointShadowIndex = 0;
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        if (shadowSystem->pointShadows[i] && shadowSystem->pointShadows[i]->isActive) {
            char uniformName[64];
            snprintf(uniformName, sizeof(uniformName), "pointShadowMaps[%d]", pointShadowIndex);
            GLint pointShadowMapLoc = glGetUniformLocation(shader, uniformName);
            if (pointShadowMapLoc != -1) {
                glUniform1i(pointShadowMapLoc, 10 + shadowMapIndex + pointShadowIndex);
            }

            snprintf(uniformName, sizeof(uniformName), "pointLightPositions[%d]", pointShadowIndex);
            GLint pointLightPosLoc = glGetUniformLocation(shader, uniformName);
            if (pointLightPosLoc != -1) {
                Vector3 lightPos = shadowSystem->pointShadows[i]->lightPosition;
                glUniform3f(pointLightPosLoc, lightPos.x, lightPos.y, lightPos.z);
            }

            snprintf(uniformName, sizeof(uniformName), "pointLightFarPlane[%d]", pointShadowIndex);
            GLint pointLightFarPlaneLoc = glGetUniformLocation(shader, uniformName);
            if (pointLightFarPlaneLoc != -1) {
                glUniform1f(pointLightFarPlaneLoc, shadowSystem->pointShadows[i]->farPlane);
            }
            pointShadowIndex++;
        }
    }
}

void setShadowQuality(int quality) {
    if (!shadowSystem) return;
    if (quality < 0 || quality > 2) return;

    shadowSystem->shadowQuality = quality;
    printf("Shadow quality set to: %s\n", 
           quality == 0 ? "Low" : quality == 1 ? "Medium" : "High");
}

void setShadowBias(float bias) {
    if (!shadowSystem) return;
    shadowSystem->shadowBias = bias;
}

void enableShadows(bool enable) {
    if (!shadowSystem) return;
    shadowSystem->enableShadows = enable;
    printf("Shadows %s\n", enable ? "enabled" : "disabled");
}

void toggleShadowQuality() {
    if (!shadowSystem) return;
    shadowSystem->shadowQuality = (shadowSystem->shadowQuality + 1) % 3;
    printf("Shadow quality: %s\n", 
           shadowSystem->shadowQuality == 0 ? "Low" : 
           shadowSystem->shadowQuality == 1 ? "Medium" : "High");
}

void debugRenderShadowMaps() {
    if (!shadowSystem || !shadowSystem->showShadowMaps || !shadowSystem->debugShader) return;

    // Simple debug rendering - display shadow maps as quads on screen
    // This is a basic implementation, can be enhanced with proper UI
    glUseProgram(shadowSystem->debugShader);
    
    // Render first directional shadow map if available
    if (shadowSystem->directionalShadows[0] && shadowSystem->directionalShadows[0]->isActive) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowSystem->directionalShadows[0]->depthTexture);
        
        GLint textureLoc = glGetUniformLocation(shadowSystem->debugShader, "depthMap");
        if (textureLoc != -1) {
            glUniform1i(textureLoc, 0);
        }
        
        // Render a simple quad (implementation depends on your quad rendering setup)
        // This would require additional quad geometry setup
    }
}

void cleanupShadowMap(ShadowMap* shadowMap) {
    if (!shadowMap) return;
    
    if (shadowMap->framebuffer) {
        glDeleteFramebuffers(1, &shadowMap->framebuffer);
    }
    if (shadowMap->depthTexture) {
        glDeleteTextures(1, &shadowMap->depthTexture);
    }
}

void cleanupCubeShadowMap(CubeShadowMap* cubeShadowMap) {
    if (!cubeShadowMap) return;
    
    if (cubeShadowMap->framebuffer) {
        glDeleteFramebuffers(1, &cubeShadowMap->framebuffer);
    }
    if (cubeShadowMap->depthCubemap) {
        glDeleteTextures(1, &cubeShadowMap->depthCubemap);
    }
}