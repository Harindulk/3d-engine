#pragma once

#include <string>
#include <vector>

struct VkObjects;
class Window;

class App {
public:
    App(int width, int height, const char* title);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void run();
    // New: perform a single frame (returns false when window requests close)
    bool frame();
    void resetFrameStats();

private:
    void initWindow(int width, int height, const char* title);
    void initVulkan();
    void createSurface();
    
    
    // Swapchain handled by vulkan::SwapchainManager

    // Rendering helpers
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void mainLoop();
    void cleanupVulkan();
    void recreateResources();

private:
    Window* window_ = nullptr;

    VkObjects* vk_ = nullptr;
    // Simple FPS counter (updated in mainLoop)
    size_t frameCount_ = 0;
    double lastFPSTime_ = 0.0;
    int fps_ = 0;
};
