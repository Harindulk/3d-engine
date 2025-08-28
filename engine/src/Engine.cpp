#include "aurora/Engine.h"
#include <stdexcept>
#include <chrono>
#include <iostream>

// Reuse existing App internals for now (will migrate later)
#include "App.h" // temporary reuse; will be removed once Vulkan moved behind PIMPL

namespace aurora {

struct Engine::Impl {
    App* app = nullptr; // temp bridge
};

Engine::Engine(const EngineConfig& cfg) : impl_(new Impl()) {
    // Map to existing App for now
    impl_->app = new App(static_cast<int>(cfg.width), static_cast<int>(cfg.height), cfg.title.c_str());
}

Engine::~Engine() {
    if (impl_->app) {
        delete impl_->app;
        impl_->app = nullptr;
    }
}

void Engine::run(IGame& game) {
    game.onInit(*this);
    using clock = std::chrono::high_resolution_clock;
    auto prev = clock::now();
    // Manual frame loop
    while (true) {
        auto now = clock::now();
        std::chrono::duration<float> dt = now - prev;
        prev = now;
        deltaTime_ = dt.count();
        try {
            // Advance one engine frame (Vulkan + window). Break if window closed.
            if (!impl_->app->frame()) break;
            game.onUpdate(*this, deltaTime_);
        } catch (const std::exception& e) {
            std::cerr << "Engine loop exception: " << e.what() << std::endl;
            break;
        } catch (...) {
            std::cerr << "Engine loop unknown exception" << std::endl;
            break;
        }
    }
    game.onShutdown(*this);
}

void Engine::init(const EngineConfig&) {}
void Engine::shutdown() {}
void Engine::mainLoop(IGame&) {}

} // namespace aurora
