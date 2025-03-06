# ClueEngine

**ClueEngine** is a cutting-edge 3D graphics engine developed using **C**, focusing on simplicity and extensibility for 3D rendering applications. With real-time **OpenGL** rendering, dynamic **PBR materials**, lighting, object management, and a powerful GUI, ClueEngine is ideal for creating 3D games and simulations. The engine is designed to be modular, with customizable features for efficient development and ease of integration.

## About


ClueEngine is built to provide developers with the tools necessary to easily manage 3D scenes, manipulate objects, and handle real-time rendering. It offers seamless **camera movement**, **texture management**, and an intuitive **user interface** for designing and interacting with 3D worlds.

This engine can be used for creating interactive applications, games, or simulations that require rich 3D rendering.

## Key Features

- **Real-Time Rendering**: Powered by **OpenGL**, supporting 3D object rendering with detailed lighting and shading.
- **PBR Materials**: The engine includes support for **Physically-Based Rendering** materials, ensuring realistic textures and effects.
- **Camera Control**: Fully functional **camera controls** for smooth navigation and user interaction.
- **GUI Integration**: A built-in **Nuklear GUI** for managing settings, controls, and object interactions.
- **Lighting and Shadows**: Advanced light sources including **Point Light**, **Directional Light**, and **Spotlight**.

## Quick Start

### 1. **Clone the repository**

```bash
git clone https://github.com/Klus3kk/ClueEngine.git
cd ClueEngine
```

### 2. **Install dependencies**

Refer to the [installation guide](docs/setup.md) for detailed steps.

### 3. **Build the project using CMake**

```bash
cmake . -Bbuild
cmake --build build
```

Or, for faster building:

```bash
cmake -B build -G Ninja
ninja -C build
```

This will compile the engine and generate the **ClueEngine** executable in the `bin` directory.

### 4. **Run the engine**

- **Linux/macOS**
```bash
./bin/ClueEngine
```

- **Windows**
```bash
./bin/ClueEngine.exe
```

### 5. **Modify or create your own scene**:
Use the **GUI** controls to add objects, adjust materials, and set up lights.

## Dependencies

- **GLFW**: For window management and input handling.
- **GLAD**: For OpenGL function loading.
- **Nuklear**: For the graphical user interface (GUI).
- **SOIL2**: For loading textures.

## Docker Deployment

To run **ClueEngine** in a **Docker container**, follow these steps:

### 1. **Install Docker**
Follow the [official Docker installation guide](https://docs.docker.com/get-docker/).

### 2. **Build the Docker Image**

```bash
docker build -t clueengine .
```

### 3. **Run the Docker Container**

```bash
docker run --rm -it --net=host --env DISPLAY=$DISPLAY \
    --device /dev/dri \
    --device /dev/snd \
    --group-add video \
    --group-add audio \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    clueengine
```

## How the Engine Works

**ClueEngine** is based on several core modules that interact seamlessly to render objects and handle input. The main components include:

### Object Management

- Objects are created using various shapes (cube, sphere, pyramid, etc.).
- Objects can be modified by changing their **position**, **rotation**, and **scale** dynamically.
- Support for creating models and applying **PBR materials**.

### Camera and Controls

- **First-Person Camera**: Navigate the 3D space using the **WASD** keys and **mouse**.
- **Camera Zooming and Panning**: Using the mouse and keyboard, zoom in/out and pan across the scene.
  
### Rendering Pipeline

- **Shaders**: Vertex and fragment shaders are used for rendering objects with different material properties.
- **Textures and Materials**: The engine supports both **basic textures** and **PBR materials**, allowing for detailed and realistic object surfaces.

### User Interface

- **Nuklear** is used to render the engine's GUI, allowing easy control over object properties, scene settings, and debug information.

## Documentation Folder (docs)

The **docs** folder contains detailed explanations of specific aspects of **ClueEngine**:

1. [setup.md](docs/setup.md): A comprehensive guide on how to set up the engine and all necessary dependencies.
2. [enginearchitecture.md](docs/enginearchitecture.md): An in-depth look at the engine's architecture, how the different modules interact, and the underlying design principles.
3. [renderingpipeline.md](docs/renderingpipeline.md): Detailed documentation on how the rendering pipeline works, including shaders, materials, and texture loading.
4. [userguide.md](docs/userguide.md): A practical guide to using the engine, including how to create scenes, manipulate objects, and use the GUI.
5. [extendingtheengine.md](docs/extendingtheengine.md): Future plans for adding **new features, shaders, and customizations**.

## License

ClueEngine is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
