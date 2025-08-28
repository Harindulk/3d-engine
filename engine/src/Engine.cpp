#include "aurora/Engine.h"
#include <stdexcept>
#include <chrono>
#include <iostream>

// Reuse existing App internals for now (will migrate later)
#include "App.h" // temporary reuse; will be removed once Vulkan moved behind PIMPL
#include "aurora/Logger.h"
#include "aurora/EditorLayer.h"
#include "vulkan/VkObjects.h"
#include "App.h"
#include <cstring>

namespace aurora {

struct Engine::Impl {
    App* app = nullptr; // temp bridge
};

Engine::Engine(const EngineConfig& cfg) : impl_(new Impl()) {
    // Map to existing App for now
    impl_->app = new App(static_cast<int>(cfg.width), static_cast<int>(cfg.height), cfg.title.c_str());
    // Reserve some space for entities to avoid realloc early
    transforms_.reserve(64);
    entities_.reserve(64);
}

Engine::~Engine() {
    if (impl_->app) {
        delete impl_->app;
        impl_->app = nullptr;
    }
}

void Engine::run(IGame& game) {
    game.onInit(*this);
    // Create a demo entity with rotation for MVP demonstration
    Entity demo = createEntity();
    if (auto* t = getTransform(demo)) { t->x = 0.f; t->y = 0.f; t->z = 0.f; }

    // Setup uniform updater callback to build a simple rotating MVP
    if (impl_->app && impl_->app->getVkObjects()) {
        VkObjects* vk = impl_->app->getVkObjects();
        // Capture this engine by pointer
        vk->uniformUpdater = [this, vk](uint32_t /*imageIndex*/, void* mapped){
            // Basic perspective * rotation * translation (column-major like GL style expected by shader)
            float aspect = vk->swapchainExtent.height ? (float)vk->swapchainExtent.width / (float)vk->swapchainExtent.height : 1.f;
            // Perspective matrix
            float fov = 60.0f * 3.1415926f / 180.f;
            float f = 1.0f / std::tan(fov/2.f);
            float zNear = 0.1f, zFar = 100.f;
            float nf = 1.f / (zNear - zFar);
            float P[16] = { f/aspect,0,0,0, 0,f,0,0, 0,0,(zFar+zNear)*nf,-1, 0,0,(2*zFar*zNear)*nf,0 };
            // Model rotation around Z using first entity's rotZ
            float rot = 0.f;
            if (!transforms_.empty()) rot = transforms_[0].rotZ;
            float c = std::cos(rot), s = std::sin(rot);
            float M[16] = { c,s,0,0, -s,c,0,0, 0,0,1,0, 0,0,-2,1 }; // translate back -2 on Z
            // MVP = P * M (since V is identity looking down -Z)
            float MVP[16];
            for(int col=0; col<4; ++col){
                for(int row=0; row<4; ++row){
                    MVP[col*4+row] = P[0*4+row]*M[col*4+0] + P[1*4+row]*M[col*4+1] + P[2*4+row]*M[col*4+2] + P[3*4+row]*M[col*4+3];
                }
            }
            std::memcpy(mapped, MVP, sizeof(MVP));
        };
    }
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
            // Update layers (editor overlays last automatically via ordering)
            for (auto &layerPtr : layerStack_.layers()) {
                if (layerPtr) layerPtr->onUpdate(deltaTime_);
            }
            // Spin demo entity
            if (!transforms_.empty()) {
                transforms_[0].rotZ += deltaTime_ * 0.5f; // slow rotation
            }
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

void Engine::pushLayer(std::unique_ptr<Layer> layer) {
    if (!layer) return;
    layerStack_.pushLayer(std::move(layer));
}

void Engine::pushOverlay(std::unique_ptr<Layer> overlay) {
    if (!overlay) return;
    layerStack_.pushOverlay(std::move(overlay));
}

void Engine::enableEditorMode() {
    if (editorEnabled_) return;
    editorEnabled_ = true;
    pushOverlay(std::make_unique<EditorLayer>());
    Logger::info("Engine", "Editor mode enabled");
}

Engine::Entity Engine::createEntity() {
    Entity e{ static_cast<uint32_t>(entities_.size()) };
    entities_.push_back(e);
    transforms_.push_back({});
    return e;
}

Engine::Transform* Engine::getTransform(Entity e) {
    if (e.id < transforms_.size()) return &transforms_[e.id];
    return nullptr;
}

} // namespace aurora
