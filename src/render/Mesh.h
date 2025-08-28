#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

struct VkObjects; // forward

struct Vertex {
    float pos[2];
    float color[3];
};

namespace render {
class Mesh {
public:
    Mesh() = default;
    ~Mesh() = default; // GPU resources managed externally for now

    const std::vector<Vertex>& vertices() const { return vertices_; }
    const std::vector<uint32_t>& indices() const { return indices_; }

    static Mesh makeTriangle();
private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
};
}
