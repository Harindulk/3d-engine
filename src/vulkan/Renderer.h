#pragma once

#include "vulkan/VkObjects.h"

// Forward declare GLFW types to avoid including GLFW headers in this header
struct GLFWwindow;

namespace vulkan {
struct Renderer {
    static void createRenderPass(VkObjects* vk);
    static void createGraphicsPipeline(VkObjects* vk);
    static void createCommandPool(VkObjects* vk);
    static void createCommandBuffers(VkObjects* vk);
    // New: per-frame recording; existing commandBuffers are allocated only
    static void recordCommandBuffer(VkObjects* vk, VkCommandBuffer cmd, uint32_t imageIndex);
    static void createSyncObjects(VkObjects* vk);
    static void cleanupRenderer(VkObjects* vk);
    static void drawFrame(VkObjects* vk, GLFWwindow* window);
    static void recreate(VkObjects* vk, GLFWwindow* window);
};
}
