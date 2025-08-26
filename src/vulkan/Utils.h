#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vkutils {
    std::vector<char> readFile(const std::string& path);
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
}
