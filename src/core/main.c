#include "gui.h"
#include "rendering.h"
#include "globals.h"

// AUDIO SYSTEM INTEGRATION
#ifdef AUDIO_ENABLED
#include "audio.h"
#endif

int main(void) {
    #ifdef _WIN32
        #include <windows.h>
        ShowWindow(GetConsoleWindow(), SW_HIDE);  // Hide console only on Windows
    #endif

    printf("=== ClueEngine v1.1.0 Starting ===\n");
    
    setup();  // Set up OpenGL context, load shaders, and other resources

    initLoadingScreen(screen.window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear buffers to set initial background
    glfwSwapBuffers(screen.window);  // Display the initial cleared screen
    run_loading_screen(screen.window);  // Display and run the loading screen
    glfwSetKeyCallback(screen.window, key_callback);  // Set key callbacks for user input
    glfwSetFramebufferSizeCallback(screen.window, framebuffer_size_callback); // Handle window resizing

    printf("Engine initialization complete!\n");
    
    #ifdef AUDIO_ENABLED
    printf("Audio system: ENABLED\n");
    #else
    printf("Audio system: DISABLED\n");
    #endif

    while (!glfwWindowShouldClose(screen.window)) {
        glfwPollEvents();  // Handle GLFW events such as input and window actions

        generate_new_frame();

        if (isRunning) {
            update(calculateDeltaTime());  // Update game logic only if the simulation is running
        }

        handleMouseInput(screen.window, &camera);  // Manage mouse input for camera control
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear the screen each frame
        render();  // Render the scene, including the loaded model if any

        main_gui();  // Update the GUI elements
        render_nuklear();  // Render the GUI to the screen

        glfwSwapBuffers(screen.window);  // Swap the front and back buffers
    }

    printf("Shutting down ClueEngine...\n");
    teardown_nuklear();  // Clean up Nuklear GUI resources
    end();  // Clean up OpenGL and other resources
    printf("ClueEngine shutdown complete.\n");
    return 0;
}