# Aurora3D

Aurora3D is a small experimental 3D engine prototype written in C++20 using Vulkan and GLFW.

Current status (2025-08-27):
- Windowing via GLFW and a small `Window` wrapper.
- Vulkan instance and optional validation/debug messenger.
- Physical device selection and logical device creation.
- Swapchain, image views, render pass, graphics pipeline, command buffers, and a simple triangle renderer.
- FPS counter shown in the window title (updated once per second).
- Project is partially modularized: `vulkan/VkObjects`, `vulkan/Utils`, `vulkan/Instance`, `window/Window`.

Known limitations / TODOs:
- Swapchain recreation on resize / VK_ERROR_OUT_OF_DATE_KHR handling still needs implementation.
- Further modularization planned: extract DeviceManager and SwapchainManager.
- Add input handling, scene graph, resource management, and more robust error handling.

Requirements
- Vulkan SDK (x64)
- Visual Studio 2022 or newer (Desktop development with C++)
- CMake 3.20+

Quick build & run (Windows x64, PowerShell)

```powershell
# Clone and init submodules
git clone https://github.com/Harindulk/3d-engine.git
cd 3d-engine
git submodule update --init --recursive

# Configure (adjust generator/paths as needed)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DVulkan_INCLUDE_DIR="$env:VULKAN_SDK\Include" `
  -DVulkan_LIBRARY="$env:VULKAN_SDK\Lib\vulkan-1.lib"

# Build
cmake --build build --config Debug

# Run
.\build\bin\Debug\aurora3d.exe
```

Notes
- The CMake script will compile the GLSL shaders (found in `src/shaders`) into SPIR-V using `glslangValidator` if it's available in your PATH or the Vulkan SDK.
- Validation layers are enabled automatically in Debug builds; you can force them on via the CMake option `-DAURORA_FORCE_VALIDATION=ON`.

If you'd like, I can:
- Implement swapchain recreation and a Resize callback.
- Continue splitting `App` into Device/Swapchain/Renderer modules.
- Add a minimal render loop UI or frame timing utilities.
