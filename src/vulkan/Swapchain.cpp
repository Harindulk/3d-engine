#include "Swapchain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <algorithm>

namespace {
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, nullptr);
    if (presentCount != 0) {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, details.presentModes.data());
    }
    return details;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available) {
    for (const auto& av : available) {
        if (av.format == VK_FORMAT_B8G8R8A8_SRGB && av.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return av;
        }
    }
    return available.empty() ? VkSurfaceFormatKHR{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR} : available[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available) {
    for (const auto& pm : available) {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR) return pm;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
    int width = 0, height = 0;
    // If window is minimized, glfwGetFramebufferSize may return 0; wait until non-zero.
    do {
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            // wait for events (blocks until something happens) to avoid busy-loop
            glfwWaitEvents();
        }
    } while (width == 0 || height == 0);
    VkExtent2D actual{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    actual.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, actual.width));
    actual.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, actual.height));
    return actual;
}
}

namespace vulkan {

void SwapchainManager::createSwapchain(VkObjects* vk, GLFWwindow* window) {
    auto details = querySwapchainSupport(vk->physicalDevice, vk->surface);
    if (details.formats.empty() || details.presentModes.empty()) throw std::runtime_error("Swapchain not supported by surface");

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
    VkExtent2D extent = chooseSwapExtent(details.capabilities, window);

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface = vk->surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = surfaceFormat.format;
    ci.imageColorSpace = surfaceFormat.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.queueFamilyIndexCount = 0;
    ci.pQueueFamilyIndices = nullptr;

    ci.preTransform = details.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = presentMode;
    ci.clipped = VK_TRUE;
    ci.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vk->device, &ci, nullptr, &vk->swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }
    // record chosen format and extent
    vk->swapchainImageFormat = surfaceFormat.format;
    vk->swapchainExtent = extent;

    uint32_t scImageCount = 0;
    vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &scImageCount, nullptr);
    vk->swapchainImages.resize(scImageCount);
    vkGetSwapchainImagesKHR(vk->device, vk->swapchain, &scImageCount, vk->swapchainImages.data());
}

void SwapchainManager::createImageViews(VkObjects* vk) {
    vk->swapchainImageViews.resize(vk->swapchainImages.size());
    auto details = querySwapchainSupport(vk->physicalDevice, vk->surface);
    VkFormat fmt = details.formats.empty() ? VK_FORMAT_B8G8R8A8_SRGB : chooseSwapSurfaceFormat(details.formats).format;

    for (size_t i = 0; i < vk->swapchainImages.size(); ++i) {
        VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        iv.image = vk->swapchainImages[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = fmt;
        iv.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.baseMipLevel = 0;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.baseArrayLayer = 0;
        iv.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vk->device, &iv, nullptr, &vk->swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views");
        }
    }
}

void SwapchainManager::createFramebuffers(VkObjects* vk) {
    vk->swapchainFramebuffers.resize(vk->swapchainImageViews.size());
    for (size_t i = 0; i < vk->swapchainImageViews.size(); ++i) {
        VkImageView attachments[] = { vk->swapchainImageViews[i] };
        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass = vk->renderPass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = vk->swapchainExtent.width;
        fci.height = vk->swapchainExtent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(vk->device, &fci, nullptr, &vk->swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void SwapchainManager::cleanupSwapchain(VkObjects* vk) {
    if (!vk) return;
    for (auto fb : vk->swapchainFramebuffers) if (fb) vkDestroyFramebuffer(vk->device, fb, nullptr);
    vk->swapchainFramebuffers.clear();
    if (vk->graphicsPipeline) vkDestroyPipeline(vk->device, vk->graphicsPipeline, nullptr);
    if (vk->pipelineLayout) vkDestroyPipelineLayout(vk->device, vk->pipelineLayout, nullptr);
    if (vk->renderPass) vkDestroyRenderPass(vk->device, vk->renderPass, nullptr);
    for (auto iv : vk->swapchainImageViews) if (iv) vkDestroyImageView(vk->device, iv, nullptr);
    vk->swapchainImageViews.clear();
    if (vk->swapchain) {
        vkDestroySwapchainKHR(vk->device, vk->swapchain, nullptr);
        vk->swapchain = VK_NULL_HANDLE;
    }
}

void SwapchainManager::recreateSwapchain(VkObjects* vk, GLFWwindow* window) {
    // wait for device idle before tearing down
    if (vk->device) vkDeviceWaitIdle(vk->device);
    cleanupSwapchain(vk);
    createSwapchain(vk, window);
    createImageViews(vk);
    // render pass and pipeline may depend on extent; leave framebuffer creation to caller
    createFramebuffers(vk);
}

} // namespace vulkan
