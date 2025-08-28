#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace aurora {

class IGame;
struct EngineConfig {
    uint32_t width = 1280;
    uint32_t height = 720;
    std::string title = "Aurora";
    bool enableValidation = false; // future toggle
};

class Engine {
public:
    explicit Engine(const EngineConfig& cfg);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void run(IGame& game);

    // timing
    float getDeltaTime() const { return deltaTime_; }

private:
    void init(const EngineConfig& cfg);
    void shutdown();
    void mainLoop(IGame& game);

private:
    struct Impl; // PIMPL hides Vulkan/window details
    std::unique_ptr<Impl> impl_;
    float deltaTime_ = 0.f;
};

class IGame {
public:
    virtual ~IGame() = default;
    virtual void onInit(Engine&)=0;
    virtual void onUpdate(Engine&, float dt)=0;
    virtual void onShutdown(Engine&)=0;
};

} // namespace aurora
