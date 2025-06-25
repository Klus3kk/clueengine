# ClueEngine

**ClueEngine** is my self-made 3D graphics engine developed using **C** with **OpenGL** rendering, **PBR materials**, lighting, object management, and GUI.
## About

ClueEngine is built to provide tools necessary to easily manage 3D scenes, manipulate objects, and handle real-time rendering. It offers **camera movement**, **texture management**, and an **user interface** for designing and interacting with 3D environment.

## Key Features

- **Real-Time Rendering**: Powered by **OpenGL**, supporting 3D object rendering with lighting and shading.
- **PBR Materials**: The engine includes support for **Physically-Based Rendering** materials.
- **Camera Control**: For smooth navigation and user interaction.
- **GUI Integration**: A built-in **Nuklear GUI** for managing settings, controls, and object interactions.
- **Lighting and shadows**: Light sources including **Point Light**, **Directional Light**, and **Spotlight**.

## Quick Start

### **Clone the repository**

```bash
git clone https://github.com/Klus3kk/ClueEngine.git
cd ClueEngine
```

### **Install dependencies**

Refer to the [installation guide](docs/setup.md) for detailed steps.

### **Build the project using CMake**

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

### **Run the engine**

- **Linux/macOS**
```bash
./bin/ClueEngine
```

- **Windows**
```bash
./bin/ClueEngine.exe
```

### **Modify or create your own scene**:
Use the **GUI** controls to add objects, adjust materials, and set up lights.

## Docker Deployment

To run **ClueEngine** in a **Docker container**, follow these steps:

### **Build the Docker Image**

```bash
docker build -t clueengine .
```

### **Run the Docker Container**

```bash
docker run --rm -it --net=host --env DISPLAY=$DISPLAY \
    --device /dev/dri \
    --device /dev/snd \
    --group-add video \
    --group-add audio \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    clueengine
```

## Documentation Folder (docs)

The **docs** folder contains detailed explanations of specific aspects of **ClueEngine**:

1. [setup.md](docs/setup.md): A comprehensive guide on how to set up the engine and all necessary dependencies.
2. [enginearchitecture.md](docs/enginearchitecture.md): An in-depth look at the engine's architecture, how the different modules interact, and the underlying design principles.
3. [renderingpipeline.md](docs/renderingpipeline.md): Detailed documentation on how the rendering pipeline works, including shaders, materials, and texture loading.
4. [userguide.md](docs/userguide.md): A practical guide to using the engine, including how to create scenes, manipulate objects, and use the GUI.
5. [extendingtheengine.md](docs/extendingtheengine.md): Future plans for adding **new features, shaders, and customizations**.
