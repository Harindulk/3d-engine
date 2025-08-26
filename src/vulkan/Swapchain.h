#pragma once

#include "vulkan/VkObjects.h"
#include <GLFW/glfw3.h>

namespace vulkan {
struct SwapchainManager {
    static void createSwapchain(VkObjects* vk, GLFWwindow* window);
    static void createImageViews(VkObjects* vk);
    static void createFramebuffers(VkObjects* vk);
    static void cleanupSwapchain(VkObjects* vk);
    static void recreateSwapchain(VkObjects* vk, GLFWwindow* window);
};
}
