#pragma once

#include "aurora/Layer.h"
#include "aurora/Logger.h"

namespace aurora {

// Placeholder editor layer (will host ImGui + panels later)
class EditorLayer : public Layer {
public:
    EditorLayer() : Layer("EditorLayer") {}
    void onAttach() override { Logger::info(name().c_str(), "attached"); }
    void onDetach() override { Logger::info(name().c_str(), "detached"); }
    void onUpdate(float dt) override {
        (void)dt; // No-op until ImGui integration
    }
};

} // namespace aurora
