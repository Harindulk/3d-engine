#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkutils {
    std::vector<char> readFile(const std::string& path);
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    
        // New image / format helpers
        VkFormat findSupportedFormat(VkPhysicalDevice phys, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat(VkPhysicalDevice phys);
        void createImage(VkDevice device, VkPhysicalDevice phys, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect);
        void transitionImageLayout(VkDevice device, VkCommandPool cmdPool, VkQueue queue, VkImage image, VkFormat format,
                                    VkImageLayout oldLayout, VkImageLayout newLayout);
}
