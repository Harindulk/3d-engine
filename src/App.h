#pragma once

#include <string>
#include <vector>

struct GLFWwindow;

class App {
public:
    App(int width, int height, const char* title);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void run();

private:
    void initWindow(int width, int height, const char* title);
    void initVulkan();
    void pickPhysicalDevice();
    void createInstance();
    void createSurface();
    void createDevice();
    void mainLoop();
    void cleanupVulkan();

private:
    GLFWwindow* window_ = nullptr;

    struct VkObjects;
    VkObjects* vk_ = nullptr;
};
