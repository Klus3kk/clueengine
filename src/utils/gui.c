// Include necessary headers
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <stdbool.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Assimp headers
#include <assimp/scene.h>
#include <assimp/vector3.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

// Nuklear headers
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear/nuklear.h"
#include "nuklear/nuklear_glfw_gl3.h"

// Project headers
#include "globals.h"
#include "gui.h"
#include "ObjectManager.h"
#include "resource_loader.h"
#include "rendering.h"
#include "textures.h"
#include "lightshading.h"
#include "file_operations.h"
#include "background.h"
#include "actions.h"

// Audio system header
#ifdef AUDIO_ENABLED
#include "audio.h"
#endif

extern int textureCount;
extern int materialCount;

static bool show_material_window = false;
static bool show_texture_window = false;
static SceneObject* material_window_obj = NULL;
static SceneObject* texture_window_obj = NULL;
extern SceneObject* selected_object;

SceneObject* clipboard_object = NULL; // Clipboard for cut/copy/paste
GLuint textureColorbuffer; // External linkage to the texture from rendering.c
GLFWwindow* window;
bool theme_dark = true;
bool ctrlPressed = false;
bool show_about = false;
bool show_debug = false;
bool show_hierarchy = true;
bool show_inspector = true; 
bool show_history = false;
bool show_settings = false; 
bool isCutOperation = false;
bool show_controls = false;
bool show_object_creator = false;
bool show_color_picker = false;
static struct nk_glfw glfw = {0};
struct nk_context *ctx = NULL;  // Global context initialization
struct nk_font* roboto_font;
bool show_change_background = false;

bool is_fullscreen = false;
int windowed_x = 100, windowed_y = 100, windowed_width = 1920, windowed_height = 1080;

// Function to get nk_context globally
struct nk_context* get_nk_context() {
    return ctx;
}

int find_selected_object_index(SceneObject* selected_object) {
    for (int i = 0; i < objectManager.count; i++) {
        if (&objectManager.objects[i] == selected_object) {
            return i;
        }
    }
    return -1; // Not found
}

// Change material function
void material_picker_window(struct nk_context* ctx) {
    if (selected_object && nk_begin(ctx, "Change Material", nk_rect(400, 650, 300, 200), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        for (int i = 0; i < materialCount; i++) {
            if (nk_button_label(ctx, materialNames[i])) {
                PBRMaterial* newMaterial = getMaterial(materialNames[i]);
                if (newMaterial) {
                    PBRMaterial* clonedMaterial = (PBRMaterial*)malloc(sizeof(PBRMaterial));
                    *clonedMaterial = *newMaterial;
                    selected_object->object.material = *clonedMaterial;
                    selected_object->object.usePBR = true;
                    selected_object->object.useTexture = false;
                    selected_object->object.useColor = false;
                    updateObjectInManager(selected_object);
                }
            }
        }
        nk_end(ctx);
    }
    else {
        show_material_window = false;
        nk_end(ctx);
    }
}

// Change texture function
void texture_picker_window(struct nk_context* ctx) {
    if (selected_object && nk_begin(ctx, "Change Texture", nk_rect(400, 860, 300, 200), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        for (int i = 0; i < textureCount; i++) {
            if (nk_button_label(ctx, textureNames[i])) {
                GLuint newTexture = getTexture(textureNames[i]);
                if (newTexture != 0) {
                    GLuint* clonedTextureID = (GLuint*)malloc(sizeof(GLuint));
                    *clonedTextureID = newTexture;
                    selected_object->object.textureID = *clonedTextureID;
                    selected_object->object.useTexture = true;
                    selected_object->object.usePBR = false;
                    selected_object->object.useColor = false;
                    updateObjectInManager(selected_object);
                }
            }
        }
        nk_end(ctx);
    }
    else {
        show_texture_window = false;
        nk_end(ctx);
    }
}


// Import model function
void import_model() {
    char const* filterPatterns[1] = { "*.obj" };
    char const* filePath = tinyfd_openFileDialog("Import Model", "", 1, filterPatterns, "Object Files", 0);

    if (!filePath) {
        fprintf(stderr, "Import operation cancelled or failed to get a valid path.\n");
        return;
    }

    Model* model = loadModel(filePath);
    if (model) {
        PBRMaterial defaultMaterial = { 0 };
        addObjectWithAction(OBJ_MODEL, false, -1, true, model, defaultMaterial, false);
    }
}

void cut_object() {
    if (selected_object) {
        int index = find_selected_object_index(selected_object);
        if (index != -1) {
            clipboard_object = (SceneObject*)malloc(sizeof(SceneObject));
            if (clipboard_object) {
                *clipboard_object = *selected_object;
                isCutOperation = true;
                removeObjectWithAction(index);
                selected_object = NULL;
                printf("Cut object at index: %d\n", index);
            }
        }
    }
}

void copy_object() {
    if (selected_object) {
        clipboard_object = (SceneObject*)malloc(sizeof(SceneObject));
        if (clipboard_object) {
            *clipboard_object = *selected_object;
            if (selected_object->object.type == OBJ_MODEL) {
                clipboard_object->object.data.model.meshes = (Mesh*)malloc(selected_object->object.data.model.meshCount * sizeof(Mesh));
                for (unsigned int i = 0; i < selected_object->object.data.model.meshCount; i++) {
                    clipboard_object->object.data.model.meshes[i] = selected_object->object.data.model.meshes[i];
                }
            }
            isCutOperation = false;
        }
    }
}

void paste_object() {
    if (clipboard_object) {
        SceneObject newObject = *clipboard_object;
        if (clipboard_object->object.type == OBJ_MODEL) {
            newObject.object.data.model.meshes = (Mesh*)malloc(clipboard_object->object.data.model.meshCount * sizeof(Mesh));
            for (unsigned int i = 0; i < clipboard_object->object.data.model.meshCount; i++) {
                newObject.object.data.model.meshes[i] = clipboard_object->object.data.model.meshes[i];
            }
        }

        if (isCutOperation) {
            addObjectWithAction(newObject.object.type, newObject.object.useTexture, newObject.object.textureID, newObject.object.useColor,
                (newObject.object.type == OBJ_MODEL ? &newObject.object.data.model : NULL), newObject.object.material, newObject.object.usePBR);
            free(clipboard_object);
            clipboard_object = NULL;
            isCutOperation = false;
        }
        else {
            addObjectWithAction(newObject.object.type, newObject.object.useTexture, newObject.object.textureID, newObject.object.useColor,
                (newObject.object.type == OBJ_MODEL ? &newObject.object.data.model : NULL), newObject.object.material, newObject.object.usePBR);
        }
        selected_object = &objectManager.objects[objectManager.count - 1];
    }
}

void close_object_windows() {
    show_material_window = false;
    show_texture_window = false;
    show_color_picker = false;
}


// Debug window function
void debug_window(struct nk_context* ctx, int x, int y, int width, int height) {
    char buffer[512];

    if (nk_begin(ctx, "Debug Information", nk_rect(x, y, width, height), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
        nk_layout_row_dynamic(ctx, 15, 1);

        // Displaying screen width
        sprintf(buffer, "Width: %d", screen.width);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Displaying screen height
        sprintf(buffer, "Height: %d", screen.height);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Displaying screen title
        sprintf(buffer, "Title: %s", screen.title);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Displaying OpenGL version
        sprintf(buffer, "OpenGL version: %s", glGetString(GL_VERSION));
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Displaying the camera's position
        sprintf(buffer, "Camera Position: (%.2f, %.2f, %.2f)", camera.Position.x, camera.Position.y, camera.Position.z);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Displaying the camera's front vector
        sprintf(buffer, "Front: (%.2f, %.2f, %.2f)", camera.Front.x, camera.Front.y, camera.Front.z);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Displaying the camera's yaw and pitch
        sprintf(buffer, "Yaw: %.2f, Pitch: %.2f", camera.Yaw, camera.Pitch);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // More details...
        sprintf(buffer, "Movement Speed: %.2f", camera.MovementSpeed);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        sprintf(buffer, "Mouse Sensitivity: %.2f", camera.MouseSensitivity);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        sprintf(buffer, "Zoom Level: %.2f", camera.Zoom);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        sprintf(buffer, "Invert Y Axis: %d", camera.invertY);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Texture and color settings
        sprintf(buffer, "Textures Enabled: %d", texturesEnabled);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        sprintf(buffer, "Color Creation Mode: %d", colorsEnabled);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        sprintf(buffer, "Light Shading: %d", lightingEnabled);
        nk_label(ctx, buffer, NK_TEXT_LEFT);

        // Light details
        nk_label(ctx, "Light Details:", NK_TEXT_LEFT);
        for (int i = 0; i < lightCount; i++) {
            sprintf(buffer, "Light %d: Pos (%.2f, %.2f, %.2f) Color (%.2f, %.2f, %.2f) Intensity %.2f",
                i, lights[i].position.x, lights[i].position.y, lights[i].position.z,
                lights[i].color.x, lights[i].color.y, lights[i].color.z,
                lights[i].intensity);
            nk_label(ctx, buffer, NK_TEXT_LEFT);
        }

        nk_end(ctx);
    }
    else {
        nk_end(ctx);
    }
}

// Setup Nuklear GUI
void setup_nuklear(GLFWwindow* existingWindow) {
    window = existingWindow;
    ctx = nk_glfw3_init(window, NK_GLFW3_INSTALL_CALLBACKS);
    struct nk_font_atlas* atlas;
    nk_glfw3_font_stash_begin(&atlas);
    roboto_font = nk_font_atlas_add_from_file(atlas, "resources/font/Roboto-Light.ttf", 16, 0);
    nk_glfw3_font_stash_end();
    nk_style_set_font(ctx, &roboto_font->handle);
    set_theme(theme_dark);
}

void start_engine() {
    // Save the current selected object to restore after starting the engine
    SceneObject* saved_selected_object = selected_object;
    isRunning = true;
    selected_object = saved_selected_object;
}

// Set theme function
void set_theme(bool dark_theme) {
    theme_dark = dark_theme;
    struct nk_color table[NK_COLOR_COUNT];
    if (dark_theme) {
        table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(30, 30, 30, 255);
        table[NK_COLOR_HEADER] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_BORDER] = nk_rgba(60, 60, 60, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(50, 50, 50, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(35, 35, 35, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(40, 40, 40, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(50, 50, 50, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_SELECT] = nk_rgba(50, 50, 50, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(38, 38, 38, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(100, 100, 100, 255);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(120, 120, 120, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(38, 38, 38, 255);
        table[NK_COLOR_EDIT] = nk_rgba(38, 38, 38, 255);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_COMBO] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_CHART] = nk_rgba(120, 120, 120, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 30, 30, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 64, 64, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(80, 80, 80, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(40, 40, 40, 255);
    }
    else {
        table[NK_COLOR_TEXT] = nk_rgba(20, 20, 20, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_HEADER] = nk_rgba(235, 235, 235, 255);
        table[NK_COLOR_BORDER] = nk_rgba(200, 200, 200, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(240, 240, 240, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(235, 235, 235, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(220, 220, 220, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(225, 225, 225, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(215, 215, 215, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
        table[NK_COLOR_SELECT] = nk_rgba(220, 220, 220, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(245, 245, 245, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(150, 150, 150, 255);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(130, 130, 130, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(100, 100, 100, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(245, 245, 245, 255);
        table[NK_COLOR_EDIT] = nk_rgba(245, 245, 245, 255);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(20, 20, 20, 255);
        table[NK_COLOR_COMBO] = nk_rgba(235, 235, 235, 255);
        table[NK_COLOR_CHART] = nk_rgba(120, 120, 120, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(235, 235, 235, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(180, 180, 180, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(170, 170, 170, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(220, 220, 220, 255);
    }
    nk_style_from_table(ctx, table);
}

// Toggle theme function
void toggle_theme() {
    set_theme(!theme_dark);
}

// Framebuffer size callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowed_width = width;
    windowed_height = height;
}

// Toggle fullscreen function
void toggle_fullscreen(GLFWwindow* window) {
    static int windowed_xpos, windowed_ypos, windowed_width, windowed_height;
    static int is_fullscreen = 0;

    if (is_fullscreen) {
        glfwSetWindowMonitor(window, NULL, windowed_xpos, windowed_ypos, windowed_width, windowed_height, 0);
        is_fullscreen = 0;
    }
    else {
        glfwGetWindowPos(window, &windowed_xpos, &windowed_ypos);
        glfwGetWindowSize(window, &windowed_width, &windowed_height);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        is_fullscreen = 1;
    }
}

// Resize callback
void resize_callback(GLFWwindow* window, int width, int height) {
    printf("Resizing: width=%d, height=%d\n", width, height);
    windowed_width = width;
    windowed_height = height;
    glViewport(0, 0, width, height);
}


// Key callback function
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        toggle_fullscreen(window);
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);  
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        isRunning = !isRunning; 
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        toggle_theme();
    }
    if (key == GLFW_KEY_Z && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        undo_last_action();
    }
    if (key == GLFW_KEY_Y && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        redo_last_action();
    }
    if (key == GLFW_KEY_X && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        cut_object();
    }
    if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        copy_object();
    }
    if (key == GLFW_KEY_V && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS) {
        paste_object();
    }
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
        if (selected_object) {
            int index = selected_object - objectManager.objects;
            removeObjectWithAction(index);
            selected_object = NULL;
        }
    }
}

// Display loading screen function
void display_loading_screen(const char* stage, float progress) {
    nk_glfw3_new_frame();  // Start Nuklear frame

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    char loading_text[256];
    snprintf(loading_text, sizeof(loading_text), "%s", stage);

    if (nk_begin(ctx, "Loading", nk_rect((window_width - 400) / 2, (window_height - 200) / 2, 400, 200), 
                 NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT)) {
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, loading_text, NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 30, 1);
        nk_progress(ctx, (nk_size*)&progress, 100, NK_FIXED);
    }
    nk_end(ctx);

    nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void generate_new_frame() {
    nk_glfw3_new_frame(); 
}

// Run loading screen function
void run_loading_screen(GLFWwindow* window) {
    if (!ctx) return;  // Ensure Nuklear is initialized

    const char* stages[] = {
        "Initializing...",
        "Loading Textures...",
        "Loading PBR Textures...",
        "Setting Up Skybox...",
        "Setting Up Lighting...",
        "Finalizing..."
    };
    int num_stages = sizeof(stages) / sizeof(stages[0]);
    float progress = 0.0f;
    float progressIncrement = 1.0f / (float)num_stages;

    for (int i = 0; i < num_stages; ++i) {
        printf("Stage: %s, Progress: %.2f%%\n", stages[i], progress * 100);
        loadResources(i, &progress);
        progress += progressIncrement;
        display_loading_screen(stages[i], progress);  // FIX: Remove `ctx`
    }
    display_loading_screen("Loading Complete", 1.0f);
}


void select_object(int index) {
    if (index >= 0 && index < objectManager.count) {
        selected_object = &objectManager.objects[index];
        printf("Selected object: ID=%d, Index=%d\n", selected_object->id, index);
    }
}

// Hierarchy window function
void hierarchy_window(struct nk_context* ctx, int hierarchyX, int hierarchyY, int hierarchyWidth, int hierarchyHeight) {
    if (nk_begin(ctx, "Hierarchy", nk_rect(hierarchyX, hierarchyY, hierarchyWidth, hierarchyHeight), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
        nk_layout_row_dynamic(ctx, 20, 1);

        for (int i = 0; i < objectManager.count; i++) {
            SceneObject* sceneObj = &objectManager.objects[i];
            char label[128];

            const char* typeName = objectTypeName(sceneObj->object.type);
            snprintf(label, sizeof(label), "%d. %s", i + 1, typeName);

            if (nk_button_label(ctx, label)) {
                select_object(i);  // Use the new selection function
            }

            if (nk_contextual_begin(ctx, 0, nk_vec2(150, 300), nk_window_get_bounds(ctx))) {
                nk_layout_row_dynamic(ctx, 30, 1);

                // Check if a valid object is selected before showing context options
                if (selected_object) {
                    if (nk_contextual_item_label(ctx, "Change Color", NK_TEXT_CENTERED)) {
                        show_color_picker = true;
                    }
                    if (nk_contextual_item_label(ctx, "Change Material", NK_TEXT_CENTERED)) {
                        show_material_window = true;
                    }
                    if (nk_contextual_item_label(ctx, "Change Texture", NK_TEXT_CENTERED)) {
                        show_texture_window = true;
                    }
                    if (nk_contextual_item_label(ctx, "Toggle Use PBR", NK_TEXT_CENTERED)) {
                        selected_object->object.usePBR = !selected_object->object.usePBR;
                        updateObjectInManager(selected_object);
                    }
                    if (nk_contextual_item_label(ctx, "Toggle Use Texture", NK_TEXT_CENTERED)) {
                        selected_object->object.useTexture = !selected_object->object.useTexture;
                        updateObjectInManager(selected_object);
                    }
                    if (nk_contextual_item_label(ctx, "Toggle Use Color", NK_TEXT_CENTERED)) {
                        selected_object->object.useColor = !selected_object->object.useColor;
                        updateObjectInManager(selected_object);
                    }
                    if (nk_contextual_item_label(ctx, "Delete", NK_TEXT_CENTERED)) {
                        removeObject(i);
                        selected_object = NULL;
                    }
                }
                else {
                    nk_label(ctx, "No object selected", NK_TEXT_CENTERED);
                }

                nk_contextual_end(ctx);
            }

            if (selected_object == sceneObj) {
                nk_layout_row_dynamic(ctx, 20, 1);
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "Type: %s", typeName);
                nk_label(ctx, buffer, NK_TEXT_LEFT);
                snprintf(buffer, sizeof(buffer), "Position: (%.2f, %.2f, %.2f)", sceneObj->position.x, sceneObj->position.y, sceneObj->position.z);
                nk_label(ctx, buffer, NK_TEXT_LEFT);
                snprintf(buffer, sizeof(buffer), "Rotation: (%.2f, %.2f, %.2f)", sceneObj->rotation.x, sceneObj->rotation.y, sceneObj->rotation.z);
                nk_label(ctx, buffer, NK_TEXT_LEFT);
                snprintf(buffer, sizeof(buffer), "Scale: (%.2f, %.2f, %.2f)", sceneObj->scale.x, sceneObj->scale.y, sceneObj->scale.z);
                nk_label(ctx, buffer, NK_TEXT_LEFT);

                snprintf(buffer, sizeof(buffer), "Use Color: %s", sceneObj->object.useColor ? "Yes" : "No");
                nk_label(ctx, buffer, NK_TEXT_LEFT);
                snprintf(buffer, sizeof(buffer), "Use Texture: %s", sceneObj->object.useTexture ? "Yes" : "No");
                nk_label(ctx, buffer, NK_TEXT_LEFT);
                snprintf(buffer, sizeof(buffer), "Use PBR: %s", sceneObj->object.usePBR ? "Yes" : "No");
                nk_label(ctx, buffer, NK_TEXT_LEFT);
                if (nk_button_label(ctx, "Delete Object")) {
                    removeObject(i);
                    selected_object = NULL;
                }
            }
        }
        nk_end(ctx);
    }

    if (show_material_window) {
        material_picker_window(ctx);
    }

    if (show_texture_window) {
        texture_picker_window(ctx);
    }
}

// Toggle object property function
void toggle_object_property(SceneObject* obj, const char* property) {
    if (obj) {
        if (strcmp(property, "usePBR") == 0) {
            obj->object.usePBR = !obj->object.usePBR;
        }
        else if (strcmp(property, "useTexture") == 0) {
            obj->object.useTexture = !obj->object.useTexture;
        }
        else if (strcmp(property, "useColor") == 0) {
            obj->object.useColor = !obj->object.useColor;
        }
        else if (strcmp(property, "useLighting") == 0) {
            obj->object.useLighting = !obj->object.useLighting;
        }
        updateObjectInManager(obj);
    }
}

// Reset GUI function
void reset_gui() {
    show_about = false;
    show_debug = false;
    show_hierarchy = true;
    show_inspector = true;
    show_color_picker = false;
    show_history = false;
    show_settings = false;
    show_controls = false;
    show_change_background = false;
    show_object_creator = false;
    selected_object = NULL; 
}

// Inspector window function
void inspector_window(struct nk_context* ctx, int inspectorX, int inspectorY, int inspectorWidth, int inspectorHeight) {
    if (selected_object != NULL) {
        if (nk_begin(ctx, "Inspector", nk_rect(inspectorX, inspectorY, inspectorWidth, inspectorHeight), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE)) {
            nk_layout_row_dynamic(ctx, 25, 1);

            nk_label(ctx, "Position", NK_TEXT_LEFT);
            nk_property_float(ctx, "#X:", -100.0f, &selected_object->position.x, 100.0f, 0.1f, 0.1f);
            nk_property_float(ctx, "#Y:", -100.0f, &selected_object->position.y, 100.0f, 0.1f, 0.1f);
            nk_property_float(ctx, "#Z:", -100.0f, &selected_object->position.z, 100.0f, 0.1f, 0.1f);

            nk_label(ctx, "Rotation", NK_TEXT_LEFT);
            nk_property_float(ctx, "#X:", -360.0f, &selected_object->rotation.x, 360.0f, 1.0f, 1.0f);
            nk_property_float(ctx, "#Y:", -360.0f, &selected_object->rotation.y, 360.0f, 1.0f, 1.0f);
            nk_property_float(ctx, "#Z:", -360.0f, &selected_object->rotation.z, 360.0f, 1.0f, 1.0f);

            nk_label(ctx, "Scale", NK_TEXT_LEFT);
            nk_property_float(ctx, "#X:", 0.1f, &selected_object->scale.x, 10.0f, 0.1f, 0.1f);
            nk_property_float(ctx, "#Y:", 0.1f, &selected_object->scale.y, 10.0f, 0.1f, 0.1f);
            nk_property_float(ctx, "#Z:", 0.1f, &selected_object->scale.z, 10.0f, 0.1f, 0.1f);

            nk_label(ctx, "Color", NK_TEXT_LEFT);
            nk_property_float(ctx, "#R:", 0.0f, &selected_object->color.x, 1.0f, 0.01f, 0.01f);
            nk_property_float(ctx, "#G:", 0.0f, &selected_object->color.y, 1.0f, 0.01f, 0.01f);
            nk_property_float(ctx, "#B:", 0.0f, &selected_object->color.z, 1.0f, 0.01f, 0.01f);

            nk_end(ctx);
        }
        else {
            show_inspector = false;
            nk_end(ctx);
        }
    }
}

// Get object type name function
const char* objectTypeName(ObjectType type) {
    switch (type) {
    case OBJ_CUBE: return "Cube";
    case OBJ_SPHERE: return "Sphere";
    case OBJ_PYRAMID: return "Pyramid";
    case OBJ_CYLINDER: return "Cylinder";
    case OBJ_PLANE: return "Plane";
    case OBJ_MODEL: return "Model";
    default: return "Unknown";
    }
}

// Show about window function
void show_about_window(struct nk_context* ctx) {
    float about_width = 400; 
    float about_height = 200; 
    if (nk_begin(ctx, "About", nk_rect((windowed_width - about_width) / 2, (windowed_height - about_height) / 2, about_width, about_height), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_static(ctx, 20, 180, 1);
        nk_label(ctx, "C1ue Engine v1.1.0", NK_TEXT_CENTERED);
        nk_label(ctx, "Made by ClueSec", NK_TEXT_CENTERED);
        nk_label(ctx, "=== ClueSec 2024-2025 ===", NK_TEXT_CENTERED);
        nk_end(ctx);
    }
    else {
        show_about = false;
        nk_end(ctx);
    }
}

// Show controls window function
void show_controls_window(struct nk_context* ctx) {
    if (nk_begin(ctx, "Controls", nk_rect(300, 200, 400, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "3D ENGINE CONTROLS:", NK_TEXT_LEFT);
        nk_label(ctx, "W/A/S/D: Move camera", NK_TEXT_LEFT);
        nk_label(ctx, "Space: Move up", NK_TEXT_LEFT);
        nk_label(ctx, "Shift: Move down", NK_TEXT_LEFT);
        nk_label(ctx, "Mouse: Look around", NK_TEXT_LEFT);
        nk_label(ctx, "Left Click + Left Alt: Drag camera while engine is turned off", NK_TEXT_LEFT);
        nk_label(ctx, "Right Click: Pan camera while engine is turned off", NK_TEXT_LEFT);
        nk_label(ctx, "F: Toggle fullscreen", NK_TEXT_LEFT);
        nk_label(ctx, "P: Pause/Resume engine", NK_TEXT_LEFT);
        nk_label(ctx, "ESC: Exit", NK_TEXT_LEFT);

        nk_label(ctx, "GUI CONTROLS:", NK_TEXT_LEFT);
        nk_label(ctx, "F1: Change the light/dark theme", NK_TEXT_LEFT);
        nk_label(ctx, "Ctrl + C: Copy selected object", NK_TEXT_LEFT);
        nk_label(ctx, "Ctrl + V: Paste copied object", NK_TEXT_LEFT);
        nk_label(ctx, "Ctrl + X: Cut selected object", NK_TEXT_LEFT);
        nk_label(ctx, "Delete: Remove selected object", NK_TEXT_LEFT);
        nk_label(ctx, "Ctrl + Z: Undo last action", NK_TEXT_LEFT);
        nk_label(ctx, "Ctrl + Y: Redo last action", NK_TEXT_LEFT);
        nk_label(ctx, "T: Toggle Textures", NK_TEXT_LEFT);
        nk_label(ctx, "L: Toggle Colours", NK_TEXT_LEFT);
        nk_label(ctx, "J: Toggle Shading", NK_TEXT_LEFT);
        nk_label(ctx, "Q: Toggle PBR", NK_TEXT_LEFT);
        nk_label(ctx, "O: Create a plane", NK_TEXT_LEFT);
        nk_label(ctx, "C: Create a cube", NK_TEXT_LEFT);
        nk_label(ctx, "H: Create a pyramid", NK_TEXT_LEFT);
        nk_label(ctx, "K: Create a sphere", NK_TEXT_LEFT);
        nk_label(ctx, "B: Create a cylinder", NK_TEXT_LEFT);
        nk_label(ctx, "I: Create light", NK_TEXT_LEFT);
        nk_end(ctx);
    }
    else {
        show_controls = false;
        nk_end(ctx);
    }
}

// Color picker window function
void color_picker_window(struct nk_context* ctx) {
    static struct nk_colorf color = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (selected_object && nk_begin(ctx, "Color Picker", nk_rect(200, 200, 300, 400), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 120, 1);

        // Set the color to the selected object's color
        color.r = selected_object->color.x;
        color.g = selected_object->color.y;
        color.b = selected_object->color.z;
        color.a = selected_object->color.w;

        // Display the color picker
        color = nk_color_picker(ctx, color, NK_RGB);

        nk_layout_row_dynamic(ctx, 25, 1);
        color.r = nk_propertyf(ctx, "#R:", 0, color.r, 1.0f, 0.01f, 0.005f);
        color.g = nk_propertyf(ctx, "#G:", 0, color.g, 1.0f, 0.01f, 0.005f);
        color.b = nk_propertyf(ctx, "#B:", 0, color.b, 1.0f, 0.01f, 0.005f);

        // Update the selected object's color
        if (selected_object) {
            selected_object->color = (Vector4){ color.r, color.g, color.b, 1.0f }; 
            updateObjectInManager(selected_object); 
        }
        nk_end(ctx);
    }
    else {
        show_color_picker = false;
        nk_end(ctx);
    }
}


// Settings window function
void settings_window(struct nk_context* ctx) {
    static int width = 1920, height = 1080;
    static struct nk_colorf bg_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    static float camera_speed;
    static float movement_speed;
    
    // AUDIO CONTROLS
    #ifdef AUDIO_ENABLED
    static float master_volume = 1.0f;
    static bool audio_initialized = false;
    static bool first_time = true;
    
    if (first_time) {
        master_volume = getMasterVolume();
        first_time = false;
    }
    #endif

    // Initialize width and height once, not every frame
    static int initialized = 0;
    if (!initialized) {
        glfwGetWindowSize(window, &width, &height);
        initialized = 1;
    }

    camera_speed = camera.MouseSensitivity;
    movement_speed = camera.MovementSpeed;

    if (nk_begin(ctx, "Settings", nk_rect(400, 200, 320, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label(ctx, "Settings Panel", NK_TEXT_CENTERED);

        // VIDEO SETTINGS
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Video Settings", NK_TEXT_LEFT);
        
        nk_property_int(ctx, "Width:", 800, &width, 3840, 10, 1);
        nk_property_int(ctx, "Height:", 600, &height, 2160, 10, 1);

        if (nk_button_label(ctx, "Apply Resolution")) {
            glfwSetWindowSize(window, width, height);
        }

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_float(ctx, "Camera Speed:", 0.1f, &camera_speed, 10.0f, 0.1f, 0.1f);
        nk_property_float(ctx, "Movement Speed:", 0.1f, &movement_speed, 50.0f, 0.1f, 0.1f);

        // Apply the changes to the camera settings
        camera.MovementSpeed = movement_speed;
        camera.MouseSensitivity = camera_speed;

        // AUDIO SETTINGS SECTION
        #ifdef AUDIO_ENABLED
        nk_layout_row_dynamic(ctx, 10, 1); // Spacer
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Audio Settings", NK_TEXT_LEFT);
        
        // Master Volume Slider
        nk_layout_row_dynamic(ctx, 25, 1);
        float old_volume = master_volume;
        master_volume = nk_slide_float(ctx, 0.0f, master_volume, 1.0f, 0.01f);
        if (master_volume != old_volume) {
            setMasterVolume(master_volume);
        }
        
        // Volume display
        char volume_text[64];
        snprintf(volume_text, sizeof(volume_text), "Master Volume: %.0f%%", master_volume * 100);
        nk_label(ctx, volume_text, NK_TEXT_LEFT);
        
        // Test Audio Buttons
        nk_layout_row_dynamic(ctx, 25, 2);
        if (nk_button_label(ctx, "Test Sound")) {
            playSound("resources/audio/test_sound.wav");
        }
        if (nk_button_label(ctx, "Stop All")) {
            stopAllSounds();
        }
        
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Play Music")) {
            playMusic("resources/audio/background_music.ogg", true);
        }
        
        // 3D Audio Test
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Test 3D Audio")) {
            Vector3 testPos = vector_add(camera.Position, vector_scale(camera.Front, 5.0f));
            playSound3D("resources/audio/test_sound.wav", testPos);
        }
        #else
        // Audio disabled message
        nk_layout_row_dynamic(ctx, 10, 1); // Spacer
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Audio: DISABLED", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label(ctx, "(Rebuild with OpenAL)", NK_TEXT_LEFT);
        #endif

        // BACKGROUND COLOR PICKER
        nk_layout_row_dynamic(ctx, 10, 1); // Spacer
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Background Color", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 120, 1);
        bg_color = nk_color_picker(ctx, bg_color, NK_RGBA);

        if (nk_button_label(ctx, "Apply Background Color")) {
            glClearColor(bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        }
        
        nk_end(ctx);
    }
    else {
        show_settings = false;
        nk_end(ctx);
    }
}




// Add point light function
void add_point_light() {
    Vector3 position = vector_add(camera.Position, vector_scale(camera.Front, 5.0f));
    createLight(position, camera.Front, vector(1.0f, 1.0f, 1.0f), 1.0f, LIGHT_POINT);
}

// Add directional light function
void add_directional_light() {
    createLight(camera.Position, camera.Front, vector(1.0f, 1.0f, 1.0f), 1.0f, LIGHT_DIRECTIONAL);
}

// Add spotlight function
void add_spotlight() {
    Vector3 position = vector_add(camera.Position, vector_scale(camera.Front, 5.0f));
    createLight(position, camera.Front, vector(1.0f, 1.0f, 1.0f), 1.0f, LIGHT_SPOT);
}

// File menu function
void file_menu(struct nk_context* ctx) {
    if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(120, 200))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "New Project", NK_TEXT_LEFT)) {
            new_project();  // Reset project data
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/project_new.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Open Project...", NK_TEXT_LEFT)) {
            load_project();  // Invoke load with file dialog
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/project_load.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Save Project", NK_TEXT_LEFT)) {
            save_project();  // Invoke save with file dialog
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/project_save.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
            glfwSetWindowShouldClose(window, 1);
        }
        nk_menu_end(ctx);
    }
}

// Edit menu function
void edit_menu(struct nk_context* ctx) {
    if (nk_menu_begin_label(ctx, "Edit", NK_TEXT_LEFT, nk_vec2(120, 200))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "Undo", NK_TEXT_LEFT)) {
            undo_last_action();
        }
        if (nk_menu_item_label(ctx, "Redo", NK_TEXT_LEFT)) {
            redo_last_action();
        }
        if (nk_menu_item_label(ctx, "Cut", NK_TEXT_LEFT)) {
            if (selected_object) {
                cut_object();
            }
        }
        if (nk_menu_item_label(ctx, "Copy", NK_TEXT_LEFT)) {
            if (selected_object) {
                copy_object();
            }
        }
        if (nk_menu_item_label(ctx, "Paste", NK_TEXT_LEFT)) {
            paste_object();
        }
        if (nk_menu_item_label(ctx, "Import Model", NK_TEXT_LEFT)) {
            selectedType = SELECT_MODEL;
            import_model();
        }
        nk_menu_end(ctx);
    }
}

// Change background function
void change_background(int backgroundIndex) {
    if (backgroundIndex < 1 || backgroundIndex > 8) {
        fprintf(stderr, "Background index out of range. Please choose from 1 to 8.\n");
        return;
    }
    initSkybox(backgroundIndex);
}

// Change background window function
void change_background_window(struct nk_context* ctx) {
    if (nk_begin(ctx, "Change Background", nk_rect(300, 200, 250, 300), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        for (int i = 1; i <= 8; i++) {
            char label[20];
            snprintf(label, sizeof(label), "Background %d", i);
            if (nk_button_label(ctx, label)) {
                change_background(i);
            }
        }
        if (nk_button_label(ctx, "Close")) {
            show_change_background = false;
        }
        nk_end(ctx);
    }
    else {
        show_change_background = false;
        nk_end(ctx);
    }
}

// View menu function
void view_menu(struct nk_context* ctx) {
    if (nk_menu_begin_label(ctx, "View", NK_TEXT_LEFT, nk_vec2(200, 400))) {
        nk_layout_row_dynamic(ctx, 25, 1);

        if (nk_menu_item_label(ctx, "Toggle Fullscreen", NK_TEXT_LEFT)) {
            toggle_fullscreen(window);  
        }
        if (nk_menu_item_label(ctx, "Toggle Debug Info", NK_TEXT_LEFT)) {
            show_debug = !show_debug; 
        }
        if (nk_menu_item_label(ctx, "Toggle Background", NK_TEXT_LEFT)) {
            backgroundEnabled = !backgroundEnabled;
        }
        if (nk_menu_item_label(ctx, "Change Background", NK_TEXT_LEFT)) {
            show_change_background = true;
        }
        if (nk_menu_item_label(ctx, "Toggle Textures", NK_TEXT_LEFT)) {
            for (int i = 0; i < objectManager.count; i++) {
                toggle_object_property(&objectManager.objects[i], "useTexture");
            }
        }
        if (nk_menu_item_label(ctx, "Toggle Shading", NK_TEXT_LEFT)) {
            lightingEnabled = !lightingEnabled;
        }
        if (nk_menu_item_label(ctx, "Toggle Colors", NK_TEXT_LEFT)) {
            for (int i = 0; i < objectManager.count; i++) {
                toggle_object_property(&objectManager.objects[i], "useColor");
            }
        }
        if (nk_menu_item_label(ctx, "Toggle PBR", NK_TEXT_LEFT)) {
            for (int i = 0; i < objectManager.count; i++) {
                toggle_object_property(&objectManager.objects[i], "usePBR");
            }
        }
        if (nk_menu_item_label(ctx, isRunning ? "Pause Engine" : "Start Engine", NK_TEXT_LEFT)) {
            if (isRunning) {
                isRunning = false;
            }
            else {
                start_engine();
            }
        }

        nk_menu_end(ctx);
    }
}

// Window menu function
void window_menu(struct nk_context* ctx) {
    if (nk_menu_begin_label(ctx, "Window", NK_TEXT_LEFT, nk_vec2(120, 200))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "Reset GUI", NK_TEXT_LEFT)) {
            reset_gui();
        }
        if (nk_menu_item_label(ctx, "Toggle Hierarchy", NK_TEXT_LEFT)) {
            show_hierarchy = !show_hierarchy;
        }
        if (nk_menu_item_label(ctx, "Toggle Inspector", NK_TEXT_LEFT)) {
            show_inspector = !show_inspector;
        }
        if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT)) {
            show_settings = true;
        }
        nk_menu_end(ctx);
    }
}

// Help menu function
void help_menu(struct nk_context* ctx) {
    if (nk_menu_begin_label(ctx, "Help", NK_TEXT_LEFT, nk_vec2(120, 200))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
            show_about = true;
        }
        if (nk_menu_item_label(ctx, "Controls", NK_TEXT_LEFT)) {
            show_controls = true; 
        }
        nk_menu_end(ctx);
    }
}

// Object menu function
void object_menu(struct nk_context* ctx) {
    if (nk_menu_begin_label(ctx, "Objects", NK_TEXT_LEFT, nk_vec2(200, 250))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_menu_item_label(ctx, "Add Cube", NK_TEXT_LEFT)) {
            addObject(&camera, OBJ_CUBE, texturesEnabled, textureIndex, colorsEnabled, NULL, *getMaterial("peacockOre"), true);
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/object_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Sphere", NK_TEXT_LEFT)) {
            addObject(&camera, OBJ_SPHERE, texturesEnabled, textureIndex, colorsEnabled, NULL, *getMaterial("peacockOre"), true);
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/object_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Pyramid", NK_TEXT_LEFT)) {
            addObject(&camera, OBJ_PYRAMID, texturesEnabled, textureIndex, colorsEnabled, NULL, *getMaterial("peacockOre"), true);
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/object_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Plane", NK_TEXT_LEFT)) {
            addObject(&camera, OBJ_PLANE, texturesEnabled, textureIndex, colorsEnabled, NULL, *getMaterial("peacockOre"), true);
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/object_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Cylinder", NK_TEXT_LEFT)) {
            addObject(&camera, OBJ_CYLINDER, texturesEnabled, textureIndex, colorsEnabled, NULL, *getMaterial("peacockOre"), true);
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/object_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Point Light", NK_TEXT_LEFT)) {
            add_point_light(); // Add point light
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/light_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Directional Light", NK_TEXT_LEFT)) {
            add_directional_light(); // Add directional light
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/light_create.wav");
            #endif
        }
        if (nk_menu_item_label(ctx, "Add Spotlight", NK_TEXT_LEFT)) {
            add_spotlight(); // Add spotlight
            #ifdef AUDIO_ENABLED
            playSound("resources/audio/light_create.wav");
            #endif
        }
        nk_menu_end(ctx);
    }
}


// History window function
void history_window(struct nk_context* ctx) {
    if (nk_begin(ctx, "History", nk_rect(50, 50, 400, 600), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
        nk_layout_row_dynamic(ctx, 25, 1);
        for (int i = 0; i < historyCount; i++) {
            nk_label(ctx, actionHistory[i].description, NK_TEXT_LEFT);
        }
        nk_end(ctx);
    }
}



// Main GUI function
void main_gui() {
    // Retrieve monitor resolution
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    int monitorWidth = mode->width;
    int monitorHeight = mode->height;

    int menuHeight = 35;
    int hierarchyWidth = monitorWidth * 0.2; // 20% of monitor width
    int inspectorWidth = monitorWidth * 0.2; // 20% of monitor width
    int mainViewWidth = monitorWidth - hierarchyWidth - inspectorWidth;
    int mainViewHeight = monitorHeight - menuHeight;
    int hierarchyHeight = mainViewHeight;
    int inspectorHeight = mainViewHeight;

    int hierarchyX = 0;
    int hierarchyY = menuHeight;
    int inspectorX = hierarchyWidth + mainViewWidth;
    int inspectorY = menuHeight;
    int mainViewX = hierarchyWidth;
    int mainViewY = menuHeight;

    if (nk_begin(ctx, "Menu", nk_rect(0, 0, monitorWidth, menuHeight), NK_WINDOW_NO_SCROLLBAR)) {
        nk_menubar_begin(ctx);
        nk_layout_row_static(ctx, 25, 80, 6);

        file_menu(ctx);
        edit_menu(ctx);
        view_menu(ctx);
        object_menu(ctx);
        window_menu(ctx);
        help_menu(ctx);

        nk_menubar_end(ctx);
    }
    nk_end(ctx);

    if (show_hierarchy && !isRunning) {
        hierarchy_window(ctx, hierarchyX, hierarchyY, hierarchyWidth, hierarchyHeight);
    }

    if (show_about) {
        show_about_window(ctx);
    }

    if (show_debug) {
        int debug_window_x = inspectorX - 200;
        int debug_window_y = menuHeight;
        int debug_window_width = 200;
        int debug_window_height = 600;

        debug_window(ctx, debug_window_x, debug_window_y, debug_window_width, debug_window_height);
    }

    if (show_color_picker && !isRunning) {
        color_picker_window(ctx);
    }

    // Show inspector window if an object is selected
    if (selected_object != NULL && show_inspector && !isRunning) {
        inspector_window(ctx, inspectorX, inspectorY, inspectorWidth, inspectorHeight);
    }

    if (show_settings) {
        settings_window(ctx);
    }

    if (show_history) {
        history_window(ctx);
    }

    if (show_change_background) {
        change_background_window(ctx);
    }

    if (show_controls) {
        show_controls_window(ctx);
    }
}

// Teardown Nuklear function
void teardown_nuklear() {
    nk_glfw3_shutdown();
}

// Render Nuklear function
void render_nuklear() {
    nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}


