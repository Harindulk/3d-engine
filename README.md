# Aurora3D

Aurora3D is a lightweight experimental C++20 Vulkan engine prototype. It focuses on a clean, incremental architecture you can extend into a reusable game engine.

## Current Status (2025-08-28)
Implemented:
- GLFW window + resize callback, swapchain recreation (resize / OUT_OF_DATE / SUBOPTIMAL handling).
- Vulkan instance (validation in Debug), surface, physical & logical device selection.
- Swapchain, image views, render pass, graphics pipeline (simple triangle), framebuffers.
- Command pool, command buffers, synchronization primitives (per-frame semaphores & fences).
- Basic rendering loop (triangle) with FPS counter in window title.
- Modular managers: `Instance`, `Device`, `SwapchainManager`, `Renderer`, shared `VkObjects` state.
- Engine layer draft: static library `aurora_engine` with `aurora::Engine` + `aurora::IGame` interface and sample `minimal_game`.
- CMake shader compilation (GLSL -> SPIR-V) using `glslangValidator` if available.

In progress / Planned next:
- Replace hardcoded triangle draw with Mesh (vertex/index buffer) abstraction.
- Depth buffer attachment & depth testing.
- Uniform buffer (MVP) + simple camera controls.
- Texture loading (stb_image) & descriptor sets.
- Asset / resource manager & logging levels.
- Optional Dear ImGui debug overlay.

Deferred (future milestones):
- ECS or lightweight scene graph & transform hierarchy.
- Hot-reload (shaders / assets) and async loading.
- Editor panels (scene hierarchy, inspector) after runtime stabilizes.
- Scripting (Lua/Python) plugin integration.

## Directory Layout
```
engine/            # Public engine API (include/aurora) + Engine implementation (temp bridge to App)
src/               # Legacy runtime modules (will be migrated behind Engine PIMPL)
  vulkan/          # Vulkan managers & helpers
  window/          # GLFW window wrapper
  shaders/         # GLSL sources (compiled to build/shaders/*.spv)
samples/MinimalGame# Sample using the engine library API
external/glfw      # GLFW (when building bundled)
```

## Build Requirements
- Vulkan SDK (x64) installed (`VULKAN_SDK` env var recommended)
- Visual Studio 2022 (C++ toolchain) or another CMake-supported compiler
- CMake 3.20+

## Configure, Build & Run (Windows PowerShell)
```powershell
# Clone + submodules
git clone https://github.com/Harindulk/3d-engine.git
cd 3d-engine
git submodule update --init --recursive

# Configure (Visual Studio generator example)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DVulkan_INCLUDE_DIR="$env:VULKAN_SDK\Include" `
  -DVulkan_LIBRARY="$env:VULKAN_SDK\Lib\vulkan-1.lib"

# Build engine + samples
cmake --build build --config Debug

# Run legacy executable
./build/bin/Debug/aurora3d.exe

# Run sample using Engine API
./build/bin/Debug/minimal_game.exe
```

## CMake Options
| Option | Default | Description |
|--------|---------|-------------|
| `AURORA_USE_EXTERNAL_GLFW` | ON | Use bundled GLFW in `external/glfw` |
| `AURORA_FORCE_VALIDATION`  | OFF | Force enable Vulkan validation regardless of build type |
| `AURORA_WARNINGS_AS_ERRORS`| OFF | Treat warnings as errors |
| `AURORA_VERBOSE_LOG`       | (unset) | Enable verbose per-frame logging (define manually) |

Enable validation in all builds:
```powershell
cmake -S . -B build -DAURORA_FORCE_VALIDATION=ON
```

## Shader Compilation
On configure, if `glslangValidator` is found (Vulkan SDK), `triangle.vert` / `triangle.frag` are compiled to SPIR-V under `build/shaders/`. Missing tool: engine expects prebuilt `.spv` files there.

## Engine API (Early Draft)
```cpp
#include <aurora/Engine.h>
class MyGame : public aurora::IGame {
  void onInit(aurora::Engine&) override {}
  void onUpdate(aurora::Engine&, float dt) override { /* game logic */ }
  void onShutdown(aurora::Engine&) override {}
};
int main(){ aurora::EngineConfig cfg; cfg.title = "My Game"; aurora::Engine e(cfg); MyGame g; e.run(g); }
```

## Troubleshooting
- Black screen: ensure SPIR-V shaders exist in `build/shaders/` (reconfigure with Vulkan SDK installed).
- Crash on resize: report if persists—swapchain / framebuffer recreation order recently updated.
- Validation errors: run Debug build or force enable validation to catch misuse early.


