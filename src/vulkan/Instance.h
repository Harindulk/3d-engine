#pragma once

#include "vulkan/VkObjects.h"

namespace vulkan {
struct InstanceManager {
    static void createInstance(VkObjects* vk);
#ifdef AURORA_ENABLE_VALIDATION
    static bool checkValidationLayerSupport();
    static void setupDebugMessenger(VkObjects* vk);
    static void destroyDebugMessenger(VkObjects* vk);
#endif
};
}
