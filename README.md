# ClueEngine

**ClueEngine** is my self-made 3D graphics engine developed using **C** with **OpenGL** rendering, **PBR materials**, lighting, object management and GUI.

## About

ClueEngine was built to provide tools necessary to easily manage 3D scenes, manipulate objects, and handle real-time rendering. It offers **camera movement**, **texture management** and an **user interface** for designing and interacting with 3D environment.

## Key Features

- **Real-time rendering**: Powered by **OpenGL**, supporting 3D object rendering with lighting and shading.
- **PBR materials**: The engine includes support for **physically-based rendering** materials.
- **Camera control**: For smooth navigation and user interaction.
- **GUI integration**: A built-in **Nuklear GUI** for managing settings, controls, and object interactions.
- **Lighting and shadows**: Light sources including **point light**, **directional light**, and **spotlight**.

## Quick Start

### Clone the repository

```bash
git clone https://github.com/Klus3kk/ClueEngine.git
cd ClueEngine
```

### Install dependencies

Refer to the [installation guide](docs/setup.md) for detailed steps.

### Build the project 

```bash
cmake -B build -G Ninja
ninja -C build
```

### Run the engine

- **Linux/macOS**

```bash
./bin/ClueEngine
```

- **Windows**

```bash
./bin/ClueEngine.exe
```

### Modify or create your own scene:
Use the **GUI** controls to add objects, import models, adjust materials, and set up lights.

## Docker Deployment

### Build the Docker Image

```bash
docker build -t clueengine .
```

### Run the Docker Container

```bash
docker run --rm -it --net=host --env DISPLAY=$DISPLAY \
    --device /dev/dri \
    --device /dev/snd \
    --group-add video \
    --group-add audio \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    clueengine
```

## Docs

The **docs** folder contains explanations of specific aspects of the engine:

1. [setup.md](docs/setup.md): a guide on how to set up the engine.
2. [enginearchitecture.md](docs/enginearchitecture.md): an in-depth look at the engine's architecture, how the different modules interact and the underlying design principles.
3. [renderingpipeline.md](docs/renderingpipeline.md): how the rendering pipeline works, including shaders, materials and texture loading.
4. [userguide.md](docs/userguide.md): guide to using the engine, including how to create scenes, manipulate objects and use the GUI.
5. [extendingtheengine.md](docs/extendingtheengine.md): future plans for my engine.
