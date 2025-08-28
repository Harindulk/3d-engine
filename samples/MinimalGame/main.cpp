#include <aurora/Engine.h>
#include <iostream>

class MinimalGame : public aurora::IGame {
public:
    void onInit(aurora::Engine& engine) override {
        std::cout << "MinimalGame::onInit\n"; (void)engine;
    }
    void onUpdate(aurora::Engine& engine, float dt) override {
        (void)engine; (void)dt; // will add logic later
    }
    void onShutdown(aurora::Engine& engine) override {
        std::cout << "MinimalGame::onShutdown\n"; (void)engine;
    }
};

int main() {
    aurora::EngineConfig cfg; cfg.title = "Aurora Minimal Game"; cfg.width=1280; cfg.height=720;
    aurora::Engine engine(cfg);
    MinimalGame game;
    engine.run(game);
    return 0;
}
