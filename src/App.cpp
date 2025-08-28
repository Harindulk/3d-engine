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

// Optional verbose logging toggle
#ifdef AURORA_VERBOSE_LOG
#define AURORA_LOG_VERBOSE(x) std::cout << x << std::endl;
#else
#define AURORA_LOG_VERBOSE(x) do {} while(0)
#endif

#ifdef AURORA_ENABLE_VALIDATION
#include <vulkan/vulkan_core.h>
#endif

// VkObjects moved to its own header
#include "vulkan/VkObjects.h"
#include "vulkan/Utils.h"
#include "vulkan/Instance.h"
#include "vulkan/Device.h"
#include "vulkan/Swapchain.h"
#include "vulkan/Renderer.h"
#include "window/Window.h"
#include "render/Mesh.h"
#include "vulkan/BufferUtils.h"
#include "vulkan/Utils.h"

    App::App(int width, int height, const char* title) {
        window_ = new Window(width, height, title);
        initVulkan();
    }

    App::~App() {
        cleanupVulkan();
        if (window_) {
            delete window_;
            window_ = nullptr;
        }
    }

    // helpers and swapchain responsibilities moved to vulkan::SwapchainManager and vkutils

    void App::initVulkan() {
        vk_ = new VkObjects();
    std::cout << "App: creating Vulkan instance..." << std::endl;
    vulkan::InstanceManager::createInstance(vk_);
    std::cout << "App: instance created" << std::endl;
    createSurface();
    std::cout << "App: surface created" << std::endl;
    vulkan::DeviceManager::pickPhysicalDevice(vk_);
    std::cout << "App: physical device selected" << std::endl;
    vulkan::DeviceManager::createLogicalDevice(vk_);
    std::cout << "App: logical device created" << std::endl;
    vulkan::SwapchainManager::createSwapchain(vk_, window_->getNativeWindow());
    std::cout << "App: swapchain created" << std::endl;
    vulkan::SwapchainManager::createImageViews(vk_);
    std::cout << "App: image views created" << std::endl;

    // Depth resources (choose format and create image+view)
    vk_->depthFormat = vkutils::findDepthFormat(vk_->physicalDevice);
    vkutils::createImage(vk_->device, vk_->physicalDevice, vk_->swapchainExtent.width, vk_->swapchainExtent.height, vk_->depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vk_->depthImage, vk_->depthImageMemory);
    vk_->depthImageView = vkutils::createImageView(vk_->device, vk_->depthImage, vk_->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    // Transition depth layout
    vulkan::Renderer::createCommandPool(vk_); // ensure a pool exists for one-shot (will be recreated later anyway)
    vkutils::transitionImageLayout(vk_->device, vk_->commandPool, vk_->graphicsQueue, vk_->depthImage, vk_->depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    // destroy temp pool resources to recreate properly below
    vkDestroyCommandPool(vk_->device, vk_->commandPool, nullptr); vk_->commandPool = VK_NULL_HANDLE;

    // swapchain format/extent are set by SwapchainManager
        vulkan::Renderer::createRenderPass(vk_);
        std::cout << "App: render pass created" << std::endl;
        vulkan::Renderer::createGraphicsPipeline(vk_);
        std::cout << "App: graphics pipeline created" << std::endl;
    vulkan::SwapchainManager::createFramebuffers(vk_);
    std::cout << "App: framebuffers created" << std::endl;

    // Create triangle mesh GPU buffers BEFORE command buffers so they can record correct draws
    auto tri = render::Mesh::makeTriangle();
    vk_->vertexCount = static_cast<uint32_t>(tri.vertices().size());
    vk_->indexCount = static_cast<uint32_t>(tri.indices().size());
    VkDeviceSize vtxSize = sizeof(Vertex) * tri.vertices().size();
    VkDeviceSize idxSize = sizeof(uint32_t) * tri.indices().size();
    if (vk_->vertexCount) {
        vkbuf::createBuffer(vk_->device, vk_->physicalDevice, vtxSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vk_->vertexBuffer, vk_->vertexBufferMemory);
        void* mapped = nullptr; vkMapMemory(vk_->device, vk_->vertexBufferMemory, 0, vtxSize, 0, &mapped);
        std::memcpy(mapped, tri.vertices().data(), vtxSize); vkUnmapMemory(vk_->device, vk_->vertexBufferMemory);
    }
    if (vk_->indexCount) {
        vkbuf::createBuffer(vk_->device, vk_->physicalDevice, idxSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vk_->indexBuffer, vk_->indexBufferMemory);
        void* mappedIdx = nullptr; vkMapMemory(vk_->device, vk_->indexBufferMemory, 0, idxSize, 0, &mappedIdx);
        std::memcpy(mappedIdx, tri.indices().data(), idxSize); vkUnmapMemory(vk_->device, vk_->indexBufferMemory);
    }

    vulkan::Renderer::createCommandPool(vk_);
    std::cout << "App: command pool created" << std::endl;
    vulkan::Renderer::createCommandBuffers(vk_);
    std::cout << "App: command buffers created" << std::endl;
    vulkan::Renderer::createSyncObjects(vk_);
    std::cout << "App: sync objects created" << std::endl;
    }

    void App::createSurface() {
    if (glfwCreateWindowSurface(vk_->instance, window_->getNativeWindow(), nullptr, &vk_->surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    // Device selection and creation handled by vulkan::DeviceManager

    // Swapchain and helper implementations moved to vulkan::SwapchainManager


    // Renderer responsibilities moved to vulkan::Renderer

    void App::cleanupVulkan() {
        if (!vk_) return;
        // Destroy sync objects
        for (auto s : vk_->renderFinishedSemaphores) if (s) vkDestroySemaphore(vk_->device, s, nullptr);
        for (auto s : vk_->imageAvailableSemaphores) if (s) vkDestroySemaphore(vk_->device, s, nullptr);
        for (auto f : vk_->inFlightFences) if (f) vkDestroyFence(vk_->device, f, nullptr);

    // Destroy command pool
    if (vk_->commandPool) vkDestroyCommandPool(vk_->device, vk_->commandPool, nullptr);
    if (vk_->depthImageView) vkDestroyImageView(vk_->device, vk_->depthImageView, nullptr);
    if (vk_->depthImage) vkDestroyImage(vk_->device, vk_->depthImage, nullptr);
    if (vk_->depthImageMemory) vkFreeMemory(vk_->device, vk_->depthImageMemory, nullptr);
    // Destroy geometry buffers
    if (vk_->vertexBuffer) vkDestroyBuffer(vk_->device, vk_->vertexBuffer, nullptr);
    if (vk_->vertexBufferMemory) vkFreeMemory(vk_->device, vk_->vertexBufferMemory, nullptr);
    if (vk_->indexBuffer) vkDestroyBuffer(vk_->device, vk_->indexBuffer, nullptr);
    if (vk_->indexBufferMemory) vkFreeMemory(vk_->device, vk_->indexBufferMemory, nullptr);

    // Destroy swapchain and related
    vulkan::SwapchainManager::cleanupSwapchain(vk_);

        // Destroy debug messenger (managed by InstanceManager)
    #ifdef AURORA_ENABLE_VALIDATION
            vulkan::InstanceManager::destroyDebugMessenger(vk_);
    #endif
        // Destroy logical device
        vulkan::DeviceManager::destroyDevice(vk_);
        if (vk_->surface) vkDestroySurfaceKHR(vk_->instance, vk_->surface, nullptr);
        if (vk_->instance) vkDestroyInstance(vk_->instance, nullptr);
        delete vk_; vk_ = nullptr;
    }

    void App::mainLoop() {
        lastFPSTime_ = glfwGetTime();
        frameCount_ = 0;
        std::cout << "App: entering mainLoop" << std::endl;
        while (!window_->shouldClose()) {
            window_->pollEvents();
            // If window was resized, recreate swapchain-dependent resources
            if (window_->wasResized()) {
                recreateResources();
            }
            vulkan::Renderer::drawFrame(vk_, window_->getNativeWindow());

            // FPS counting
            frameCount_++;
            double now = glfwGetTime();
            double elapsed = now - lastFPSTime_;
            if (elapsed >= 1.0) {
                fps_ = static_cast<int>(frameCount_ / elapsed + 0.5);
                frameCount_ = 0;
                lastFPSTime_ = now;
                // update window title with FPS (Window encapsulates GLFW)
                std::string title = "Aurora3D - FPS: " + std::to_string(fps_);
                window_->setTitle(title);
            }
        }
    }

    void App::recreateResources() {
        if (!vk_ || !vk_->device) return;
    std::cout << "App: recreating resources due to resize" << std::endl;
    // wait and recreate swapchain and renderer resources
    vulkan::SwapchainManager::recreateSwapchain(vk_, window_->getNativeWindow());
    std::cout << "App: swapchain recreation returned" << std::endl;
    // Recreate depth resources for new extent
    if (vk_->depthImageView) vkDestroyImageView(vk_->device, vk_->depthImageView, nullptr);
    if (vk_->depthImage) vkDestroyImage(vk_->device, vk_->depthImage, nullptr);
    if (vk_->depthImageMemory) vkFreeMemory(vk_->device, vk_->depthImageMemory, nullptr);
    vkutils::createImage(vk_->device, vk_->physicalDevice, vk_->swapchainExtent.width, vk_->swapchainExtent.height, vk_->depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vk_->depthImage, vk_->depthImageMemory);
    vk_->depthImageView = vkutils::createImageView(vk_->device, vk_->depthImage, vk_->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    // transition using existing command pool after renderer recreate will rebuild pool; create temp if needed
        try {
            vulkan::Renderer::recreate(vk_, window_->getNativeWindow());
            std::cout << "App: renderer recreation returned" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "App: exception during Renderer::recreate: " << e.what() << std::endl;
            return;
        } catch (...) {
            std::cerr << "App: unknown exception during Renderer::recreate" << std::endl;
            return;
        }
        window_->clearResizedFlag();
    }

    void App::run() {
        try {
            mainLoop();
        } catch (const std::exception &e) {
            std::cerr << "Unhandled exception in App::run: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "Unhandled unknown exception in App::run" << std::endl;
            throw;
        }
    }

    bool App::frame() {
        if (window_->shouldClose()) return false;
        if (lastFPSTime_ == 0.0) {
            lastFPSTime_ = glfwGetTime();
        }
        window_->pollEvents();
        if (window_->wasResized()) {
            recreateResources();
        }
        vulkan::Renderer::drawFrame(vk_, window_->getNativeWindow());
        frameCount_++;
        double now = glfwGetTime();
        double elapsed = now - lastFPSTime_;
        if (elapsed >= 1.0) {
            fps_ = static_cast<int>(frameCount_ / elapsed + 0.5);
            frameCount_ = 0;
            lastFPSTime_ = now;
            std::string title = "Aurora3D - FPS: " + std::to_string(fps_);
            window_->setTitle(title);
        }
        return true;
    }

    void App::resetFrameStats() {
        frameCount_ = 0;
        lastFPSTime_ = 0.0;
        fps_ = 0;
    }

    // Validation / debug handled by vulkan::InstanceManager
