#include "Instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

#ifdef AURORA_ENABLE_VALIDATION
#include <vulkan/vulkan_core.h>
#endif

namespace vulkan {

void InstanceManager::createInstance(VkObjects* vk) {
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Aurora3D";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Aurora3D";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

#ifdef AURORA_ENABLE_VALIDATION
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &appInfo;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

#ifdef AURORA_ENABLE_VALIDATION
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    if (!checkValidationLayerSupport()) {
        std::cerr << "Warning: validation layers requested but not available\n";
    } else {
        ci.enabledLayerCount = 1;
        ci.ppEnabledLayerNames = layers;
    }
#endif

    if (vkCreateInstance(&ci, nullptr, &vk->instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

#ifdef AURORA_ENABLE_VALIDATION
    setupDebugMessenger(vk);
#endif
}

#ifdef AURORA_ENABLE_VALIDATION
bool InstanceManager::checkValidationLayerSupport() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available.data());

    const char* layerName = "VK_LAYER_KHRONOS_validation";
    for (const auto& layer : available) {
        if (strcmp(layer.layerName, layerName) == 0) return true;
    }
    return false;
}

void InstanceManager::setupDebugMessenger(VkObjects* vk) {
    if (!vk->instance) return;

    VkDebugUtilsMessengerCreateInfoEXT ci{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                             void* pUserData) -> VkBool32 {
        std::cerr << "[VULKAN] " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    };

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk->instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        if (func(vk->instance, &ci, nullptr, &vk->debugMessenger) != VK_SUCCESS) {
            std::cerr << "Failed to set up debug messenger\n";
        }
    } else {
        std::cerr << "vkCreateDebugUtilsMessengerEXT not found\n";
    }
}

void InstanceManager::destroyDebugMessenger(VkObjects* vk) {
    if (!vk->instance || !vk->debugMessenger) return;
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) func(vk->instance, vk->debugMessenger, nullptr);
    vk->debugMessenger = VK_NULL_HANDLE;
}
#endif

} // namespace vulkan
