#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <stdio.h>
#include "Vectors.h"
#include "Screen.h"

#ifdef _WIN32
    #include <Windows.h> // Windows-specific
#else
    #include <stdlib.h>  // Standard library for Linux/macOS
#endif

// Forward declaration to break circular dependency
struct SceneObject;
struct nk_context;
// Nuklear GUI context
extern struct SceneObject* selected_object;
extern struct nk_context* ctx;

#include "loading.h"
#include "ModelLoad.h"
#include "Camera.h"
#include "types.h"
#include "gui.h"

#define MAX_OBJECTS 1000

// Screen and rendering
extern Screen screen;
extern unsigned int shaderProgram;
extern int screen_width;
extern int screen_height;
extern Camera camera;

extern int viewLoc;
extern int projLoc;

// Indexes and flags for texture and color
extern int textureIndex;
extern int colorCreation;

// Flags for engine state
extern bool isRunning;
extern bool debugPressed;
extern bool cubePressed;
extern bool spherePressed;
extern bool pyramidPressed;
extern bool cylinderPressed;
extern bool texturesPressed;
extern bool lightPressed1;
extern bool lightPressed2;
extern bool texturesEnabled;
extern bool colorsEnabled;
extern bool lightingEnabled;
extern bool colorTogglePressed;
extern bool noShading;
extern bool planePressed;
extern bool modelPressed;
extern bool usePBR;
extern bool pbrTogglePressed;
extern bool backgroundEnabled;
extern bool cameraEnabled;
extern bool shadowsEnabled;

// Model and rendering data
extern Model* loadedModel;
extern Mesh* loadedModelMesh;
extern bool show_color_picker;
extern bool show_inspector;
extern bool show_change_texture;
extern bool show_change_material;

#endif 
