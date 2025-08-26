#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <optional>
#include <cstring>
#include <vector>
#include <fstream>
#include <algorithm>

#ifdef AURORA_ENABLE_VALIDATION
#include <vulkan/vulkan_core.h>
#endif

struct VkObjects {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    // Debug messenger (optional)
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    // Swapchain objects
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D swapchainExtent{};

    // Render objects
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Sync
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
};

    App::App(int width, int height, const char* title) {
        initWindow(width, height, title);
        initVulkan();
    }

    App::~App() {
        cleanupVulkan();
        if (window_) {
            glfwDestroyWindow(window_);
            glfwTerminate();
            window_ = nullptr;
        }
    }

    void App::initWindow(int width, int height, const char* title) {
        if (!glfwInit()) throw std::runtime_error("GLFW init failed");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
        window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window_) throw std::runtime_error("GLFW window creation failed");
    }

    // forward declarations for helpers
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    static SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available);
    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window);
    static std::vector<char> readFile(const std::string& path);
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

    void App::initVulkan() {
        vk_ = new VkObjects();
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createDevice();
        createSwapchain();
        createImageViews();

        // store chosen format/extent for later use
        {
            auto details = querySwapchainSupport(vk_->physicalDevice, vk_->surface);
            vk_->swapchainImageFormat = chooseSwapSurfaceFormat(details.formats).format;
            vk_->swapchainExtent = chooseSwapExtent(details.capabilities, window_);
        }
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void App::createInstance() {
        VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = "Aurora3D";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "Aurora3D";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        // Required extensions from GLFW
        uint32_t glfwExtCount = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
        std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

    #ifdef AURORA_ENABLE_VALIDATION
        // Add debug utils extension
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ci.pApplicationInfo = &appInfo;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();

    #ifdef AURORA_ENABLE_VALIDATION
        // Validation layers
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        if (!checkValidationLayerSupport()) {
            std::cerr << "Warning: validation layers requested but not available\n";
            // continue without layers
        } else {
            ci.enabledLayerCount = 1;
            ci.ppEnabledLayerNames = layers;
        }
    #endif

        if (vkCreateInstance(&ci, nullptr, &vk_->instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance");
        }

    #ifdef AURORA_ENABLE_VALIDATION
        setupDebugMessenger();
    #endif
    }

    void App::createSurface() {
        if (glfwCreateWindowSurface(vk_->instance, window_, nullptr, &vk_->surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void App::pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vk_->instance, &deviceCount, nullptr);
        if (deviceCount == 0) throw std::runtime_error("No Vulkan physical devices found");
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vk_->instance, &deviceCount, devices.data());

        for (auto dev : devices) {
            uint32_t qCount = 0; 
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
            std::vector<VkQueueFamilyProperties> qProps(qCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, qProps.data());

            for (uint32_t i = 0; i < qCount; ++i) {
                VkBool32 present = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, vk_->surface, &present);
                if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                    vk_->physicalDevice = dev;
                    vk_->graphicsQueueFamily = i;
                    break;
                }
            }
            if (vk_->physicalDevice) break;
        }

        if (vk_->physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU");
        }

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(vk_->physicalDevice, &props);
        std::cout << "Selected GPU: " << props.deviceName << std::endl;
    }

    void App::createDevice() {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qci.queueFamilyIndex = vk_->graphicsQueueFamily;
        qci.queueCount = 1;
        qci.pQueuePriorities = &priority;

        const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        dci.queueCreateInfoCount = 1;
        dci.pQueueCreateInfos = &qci;
        dci.enabledExtensionCount = 1;
        dci.ppEnabledExtensionNames = deviceExts;

        if (vkCreateDevice(vk_->physicalDevice, &dci, nullptr, &vk_->device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }
        vkGetDeviceQueue(vk_->device, vk_->graphicsQueueFamily, 0, &vk_->graphicsQueue);
    }

    // --- Swapchain helpers ---

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
        return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available
    }

    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
        if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actual{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        actual.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, actual.width));
        actual.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, actual.height));
        return actual;
    }

    void App::createSwapchain() {
        auto details = querySwapchainSupport(vk_->physicalDevice, vk_->surface);
        if (details.formats.empty() || details.presentModes.empty()) throw std::runtime_error("Swapchain not supported by surface");

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
        VkExtent2D extent = chooseSwapExtent(details.capabilities, window_);

        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
            imageCount = details.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        ci.surface = vk_->surface;
        ci.minImageCount = imageCount;
        ci.imageFormat = surfaceFormat.format;
        ci.imageColorSpace = surfaceFormat.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { vk_->graphicsQueueFamily };
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.queueFamilyIndexCount = 0;
        ci.pQueueFamilyIndices = nullptr;

        ci.preTransform = details.capabilities.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = presentMode;
        ci.clipped = VK_TRUE;
        ci.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(vk_->device, &ci, nullptr, &vk_->swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        uint32_t scImageCount = 0;
        vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &scImageCount, nullptr);
        vk_->swapchainImages.resize(scImageCount);
        vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &scImageCount, vk_->swapchainImages.data());
    }

    void App::createImageViews() {
        vk_->swapchainImageViews.resize(vk_->swapchainImages.size());
        // For simplicity we'll query the surface format again
        auto details = querySwapchainSupport(vk_->physicalDevice, vk_->surface);
        VkFormat fmt = details.formats.empty() ? VK_FORMAT_B8G8R8A8_SRGB : chooseSwapSurfaceFormat(details.formats).format;

        for (size_t i = 0; i < vk_->swapchainImages.size(); ++i) {
            VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            iv.image = vk_->swapchainImages[i];
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

            if (vkCreateImageView(vk_->device, &iv, nullptr, &vk_->swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views");
            }
        }
    }

    void App::cleanupSwapchain() {
        if (!vk_) return;
        // Destroy framebuffers
        for (auto fb : vk_->swapchainFramebuffers) if (fb) vkDestroyFramebuffer(vk_->device, fb, nullptr);
        vk_->swapchainFramebuffers.clear();
        // Destroy pipeline and layout
        if (vk_->graphicsPipeline) vkDestroyPipeline(vk_->device, vk_->graphicsPipeline, nullptr);
        if (vk_->pipelineLayout) vkDestroyPipelineLayout(vk_->device, vk_->pipelineLayout, nullptr);
        if (vk_->renderPass) vkDestroyRenderPass(vk_->device, vk_->renderPass, nullptr);
        // Destroy image views
        for (auto iv : vk_->swapchainImageViews) if (iv) vkDestroyImageView(vk_->device, iv, nullptr);
        vk_->swapchainImageViews.clear();
        // Destroy swapchain
        if (vk_->swapchain) {
            vkDestroySwapchainKHR(vk_->device, vk_->swapchain, nullptr);
            vk_->swapchain = VK_NULL_HANDLE;
        }
    }

    static std::vector<char> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("failed to open file: " + path);
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    void App::createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = vk_->swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = 1;
        rpci.pAttachments = &colorAttachment;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;

        if (vkCreateRenderPass(vk_->device, &rpci, nullptr, &vk_->renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass");
        }
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module;
        if (vkCreateShaderModule(device, &ci, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module");
        }
        return module;
    }

    void App::createGraphicsPipeline() {
        std::string vertPath = std::string("build/shaders/triangle.vert.spv");
        std::string fragPath = std::string("build/shaders/triangle.frag.spv");
        auto vertCode = readFile(vertPath);
        auto fragCode = readFile(fragPath);

        VkShaderModule vertModule = createShaderModule(vk_->device, vertCode);
        VkShaderModule fragModule = createShaderModule(vk_->device, fragCode);

        VkPipelineShaderStageCreateInfo vertStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

        VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertexInput.vertexBindingDescriptionCount = 0;
        vertexInput.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAsm{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAsm.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)vk_->swapchainExtent.width;
        viewport.height = (float)vk_->swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0,0};
        scissor.extent = vk_->swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.depthClampEnable = VK_FALSE;
        raster.rasterizerDiscardEnable = VK_FALSE;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.lineWidth = 1.0f;
        raster.cullMode = VK_CULL_MODE_BACK_BIT;
        raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.logicOpEnable = VK_FALSE;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &colorBlendAttachment;

        VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        plci.setLayoutCount = 0;
        plci.pushConstantRangeCount = 0;
        if (vkCreatePipelineLayout(vk_->device, &plci, nullptr, &vk_->pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        VkGraphicsPipelineCreateInfo pci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pci.stageCount = 2;
        pci.pStages = shaderStages;
        pci.pVertexInputState = &vertexInput;
        pci.pInputAssemblyState = &inputAsm;
        pci.pViewportState = &viewportState;
        pci.pRasterizationState = &raster;
        pci.pMultisampleState = &multisample;
        pci.pColorBlendState = &colorBlend;
        pci.layout = vk_->pipelineLayout;
        pci.renderPass = vk_->renderPass;
        pci.subpass = 0;

        if (vkCreateGraphicsPipelines(vk_->device, VK_NULL_HANDLE, 1, &pci, nullptr, &vk_->graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }

        vkDestroyShaderModule(vk_->device, fragModule, nullptr);
        vkDestroyShaderModule(vk_->device, vertModule, nullptr);
    }

    void App::createFramebuffers() {
        vk_->swapchainFramebuffers.resize(vk_->swapchainImageViews.size());
        for (size_t i = 0; i < vk_->swapchainImageViews.size(); ++i) {
            VkImageView attachments[] = { vk_->swapchainImageViews[i] };
            VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fci.renderPass = vk_->renderPass;
            fci.attachmentCount = 1;
            fci.pAttachments = attachments;
            fci.width = vk_->swapchainExtent.width;
            fci.height = vk_->swapchainExtent.height;
            fci.layers = 1;
            if (vkCreateFramebuffer(vk_->device, &fci, nullptr, &vk_->swapchainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
    }

    void App::createCommandPool() {
        VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        ci.queueFamilyIndex = vk_->graphicsQueueFamily;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (vkCreateCommandPool(vk_->device, &ci, nullptr, &vk_->commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }
    }

    void App::createCommandBuffers() {
        vk_->commandBuffers.resize(vk_->swapchainFramebuffers.size());
        VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ai.commandPool = vk_->commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = static_cast<uint32_t>(vk_->commandBuffers.size());
        if (vkAllocateCommandBuffers(vk_->device, &ai, vk_->commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers");
        }

        for (size_t i = 0; i < vk_->commandBuffers.size(); ++i) {
            VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            if (vkBeginCommandBuffer(vk_->commandBuffers[i], &bi) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer");
            }

            VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            rpbi.renderPass = vk_->renderPass;
            rpbi.framebuffer = vk_->swapchainFramebuffers[i];
            rpbi.renderArea.offset = {0,0};
            rpbi.renderArea.extent = vk_->swapchainExtent;
            VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            rpbi.clearValueCount = 1;
            rpbi.pClearValues = &clearColor;

            vkCmdBeginRenderPass(vk_->commandBuffers[i], &rpbi, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(vk_->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_->graphicsPipeline);
            vkCmdDraw(vk_->commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(vk_->commandBuffers[i]);

            if (vkEndCommandBuffer(vk_->commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to record command buffer");
            }
        }
    }

    void App::createSyncObjects() {
        size_t maxFrames = vk_->swapchainFramebuffers.size();
        vk_->imageAvailableSemaphores.resize(maxFrames);
        vk_->renderFinishedSemaphores.resize(maxFrames);
        vk_->inFlightFences.resize(maxFrames);

        VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (size_t i = 0; i < maxFrames; ++i) {
            if (vkCreateSemaphore(vk_->device, &sci, nullptr, &vk_->imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(vk_->device, &sci, nullptr, &vk_->renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(vk_->device, &fci, nullptr, &vk_->inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects for a frame");
            }
        }
    }

    void App::drawFrame() {
        vkWaitForFences(vk_->device, 1, &vk_->inFlightFences[vk_->currentFrame], VK_TRUE, UINT64_MAX);
        uint32_t imageIndex;
        VkResult res = vkAcquireNextImageKHR(vk_->device, vk_->swapchain, UINT64_MAX, vk_->imageAvailableSemaphores[vk_->currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            // swapchain recreation not implemented yet
            return;
        } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swapchain image");
        }

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkSemaphore waitSemaphores[] = { vk_->imageAvailableSemaphores[vk_->currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vk_->commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = { vk_->renderFinishedSemaphores[vk_->currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(vk_->device, 1, &vk_->inFlightFences[vk_->currentFrame]);
        if (vkQueueSubmit(vk_->graphicsQueue, 1, &submitInfo, vk_->inFlightFences[vk_->currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vk_->swapchain;
        presentInfo.pImageIndices = &imageIndex;

        res = vkQueuePresentKHR(vk_->graphicsQueue, &presentInfo);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            // handle swapchain recreation later
        } else if (res != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swapchain image");
        }

        vk_->currentFrame = (vk_->currentFrame + 1) % vk_->inFlightFences.size();
    }

    void App::cleanupVulkan() {
        if (!vk_) return;
        // Destroy sync objects
        for (auto s : vk_->renderFinishedSemaphores) if (s) vkDestroySemaphore(vk_->device, s, nullptr);
        for (auto s : vk_->imageAvailableSemaphores) if (s) vkDestroySemaphore(vk_->device, s, nullptr);
        for (auto f : vk_->inFlightFences) if (f) vkDestroyFence(vk_->device, f, nullptr);

        // Destroy command pool
        if (vk_->commandPool) vkDestroyCommandPool(vk_->device, vk_->commandPool, nullptr);

        // Destroy swapchain and related
        cleanupSwapchain();

        // Destroy debug messenger first if present
    #ifdef AURORA_ENABLE_VALIDATION
        if (vk_->debugMessenger) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_->instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func) func(vk_->instance, vk_->debugMessenger, nullptr);
            vk_->debugMessenger = VK_NULL_HANDLE;
        }
    #endif
        if (vk_->device) {
            vkDeviceWaitIdle(vk_->device);
            vkDestroyDevice(vk_->device, nullptr);
        }
        if (vk_->surface) vkDestroySurfaceKHR(vk_->instance, vk_->surface, nullptr);
        if (vk_->instance) vkDestroyInstance(vk_->instance, nullptr);
        delete vk_; vk_ = nullptr;
    }

    void App::mainLoop() {
        lastFPSTime_ = glfwGetTime();
        frameCount_ = 0;
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            drawFrame();

            // FPS counting
            frameCount_++;
            double now = glfwGetTime();
            double elapsed = now - lastFPSTime_;
            if (elapsed >= 1.0) {
                fps_ = static_cast<int>(frameCount_ / elapsed + 0.5);
                frameCount_ = 0;
                lastFPSTime_ = now;
                // update window title with FPS
                std::string title = "Aurora3D - FPS: " + std::to_string(fps_);
                glfwSetWindowTitle(window_, title.c_str());
            }
        }
    }

    void App::run() {
        mainLoop();
    }

    #ifdef AURORA_ENABLE_VALIDATION
    bool App::checkValidationLayerSupport() {
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

    void App::setupDebugMessenger() {
        if (!vk_->instance) return;

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

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_->instance, "vkCreateDebugUtilsMessengerEXT");
        if (func) {
            if (func(vk_->instance, &ci, nullptr, &vk_->debugMessenger) != VK_SUCCESS) {
                std::cerr << "Failed to set up debug messenger\\n";
            }
        } else {
            std::cerr << "vkCreateDebugUtilsMessengerEXT not found\\n";
        }
    }
    #endif
