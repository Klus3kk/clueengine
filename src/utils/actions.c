#include "actions.h"
#include "ObjectManager.h"
#include "Camera.h"
#include "globals.h"
#include "SceneObject.h"
#include "ModelLoad.h"
#include <string.h>

#ifdef AUDIO_ENABLED
#include "audio.h"
#endif

// Define the stacks for undo and redo
Action undoStack[MAX_ACTIONS];
Action redoStack[MAX_ACTIONS];
int undoTop = -1;
int redoTop = -1;

// Define the action history
Action actionHistory[MAX_ACTIONS];
int historyCount = 0;

void pushUndoAction(Action action) {
    if (undoTop < MAX_ACTIONS - 1) {
        undoStack[++undoTop] = action;
        redoTop = -1; // Clear redo stack whenever a new action is performed
    }
}

Action popUndoAction() {
    if (undoTop >= 0) {
        return undoStack[undoTop--];
    }
    return (Action) { .type = -1 };
}

void pushRedoAction(Action action) {
    if (redoTop < MAX_ACTIONS - 1) {
        redoStack[++redoTop] = action;
    }
}

Action popRedoAction() {
    if (redoTop >= 0) {
        return redoStack[redoTop--];
    }
    return (Action) { .type = -1 };
}

void addToHistory(Action action) {
    if (historyCount < MAX_ACTIONS) {
        actionHistory[historyCount++] = action;
    }
}

void undo_last_action() {
    if (undoTop >= 0) {
        Action action = popUndoAction();
        switch (action.type) {
        case ACTION_ADD:
            removeObject(action.objectIndex);
            break;
        case ACTION_REMOVE:
            addObjectToManager(action.previousState);
            break;
        case ACTION_TRANSFORM:
            objectManager.objects[action.objectIndex] = action.previousState;
            break;
        case ACTION_CHANGE_COLOR:
            objectManager.objects[action.objectIndex].color = action.previousState.color;
            break;
        default:
            break;
        }
        pushRedoAction(action);
    }
}

void redo_last_action() {
    if (redoTop >= 0) {
        Action action = popRedoAction();
        switch (action.type) {
        case ACTION_ADD:
            addObjectToManager(action.newState);
            break;
        case ACTION_REMOVE:
            removeObject(action.objectIndex);
            break;
        case ACTION_TRANSFORM:
            objectManager.objects[action.objectIndex] = action.newState;
            break;
        case ACTION_CHANGE_COLOR:
            objectManager.objects[action.objectIndex].color = action.newState.color;
            break;
        default:
            break;
        }
        pushUndoAction(action);
    }
}

void removeObjectWithAction(int index) {
    if (index < 0 || index >= objectManager.count) return;

    printf("Removing object at index: %d\n", index);

    Action action = {
        .type = ACTION_REMOVE,
        .previousState = objectManager.objects[index],
        .objectIndex = index
    };

    pushUndoAction(action);
    addToHistory(action);

    // AUDIO FEEDBACK
    #ifdef AUDIO_ENABLED
    playSound("resources/audio/object_delete.wav");
    #endif

    // Remove the object
    removeObject(index);

    // Adjust selected_object to a valid object if possible
    if (objectManager.count > 0) {
        if (index >= objectManager.count) {
            selected_object = &objectManager.objects[objectManager.count - 1];
        }
        else {
            selected_object = &objectManager.objects[index];
        }
    }
    else {
        selected_object = NULL;
    }

    if (selected_object) {
        printf("New selected object: ID=%d, Index=%ld\n", selected_object->id, selected_object - objectManager.objects);
    }
    else {
        printf("No selected object\n");
    }

    // Hide the relevant windows
    show_color_picker = false;
    show_inspector = false;
    show_change_texture = false;
    show_change_material = false;
}

void addObjectWithAction(ObjectType type, bool useTextures, int textureID, bool useColors, Model* model, PBRMaterial material, bool usePBR) {
    addObject(&camera, type, useTextures, textureID, useColors, model, material, usePBR);
    SceneObject* newObject = &objectManager.objects[objectManager.count - 1]; // Get the last added object
    Action action = {
        .type = ACTION_ADD,
        .newState = *newObject,
        .objectIndex = objectManager.count - 1
    };
    snprintf(action.description, sizeof(action.description), "Added object of type %d", type);
    pushUndoAction(action);
    addToHistory(action);
    
    // AUDIO FEEDBACK
    #ifdef AUDIO_ENABLED
    playSound("resources/audio/object_create.wav");
    #endif
}

void transformObjectWithAction(int index, Vector3 position, Vector3 rotation, Vector3 scale) {
    if (index < 0 || index >= objectManager.count) return;
    Action action = {
        .type = ACTION_TRANSFORM,
        .previousState = objectManager.objects[index],
        .objectIndex = index,
        .newState = objectManager.objects[index]
    };
    snprintf(action.description, sizeof(action.description), "Transformed object at index %d", index);
    action.newState.position = position;
    action.newState.rotation = rotation;
    action.newState.scale = scale;
    pushUndoAction(action);
    addToHistory(action);
    objectManager.objects[index].position = position;
    objectManager.objects[index].rotation = rotation;
    objectManager.objects[index].scale = scale;
}

void changeColorWithAction(int index, Vector4 color) {
    if (index < 0 || index >= objectManager.count) return;
    Action action = {
        .type = ACTION_CHANGE_COLOR,
        .previousState = objectManager.objects[index],
        .objectIndex = index,
        .newState = objectManager.objects[index]
    };
    snprintf(action.description, sizeof(action.description), "Changed color of object at index %d", index);
    action.newState.color = color;
    pushUndoAction(action);
    addToHistory(action);
    objectManager.objects[index].color = color;
}

void toggleOptionWithAction(const char* optionName, bool newValue) {
    Action action = {
        .type = ACTION_TOGGLE_OPTION,
        .objectIndex = -1
    };
    snprintf(action.description, sizeof(action.description), "Toggled option %s to %s", optionName, newValue ? "true" : "false");
    addToHistory(action);
}
