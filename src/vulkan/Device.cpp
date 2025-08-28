#include "Device.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <algorithm>

#include <vulkan/vulkan_core.h>

namespace vulkan {

void DeviceManager::pickPhysicalDevice(VkObjects* vk) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk->instance, &deviceCount, nullptr);
    if (deviceCount == 0) throw std::runtime_error("No Vulkan physical devices found");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk->instance, &deviceCount, devices.data());

    struct Candidate { VkPhysicalDevice dev; uint32_t graphicsIndex; uint32_t presentIndex; int score; };
    std::vector<Candidate> candidates;

    const std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    for (auto dev : devices) {
        VkPhysicalDeviceProperties props{}; VkPhysicalDeviceFeatures features{}; VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceProperties(dev, &props);
        vkGetPhysicalDeviceFeatures(dev, &features);
        vkGetPhysicalDeviceMemoryProperties(dev, &memProps);

        // Check required extensions
        uint32_t extCount = 0; vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> extProps(extCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, extProps.data());
        bool allExt = true;
        for (auto req : requiredExtensions) {
            bool found = false; for (auto &ep : extProps) if (strcmp(ep.extensionName, req)==0) { found = true; break; }
            if (!found) { allExt = false; break; }
        }
        if (!allExt) continue; // cannot use device

        // Queue family selection
        uint32_t qCount = 0; vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> qProps(qCount); vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, qProps.data());
        int foundGraphics = -1; int foundPresent = -1;
        for (uint32_t i = 0; i < qCount; ++i) {
            VkBool32 present = VK_FALSE; vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, vk->surface, &present);
            if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && foundGraphics < 0) foundGraphics = static_cast<int>(i);
            if (present && foundPresent < 0) foundPresent = static_cast<int>(i);
            if (foundGraphics >=0 && foundPresent >=0) break;
        }
        if (foundGraphics < 0 || foundPresent < 0) continue;

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
        if (features.samplerAnisotropy) score += 100; // optional nice-to-have
        score += static_cast<int>(props.limits.maxImageDimension2D);

    candidates.push_back({dev, static_cast<uint32_t>(foundGraphics), static_cast<uint32_t>(foundPresent), score});
    }

    if (candidates.empty()) throw std::runtime_error("Failed to find a suitable GPU (no candidate with required features/extensions)");

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b){ return a.score > b.score; });
    vk->physicalDevice = candidates.front().dev;
    vk->graphicsQueueFamily = candidates.front().graphicsIndex;
    vk->presentQueueFamily = candidates.front().presentIndex;

    VkPhysicalDeviceProperties props{}; vkGetPhysicalDeviceProperties(vk->physicalDevice, &props);
    std::cout << "Selected GPU: " << props.deviceName << " (score=" << candidates.front().score << ")" << std::endl;
}

void DeviceManager::createLogicalDevice(VkObjects* vk) {
    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::vector<uint32_t> uniqueFamilies;
    uniqueFamilies.push_back(vk->graphicsQueueFamily);
    if (vk->presentQueueFamily != vk->graphicsQueueFamily) uniqueFamilies.push_back(vk->presentQueueFamily);
    for (uint32_t fam : uniqueFamilies) {
        VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qci.queueFamilyIndex = fam; qci.queueCount = 1; qci.pQueuePriorities = &priority; queueInfos.push_back(qci);
    }

    const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    dci.pQueueCreateInfos = queueInfos.data();
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExts;

    if (vkCreateDevice(vk->physicalDevice, &dci, nullptr, &vk->device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
    vkGetDeviceQueue(vk->device, vk->graphicsQueueFamily, 0, &vk->graphicsQueue);
    vkGetDeviceQueue(vk->device, vk->presentQueueFamily, 0, &vk->presentQueue);
}

void DeviceManager::destroyDevice(VkObjects* vk) {
    if (!vk) return;
    if (vk->device) {
        vkDeviceWaitIdle(vk->device);
        vkDestroyDevice(vk->device, nullptr);
        vk->device = VK_NULL_HANDLE;
    }
}

} // namespace vulkan
