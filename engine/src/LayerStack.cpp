#include "aurora/LayerStack.h"
#include <algorithm>

namespace aurora {

LayerStack::~LayerStack() {
    // Detach in reverse order
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        if (it->get()) (*it)->onDetach();
    }
}

void LayerStack::pushLayer(std::unique_ptr<Layer> layer) {
    if (!layer) return;
    layers_.insert(layers_.begin() + static_cast<long>(firstOverlayIndex_), std::move(layer));
    ++firstOverlayIndex_;
    layers_[firstOverlayIndex_-1]->onAttach();
}

void LayerStack::pushOverlay(std::unique_ptr<Layer> overlay) {
    if (!overlay) return;
    layers_.push_back(std::move(overlay));
    layers_.back()->onAttach();
}

void LayerStack::popLayer(Layer* layer) {
    if (!layer) return;
    auto it = std::find_if(layers_.begin(), layers_.begin() + static_cast<long>(firstOverlayIndex_), [&](const std::unique_ptr<Layer>& p){ return p.get()==layer; });
    if (it != layers_.begin() + static_cast<long>(firstOverlayIndex_)) {
        (*it)->onDetach();
        layers_.erase(it);
        --firstOverlayIndex_;
    }
}

void LayerStack::popOverlay(Layer* overlay) {
    if (!overlay) return;
    auto it = std::find_if(layers_.begin() + static_cast<long>(firstOverlayIndex_), layers_.end(), [&](const std::unique_ptr<Layer>& p){ return p.get()==overlay; });
    if (it != layers_.end()) {
        (*it)->onDetach();
        layers_.erase(it);
    }
}

} // namespace aurora
