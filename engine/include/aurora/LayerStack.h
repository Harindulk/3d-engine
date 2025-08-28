#pragma once

#include <vector>
#include <memory>
#include "aurora/Layer.h"

namespace aurora {

// Manages ordered collection of layers + overlays (overlays rendered last)
class LayerStack {
public:
    LayerStack() = default;
    ~LayerStack();

    // Regular layers inserted before first overlay
    void pushLayer(std::unique_ptr<Layer> layer);
    void pushOverlay(std::unique_ptr<Layer> overlay);
    void popLayer(Layer* layer);
    void popOverlay(Layer* overlay);

    std::vector<std::unique_ptr<Layer>>& layers() { return layers_; }
    const std::vector<std::unique_ptr<Layer>>& layers() const { return layers_; }

private:
    std::vector<std::unique_ptr<Layer>> layers_;
    size_t firstOverlayIndex_ = 0; // index where overlays start
};

} // namespace aurora
