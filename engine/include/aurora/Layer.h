#pragma once

#include <string>

namespace aurora {

// Basic engine layer interface (runtime/editor systems derive from this)
class Layer {
public:
    explicit Layer(std::string name = "Layer") : name_(std::move(name)) {}
    virtual ~Layer() = default;

    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(float /*dt*/) {}

    const std::string& name() const { return name_; }

protected:
    std::string name_;
};

} // namespace aurora
