#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <optional>
#include <cstring>
#include <vector>

struct App::VkObjects {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
};

App::App(int width, int height, const char* title) {
    initWindow(width, height, title);
    initVulkan();
}

App::~App() {
    cleanupVulkan();
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

void App::initWindow(int width, int height, const char* title) {
    if (!glfwInit()) throw std::runtime_error("GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) throw std::runtime_error("GLFW window creation failed");
}

void App::initVulkan() {
    vk_ = new VkObjects();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createDevice();
}

void App::createInstance() {
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Aurora3D";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Aurora3D";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &appInfo;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&ci, nullptr, &vk_->instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

void App::createSurface() {
    if (glfwCreateWindowSurface(vk_->instance, window_, nullptr, &vk_->surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
}

void App::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk_->instance, &deviceCount, nullptr);
    if (deviceCount == 0) throw std::runtime_error("No Vulkan physical devices found");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk_->instance, &deviceCount, devices.data());

    for (auto dev : devices) {
        uint32_t qCount = 0; 
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> qProps(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, qProps.data());

        for (uint32_t i = 0; i < qCount; ++i) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, vk_->surface, &present);
            if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                vk_->physicalDevice = dev;
                vk_->graphicsQueueFamily = i;
                break;
            }
        }
        if (vk_->physicalDevice) break;
    }

    if (vk_->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(vk_->physicalDevice, &props);
    std::cout << "Selected GPU: " << props.deviceName << std::endl;
}

void App::createDevice() {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = vk_->graphicsQueueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExts;

    if (vkCreateDevice(vk_->physicalDevice, &dci, nullptr, &vk_->device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
    vkGetDeviceQueue(vk_->device, vk_->graphicsQueueFamily, 0, &vk_->graphicsQueue);
}

void App::cleanupVulkan() {
    if (!vk_) return;
    if (vk_->device) {
        vkDeviceWaitIdle(vk_->device);
        vkDestroyDevice(vk_->device, nullptr);
    }
    if (vk_->surface) vkDestroySurfaceKHR(vk_->instance, vk_->surface, nullptr);
    if (vk_->instance) vkDestroyInstance(vk_->instance, nullptr);
    delete vk_; vk_ = nullptr;
}

void App::mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        // rendering will come later
    }
}

void App::run() {
    mainLoop();
}
