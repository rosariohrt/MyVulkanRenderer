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

Dependency-install scripts live in `scripts/`. They install the build tools,
GLFW, and GLM, then print instructions for installing the **Vulkan SDK**
(which must be downloaded manually from <https://vulkan.lunarg.com/>).

### Linux

```sh
./scripts/install_dependencies_linux.sh   # apt / dnf / pacman auto-detected
mkdir build && cd build
cmake ..
make
./MyVulkanRenderer
```

### macOS

```sh
./scripts/install_dependencies_macos.sh   # requires Homebrew
mkdir build && cd build
cmake ..
make
./MyVulkanRenderer
```

### Windows (Visual Studio — recommended)

Visual Studio 2022/2026 (with the "Desktop development with C++" workload, which
includes CMake) opens the project directly:

1. Install the [Vulkan SDK](https://vulkan.lunarg.com/) and
   [vcpkg](https://github.com/microsoft/vcpkg), then make vcpkg's packages
   visible to Visual Studio:

   ```powershell
   .\scripts\install_dependencies_windows.bat   # installs glfw3, glm via vcpkg
   vcpkg integrate install                       # let VS auto-discover them
   ```

2. In Visual Studio: **File → Open → Folder…** and select the repository root.
   VS detects `CMakeLists.txt` and configures automatically.
3. Pick `MyVulkanRenderer.exe` as the startup item and press **F5** (or Ctrl+F5).

### Windows (command line)

```powershell
.\scripts\install_dependencies_windows.bat
# Point the toolchain at the vcpkg the script installed into
# (the vcpkg on your PATH, e.g. C:\vcpkg)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Debug
.\build\Debug\MyVulkanRenderer.exe
```


## Credit

- [Vulkan-Tutorial](https://github.com/KhronosGroup/Vulkan-Tutorial)
- [littleVulkanEngine](https://github.com/blurrypiano/littleVulkanEngine)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
- [Learn OpenGL](https://learnopengl.com/)
- [stb](https://github.com/nothings/stb)