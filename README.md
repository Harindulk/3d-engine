# Aurora3D Starter

Aurora3D is a **from-scratch 3D game engine experiment** built with **C++20**, **Vulkan**, and **GLFW**.  
Right now it opens a window, initializes Vulkan, and selects a GPU.  
The next steps are building a swapchain and rendering the first triangle.

---

## Features (so far)
- C++20 / CMake project structure
- GLFW window and input handling
- Vulkan instance, surface, and logical device creation
- GPU selection with console output
- Ready for swapchain + rendering setup

---

## Requirements
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (latest version, x64 recommended)
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.20+
- Git (for submodules like GLFW)

---

## Build Instructions (Windows x64)

```powershell
# Clone and init submodules
git clone https://github.com/yourname/3d-engine.git
cd 3d-engine
git submodule update --init --recursive

# Configure
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DVulkan_INCLUDE_DIR="$env:VULKAN_SDK\Include" `
  -DVulkan_LIBRARY="$env:VULKAN_SDK\Lib\vulkan-1.lib"

# Build
cmake --build build --config Debug

# Run
.\build\bin\Debug\aurora3d.exe
