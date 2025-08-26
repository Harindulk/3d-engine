#include "Device.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vulkan {

void DeviceManager::pickPhysicalDevice(VkObjects* vk) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk->instance, &deviceCount, nullptr);
    if (deviceCount == 0) throw std::runtime_error("No Vulkan physical devices found");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk->instance, &deviceCount, devices.data());

    for (auto dev : devices) {
        uint32_t qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> qProps(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, qProps.data());

        for (uint32_t i = 0; i < qCount; ++i) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, vk->surface, &present);
            if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                vk->physicalDevice = dev;
                vk->graphicsQueueFamily = i;
                break;
            }
        }
        if (vk->physicalDevice) break;
    }

    if (vk->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(vk->physicalDevice, &props);
    std::cout << "Selected GPU: " << props.deviceName << std::endl;
}

void DeviceManager::createLogicalDevice(VkObjects* vk) {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = vk->graphicsQueueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExts;

    if (vkCreateDevice(vk->physicalDevice, &dci, nullptr, &vk->device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
    vkGetDeviceQueue(vk->device, vk->graphicsQueueFamily, 0, &vk->graphicsQueue);
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
