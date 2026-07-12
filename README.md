# Heron Engine

Heron Engine is a standalone, lightweight 3D game engine and editor built from scratch in C++. It is designed to be highly modular and independent, with a core focus on performance, ease of use, and native C++ game logic integration.

## Architecture and Core Systems

The engine is built around a standard Entry Point, Application, and Layer Stack architecture, providing a clean separation between the engine core and the client application.

### Graphics and Rendering
*   **OpenGL 3.3 Backend:** The rendering system uses a custom abstraction layer over OpenGL (via GLAD and GLFW).
*   **ImGui Integration:** The Editor interface is built using Dear ImGui, rendering directly to an OpenGL Framebuffer (the Viewport) rather than the main application window, enabling a modern docked editor layout.
*   **Renderer API:** Encapsulates Vertex Arrays, Vertex Buffers, Index Buffers, and Shader compilation.

### Scene and Entity Component System (ECS)
*   **EnTT Integration:** Heron Engine uses the `entt` library for a robust, high-performance Entity Component System.
*   **Standard Components:** Built-in support for `TagComponent`, `TransformComponent`, `MeshComponent`, and Physics/Scripting components.

### Native C++ Scripting
*   **ScriptableEntity:** A pure C++ scripting system. Game logic is written by subclassing `Heron::ScriptableEntity` with virtual `OnCreate`, `OnUpdate`, and `OnDestroy` lifecycle methods.
*   **ScriptRegistry:** A macro-driven factory registry (`HERON_REGISTER_SCRIPT`) that automatically exposes compiled C++ scripts to the Editor UI without requiring external dependencies (like Lua or Mono).
*   **Editor Hookup:** Scripts can be attached directly to entities via the Properties panel and are instantiated automatically during the runtime simulation loop.

### Physics Simulation
*   **Box3D Integration:** The engine utilizes Erin Catto's Box3D for rigorous rigid body dynamics and collision detection.
*   **Components:** Support for `RigidBody3DComponent` (Static, Kinematic, Dynamic) and `BoxCollider3DComponent`.
*   **Script Sync:** The runtime loop automatically synchronizes `TransformComponent` modifications made by Native Scripts into the Box3D physics engine before stepping the simulation, allowing scripts to seamlessly interact with physics bodies.

### Project Management and Serialization
*   **YAML-CPP:** Scene serialization is handled via `yaml-cpp`.
*   **Scene Saving/Loading:** The entire state of a scene (Entities, Components, Physics data, and attached Scripts) can be saved to `.heron` files.
*   **Automated Project Workflow:** Creating or opening a project automatically points the Content Browser to the project directory and manages a persistent `essentials/main.heron` file, ensuring seamless editor startup.

## Dependencies

Heron Engine manages its dependencies primarily through Git Submodules and CMake `FetchContent`, ensuring a self-contained build environment.

*   [GLFW](https://www.glfw.org/) - Windowing and input
*   [GLAD](https://glad.dav1d.de/) - OpenGL loading
*   [GLM](https://github.com/g-truc/glm) - Mathematics
*   [ImGui](https://github.com/ocornut/imgui) - Editor UI
*   [EnTT](https://github.com/skypjack/entt) - Entity Component System
*   [Box3D](https://box2d.org/) - 3D Physics Simulation
*   [spdlog](https://github.com/gabime/spdlog) - Logging
*   [yaml-cpp](https://github.com/jbeder/yaml-cpp) - Serialization

## Building the Engine

Heron Engine uses CMake as its primary build system. It is currently configured and tested on Windows using MSVC.

1. Clone the repository.
2. Run CMake to configure the build (e.g., `cmake -S . -B build`).
3. Build the solution (e.g., `cmake --build build --config Debug`).
4. Run `HeronEditor.exe` to launch the engine.
