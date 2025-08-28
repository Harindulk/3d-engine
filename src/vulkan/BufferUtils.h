#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

struct VkObjects;

namespace vkbuf {
VkDeviceSize findMemoryTypeIndex(VkPhysicalDevice phys, uint32_t typeBits, VkMemoryPropertyFlags props);

void createBuffer(VkDevice device,
                  VkPhysicalDevice phys,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props,
                  VkBuffer& outBuffer,
                  VkDeviceMemory& outMemory);

template<typename T>
void uploadToMappedMemory(VkDevice device, VkDeviceMemory memory, const std::vector<T>& data) {
    void* mapped = nullptr;
    vkMapMemory(device, memory, 0, sizeof(T)*data.size(), 0, &mapped);
    std::memcpy(mapped, data.data(), sizeof(T)*data.size());
    vkUnmapMemory(device, memory);
}
}
