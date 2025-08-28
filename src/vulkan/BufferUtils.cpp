#include "vulkan/BufferUtils.h"
#include <stdexcept>
#include <cstring>

namespace vkbuf {

VkDeviceSize findMemoryTypeIndex(VkPhysicalDevice phys, uint32_t typeBits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps; vkGetPhysicalDeviceMemoryProperties(phys, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

void createBuffer(VkDevice device,
                  VkPhysicalDevice phys,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props,
                  VkBuffer& outBuffer,
                  VkDeviceMemory& outMemory) {
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bci, nullptr, &outBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(device, outBuffer, &req);
    uint32_t memType = (uint32_t)findMemoryTypeIndex(phys, req.memoryTypeBits, props);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size; mai.memoryTypeIndex = memType;
    if (vkAllocateMemory(device, &mai, nullptr, &outMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory");
    }
    vkBindBufferMemory(device, outBuffer, outMemory, 0);
}

}
