#include "rendering.h"
#include "Screen.h"
#include "Camera.h"
#include "ObjectManager.h"
#include "shaders.h"
#include "textures.h"
#include "lightshading.h"
#include "background.h"
#include "globals.h"
#include "materials.h"
#include "gui.h"
#include "shadow_system.h"

#ifdef AUDIO_ENABLED
#include "audio.h"
#endif


// Function prototypes
static Model* model = NULL;

// Delta time variables
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

void loadResources(int stage, float* progress) {
    switch (stage) {
    case 0:  // Initialization
        *progress += 0.1f;
        break;
    case 1:  // Load Textures
        loadAllTextures();
        *progress += 0.2f;
        break;
    case 2:  // Load PBR Textures
        loadPBRTextures();
        *progress += 0.2f;
        break;
    case 3:  // Setup Skybox
        initSkybox(7);
        *progress += 0.2f;
        break;
    case 4:  // Setup Lighting
        initLightingSystem();
        *progress += 0.2f;
        break;
    case 5:  // Setup Audio (NEW)
        #ifdef AUDIO_ENABLED
        if (initAudioSystem()) {
            printf("Audio system initialized successfully\n");
        } else {
            printf("Audio system failed to initialize - continuing without audio\n");
        }
        #endif
        *progress += 0.1f;
        break;
    default:
        break;
    }
}
void setup() {
    strncpy(screen.title, "C1ue Engine v1.1.0", sizeof(screen.title) - 1);

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    // Get the primary monitor and its current video mode
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    screen.width = mode->width;
    screen.height = mode->height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    screen.window = glfwCreateWindow(screen.width, screen.height, screen.title, NULL, NULL);
    if (!screen.window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Center the window on the screen
    int windowX = (mode->width - screen.width) / 2;
    int windowY = (mode->height - screen.height) / 2;
    glfwSetWindowPos(screen.window, windowX, windowY);

    glfwMakeContextCurrent(screen.window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        exit(EXIT_FAILURE);
    }
    glfwSwapInterval(1);
    setup_nuklear(screen.window);

    // Set up shaders and get uniform locations
    shaderProgram = loadShader("shaders/objects/vertex.glsl", "shaders/objects/fragment.glsl");
    if (shaderProgram == 0) {
        fprintf(stderr, "Failed to load shaders\n");
    }
    glUseProgram(shaderProgram);

    viewLoc = glGetUniformLocation(shaderProgram, "view");
    if (viewLoc == -1) {
        fprintf(stderr, "Could not find uniform variable 'view'\n");
    }

    projLoc = glGetUniformLocation(shaderProgram, "projection");
    if (projLoc == -1) {
        fprintf(stderr, "Could not find uniform variable 'projection'\n");
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glfwSetInputMode(screen.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize camera, object manager, and other essential systems
    initCamera(&camera);
    initObjectManager();
    initLightingSystem();
    
    // Initialize shadow system
    if (!initShadowSystem()) {
        printf("Warning: Shadow system initialization failed\n");
        shadowsEnabled = false;
    } else {
        printf("Shadow system initialized successfully\n");
    }

    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Disable face culling to ensure all faces are rendered
    glDisable(GL_CULL_FACE);

        printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void drawMesh(const Mesh* mesh) {
    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void processKeyboardMovements(Camera* camera, float deltaTime) {
    float velocity = camera->MovementSpeed * deltaTime;
    if (glfwGetKey(screen.window, GLFW_KEY_W) == GLFW_PRESS) {
        camera->Position = vector_add(camera->Position, vector_scale(camera->Front, velocity));
    }
    if (glfwGetKey(screen.window, GLFW_KEY_S) == GLFW_PRESS) {
        camera->Position = vector_sub(camera->Position, vector_scale(camera->Front, velocity));
    }
    if (glfwGetKey(screen.window, GLFW_KEY_A) == GLFW_PRESS) {
        camera->Position = vector_sub(camera->Position, vector_scale(camera->Right, velocity));
    }
    if (glfwGetKey(screen.window, GLFW_KEY_D) == GLFW_PRESS) {
        camera->Position = vector_add(camera->Position, vector_scale(camera->Right, velocity));
    }
    if (glfwGetKey(screen.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera->Position = vector_add(camera->Position, vector_scale(camera->Up, velocity));
    }
    if (glfwGetKey(screen.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera->Position = vector_sub(camera->Position, vector_scale(camera->Up, velocity));
    }
}

void handleToggleInput(int key, bool* pressedFlag, bool* toggleFlag, const char* toggleName) {
    if (glfwGetKey(screen.window, key) == GLFW_PRESS && !(*pressedFlag)) {
        *toggleFlag = !(*toggleFlag);
        printf("%s %s.\n", toggleName, *toggleFlag ? "Enabled" : "Disabled");
        *pressedFlag = true;
    }
    else if (glfwGetKey(screen.window, key) == GLFW_RELEASE) {
        *pressedFlag = false;
    }
}

void handleObjectCreation(int key, bool* pressedFlag, ObjectType objType) {
    if (glfwGetKey(screen.window, key) == GLFW_PRESS && !(*pressedFlag)) {
        PBRMaterial defaultMaterial = { 0 }; // Initialize material to zero
        addObject(&camera, objType, false, -1, true, NULL, defaultMaterial, false); // No texture by default
        *pressedFlag = true;
    }
    else if (glfwGetKey(screen.window, key) == GLFW_RELEASE) {
        *pressedFlag = false;
    }
}


void render_scene(const Matrix4x4 viewMatrix, const Matrix4x4 projMatrix) {
    for (int i = 0; i < objectManager.count; i++) {
        drawObject(&objectManager.objects[i], viewMatrix, projMatrix);
    }
}

float distanceFromCamera(SceneObject* obj) {
    Vector3 diff = vector_sub(camera.Position, obj->position);
    return vector_length(diff);
}

int compareObjects(const void* a, const void* b) {
    SceneObject* objA = *(SceneObject**)a;
    SceneObject* objB = *(SceneObject**)b;
    float distanceA = vector_length(vector_sub(camera.Position, objA->position));
    float distanceB = vector_length(vector_sub(camera.Position, objB->position));
    return (distanceA < distanceB) - (distanceA > distanceB); // Sort descending
}

void setShaderUniforms(SceneObject* obj) {
    glUseProgram(shaderProgram);
    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    GLint usePBRLoc = glGetUniformLocation(shaderProgram, "usePBR");
    GLint useColorLoc = glGetUniformLocation(shaderProgram, "useColor");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "inputColor");

    // Set texture usage
    glUniform1i(useTextureLoc, texturesEnabled && obj->object.useTexture && !obj->object.usePBR);
    // Set PBR usage
    glUniform1i(usePBRLoc, usePBR && obj->object.usePBR);
    // Set color usage
    glUniform1i(useColorLoc, colorsEnabled && obj->object.useColor);
    // Set input color
    glUniform4f(colorLoc, obj->color.x, obj->color.y, obj->color.z, obj->color.w);

    if (obj->object.useTexture && texturesEnabled) {
        glBindTexture(GL_TEXTURE_2D, obj->object.textureID);
    }

    if (usePBR && obj->object.usePBR) {
        bindPBRMaterial(obj->object.material);
    }
}

// In the render() function in rendering.c, modify the rendering pipeline:
void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4x4 projMatrix = getProjectionMatrix(45.0f, (float)screen.width / screen.height, 0.1f, 100.0f);
    Matrix4x4 viewMatrix = getViewMatrix(&camera);

    // First pass: Render shadow maps
    if (shadowsEnabled && shadowSystem && shadowSystem->enableShadows) {
        updateShadowMaps();
        renderShadowMaps();
    }

    // Draw skybox first if background is enabled
    if (backgroundEnabled) {
        glDepthFunc(GL_LEQUAL);
        drawSkybox(&camera, &projMatrix);
        glDepthFunc(GL_LESS);
    }

    // Second pass: Render scene with shadows
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMatrix.data[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMatrix.data[0][0]);
    
    // Update lighting uniforms
    updateShaderLights();
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, (const GLfloat*)&camera.Position);
    glUniform1i(glGetUniformLocation(shaderProgram, "useLighting"), lightingEnabled);
    glUniform1i(glGetUniformLocation(shaderProgram, "noShading"), !lightingEnabled);

    // Bind shadow maps and set shadow uniforms
    if (shadowsEnabled && shadowSystem && shadowSystem->enableShadows) {
        bindShadowMapsForRendering();
        setShadowUniforms(shaderProgram);
    } else {
        // Disable shadows in shader
        GLint enableShadowsLoc = glGetUniformLocation(shaderProgram, "enableShadows");
        if (enableShadowsLoc != -1) {
            glUniform1i(enableShadowsLoc, 0);
        }
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Separate objects into opaque and transparent lists
    SceneObject* opaqueObjects[MAX_OBJECTS];
    SceneObject* transparentObjects[MAX_OBJECTS];
    int opaqueCount = 0;
    int transparentCount = 0;

    for (int i = 0; i < objectManager.count; i++) {
        SceneObject* obj = &objectManager.objects[i];
        if (obj->color.w < 1.0f) {
            transparentObjects[transparentCount++] = obj;
        }
        else {
            opaqueObjects[opaqueCount++] = obj;
        }
    }

    // Sort transparent objects by distance from the camera (farthest first)
    qsort(transparentObjects, transparentCount, sizeof(SceneObject*), compareObjects);

    // Render opaque objects first
    for (int i = 0; i < opaqueCount; i++) {
        SceneObject* obj = opaqueObjects[i];
        setShaderUniforms(obj);
        drawObject(obj, viewMatrix, projMatrix);
    }

    // Render transparent objects last
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < transparentCount; i++) {
        SceneObject* obj = transparentObjects[i];
        setShaderUniforms(obj);
        drawObject(obj, viewMatrix, projMatrix);
    }
    glDisable(GL_BLEND);

    // Draw model's meshes if loaded
    if (model) {
        for (unsigned int i = 0; i < model->meshCount; i++) {
            drawMesh(&model->meshes[i]);
        }
    }

    // Debug render shadow maps if enabled
    if (shadowsEnabled && shadowSystem && shadowSystem->showShadowMaps) {
        debugRenderShadowMaps();
    }
}

double calculateDeltaTime() {
    double currentFrameTime = glfwGetTime();
    deltaTime = currentFrameTime - lastFrame;
    lastFrame = currentFrameTime;
    return deltaTime;
}

void update(double deltaTime) {
    if (!isRunning) {
        return;
    }

    if (glfwGetKey(screen.window, GLFW_KEY_E) == GLFW_PRESS) {
        printf("\nExiting...\n");
        exit(EXIT_SUCCESS);
    }

    handleToggleInput(GLFW_KEY_T, &texturesPressed, &texturesEnabled, "Textures");
    handleToggleInput(GLFW_KEY_L, &colorTogglePressed, &colorsEnabled, "Colors");
    handleToggleInput(GLFW_KEY_J, &lightPressed1, &noShading, "Shading");
    handleToggleInput(GLFW_KEY_Q, &pbrTogglePressed, &usePBR, "PBR");
    
    // Add shadow controls
    static bool shadowTogglePressed = false;
    static bool shadowQualityPressed = false;
    static bool shadowDebugPressed = false;
    
    handleToggleInput(GLFW_KEY_M, &shadowTogglePressed, &shadowsEnabled, "Shadows");
    
    if (glfwGetKey(screen.window, GLFW_KEY_N) == GLFW_PRESS && !shadowQualityPressed) {
        if (shadowSystem) {
            toggleShadowQuality();
        }
        shadowQualityPressed = true;
    }
    else if (glfwGetKey(screen.window, GLFW_KEY_N) == GLFW_RELEASE) {
        shadowQualityPressed = false;
    }
    
    if (glfwGetKey(screen.window, GLFW_KEY_COMMA) == GLFW_PRESS && !shadowDebugPressed) {
        if (shadowSystem) {
            shadowSystem->showShadowMaps = !shadowSystem->showShadowMaps;
            printf("Shadow debug view %s\n", shadowSystem->showShadowMaps ? "enabled" : "disabled");
        }
        shadowDebugPressed = true;
    }
    else if (glfwGetKey(screen.window, GLFW_KEY_COMMA) == GLFW_RELEASE) {
        shadowDebugPressed = false;
    }

    handleObjectCreation(GLFW_KEY_O, &planePressed, OBJ_PLANE);
    handleObjectCreation(GLFW_KEY_C, &cubePressed, OBJ_CUBE);
    handleObjectCreation(GLFW_KEY_H, &pyramidPressed, OBJ_PYRAMID);
    handleObjectCreation(GLFW_KEY_K, &spherePressed, OBJ_SPHERE);
    handleObjectCreation(GLFW_KEY_B, &cylinderPressed, OBJ_CYLINDER);

    if (glfwGetKey(screen.window, GLFW_KEY_I) == GLFW_PRESS && !lightPressed2) {
        createLight(camera.Position, camera.Front, vector(1.0f, 1.0f, 1.0f), 1.0f, LIGHT_POINT);
        lightPressed2 = true;
    }
    else if (glfwGetKey(screen.window, GLFW_KEY_I) == GLFW_RELEASE) {
        lightPressed2 = false;
    }

    processKeyboardMovements(&camera, deltaTime);
    #ifdef AUDIO_ENABLED
    updateAudioSystem();
    setListenerPosition(&camera.Position);
    setListenerOrientation(&camera.Front, &camera.Up);
    #endif
}


void handleMouseInput(GLFWwindow* window, Camera* camera) {
    static double lastX = 0, lastY = 0;
    static bool firstMouse = true;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (!isRunning) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            processMousePan(camera, xoffset, yoffset); // Adjust position in 3D space
        }
        else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            processMouseMovement(camera, xoffset, yoffset, true);
        }
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        processMouseMovement(camera, xoffset, yoffset, true);
    }
}

void end() {
    cleanupObjects();

    if (shadowSystem) {
        shutdownShadowSystem();
    }
    
    #ifdef AUDIO_ENABLED
    shutdownAudioSystem();
    #endif
    glfwDestroyWindow(screen.window);
    glfwTerminate();
}
