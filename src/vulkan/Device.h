#pragma once

#include "vulkan/VkObjects.h"

namespace vulkan {
struct DeviceManager {
    // Select a suitable physical device and set vk->physicalDevice and vk->graphicsQueueFamily
    static void pickPhysicalDevice(VkObjects* vk);
    // Create a logical device and obtain the graphics queue
    static void createLogicalDevice(VkObjects* vk);
    // Destroy logical device (wait idle and destroy)
    static void destroyDevice(VkObjects* vk);
};
}
