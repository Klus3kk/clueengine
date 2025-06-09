#include "globals.h"

Screen screen;
unsigned int shaderProgram;
Camera camera;

int viewLoc;
int projLoc;
int textureIndex = 0;
int colorCreation = 1;

bool isRunning = true;
bool debugPressed = false;
bool cubePressed = false;
bool spherePressed = false;
bool pyramidPressed = false;
bool cylinderPressed = false;
bool texturesPressed = false;
bool lightPressed1 = false;
bool lightPressed2 = false;
bool texturesEnabled = true;
bool colorsEnabled = true;
bool lightingEnabled = true;
bool colorTogglePressed = false;
bool noShading = false;
bool planePressed = false;
bool modelPressed = false;
bool usePBR = true;
bool pbrTogglePressed = false;
bool backgroundEnabled = true;
bool show_change_texture = false;
bool show_change_material = false;
bool cameraEnabled = true;
bool shadowsEnabled = true;
SceneObject* selected_object = NULL;
// Initialize the model variable
Model* loadedModel = NULL;
Mesh* loadedModelMesh = NULL;
SelectedType selectedType = SELECT_NONE;