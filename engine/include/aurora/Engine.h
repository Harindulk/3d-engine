#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

#include "aurora/LayerStack.h"

namespace aurora {

class IGame;
struct EngineConfig {
    uint32_t width = 1280;
    uint32_t height = 720;
    std::string title = "Aurora";
    bool enableValidation = false; // runtime intent (compile-time still controlled by AURORA_ENABLE_VALIDATION)
    bool vsync = true;             // choose FIFO vs mailbox when available
    uint32_t maxFramesInFlight = 2; // synchronization frame count
    bool preferDiscreteGpu = true; // device selection preference
};

class Engine {
public:
    explicit Engine(const EngineConfig& cfg);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void run(IGame& game);

    // Layer management (editor & runtime systems)
    void pushLayer(std::unique_ptr<Layer> layer);
    void pushOverlay(std::unique_ptr<Layer> overlay);

    // Convenience: enable editor (adds EditorLayer once)
    void enableEditorMode();

    // timing
    float getDeltaTime() const { return deltaTime_; }

    // Scene access (very early minimal ECS)
    struct Entity { uint32_t id; };
    struct Transform { float x=0,y=0,z=0; float rotZ=0; };
    Entity createEntity();
    Transform* getTransform(Entity e);

private:
    void init(const EngineConfig& cfg);
    void shutdown();
    void mainLoop(IGame& game);

private:
    struct Impl; // PIMPL hides Vulkan/window details
    std::unique_ptr<Impl> impl_;
    float deltaTime_ = 0.f;
    LayerStack layerStack_;
    bool editorEnabled_ = false;

    // Minimal scene storage
    std::vector<Transform> transforms_;
    std::vector<Entity> entities_;
};

class IGame {
public:
    virtual ~IGame() = default;
    virtual void onInit(Engine&)=0;
    virtual void onUpdate(Engine&, float dt)=0;
    virtual void onShutdown(Engine&)=0;
};

} // namespace aurora
