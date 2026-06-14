# MyVulkanRenderer

A small real-time renderer built with **Vulkan** and **GLFW**.
> This project is a work in progress - see the other branches for the latest code.

## Features

- RAII-based resource management (Vulkan-Hpp `vk::raii`)
- Dynamic rendering
- Manual GPU memory management (no VMA)
- Camera controls (WASD + mouse)

## Future Work

Goal: switch between rasterization and ray tracing at runtime.

- Model loading (glTF)
- GUI (Dear ImGui)
- VMA for memory management
- Ray tracing
- Other funny stuff

## Controls

| Input | Action |
|-------|--------|
| `W` / `A` / `S` / `D` | Move |
| Left mouse drag | Look around |
| Scroll | Zoom |
| `Shift` + Scroll | Move vertically |
| `Esc` | Quit |

## Requirements

- A GPU/driver supporting Vulkan 1.3 (dynamic rendering + synchronization2)
- [Vulkan SDK](https://vulkan.lunarg.com/) (provides headers, loader, and `glslc`)
- CMake 3.16+
- A C++20 compiler
- GLFW3, GLM

`glslc` must be on `PATH` or reachable via the `VULKAN_SDK` environment variable.

## How to Run



`scripts/` constains dependency-install scripts.\
They install depedencies and print instructions for installing the **Vulkan SDK**
(which must be downloaded manually from <https://vulkan.lunarg.com/>).

### Linux and macOS

```sh
./scripts/install_dependencies_linux.sh        # Make sure to install Vulkan SDK manually
mkdir build && cd build
cmake .. && make
./MyVulkanRenderer
```

### Windows (Visual Studio — recommended)

Prerequisites: install the [vcpkg](https://github.com/microsoft/vcpkg) first.

1. Install dependencies from the repository root:

```powershell
.\scripts\install_dependencies_windows.bat     # Make sure to install Vulkan SDK manually
vcpkg integrate install                        # let VS auto-discover them
```

2. Open the repository root in Visual Studio 2022/2026 (File → Open → Folder).
   VS detects `CMakeLists.txt` and configures automatically.
3. Pick `MyVulkanRenderer.exe` as the startup item and press F5 (or Ctrl+F5).

### Windows (command line)

WIP — to be added.


## Credit

- [Vulkan-Tutorial](https://github.com/KhronosGroup/Vulkan-Tutorial)
- [littleVulkanEngine](https://github.com/blurrypiano/littleVulkanEngine)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
- [Learn OpenGL](https://learnopengl.com/)
- [stb](https://github.com/nothings/stb)