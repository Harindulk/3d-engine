#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

#include "vulkan/Utils.h"
#include "vulkan/Swapchain.h"

namespace vulkan {

void Renderer::createRenderPass(VkObjects* vk) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vk->swapchainImageFormat;
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

    if (vkCreateRenderPass(vk->device, &rpci, nullptr, &vk->renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void Renderer::createGraphicsPipeline(VkObjects* vk) {
    auto findShader = [](const std::string& name) {
        std::vector<std::string> candidates = {
            std::string("build/shaders/") + name,
            std::string("shaders/") + name,
            std::string("../build/shaders/") + name,
            std::string("../shaders/") + name
        };
        for (const auto &c : candidates) {
            if (std::filesystem::exists(c)) return c;
        }
        return std::string();
    };

    std::string vertPath = findShader("triangle.vert.spv");
    std::string fragPath = findShader("triangle.frag.spv");
    if (vertPath.empty() || fragPath.empty()) {
        throw std::runtime_error("Failed to find triangle shader SPIR-V files in expected locations");
    }
    auto vertCode = vkutils::readFile(vertPath);
    auto fragCode = vkutils::readFile(fragPath);

    VkShaderModule vertModule = vkutils::createShaderModule(vk->device, vertCode);
    VkShaderModule fragModule = vkutils::createShaderModule(vk->device, fragCode);

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
    viewport.width = (float)vk->swapchainExtent.width;
    viewport.height = (float)vk->swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0,0};
    scissor.extent = vk->swapchainExtent;

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
    if (vkCreatePipelineLayout(vk->device, &plci, nullptr, &vk->pipelineLayout) != VK_SUCCESS) {
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
    pci.layout = vk->pipelineLayout;
    pci.renderPass = vk->renderPass;
    pci.subpass = 0;

    if (vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pci, nullptr, &vk->graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(vk->device, fragModule, nullptr);
    vkDestroyShaderModule(vk->device, vertModule, nullptr);
}

void Renderer::createCommandPool(VkObjects* vk) {
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    ci.queueFamilyIndex = vk->graphicsQueueFamily;
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vk->device, &ci, nullptr, &vk->commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void Renderer::createCommandBuffers(VkObjects* vk) {
    vk->commandBuffers.resize(vk->swapchainFramebuffers.size());
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = vk->commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = static_cast<uint32_t>(vk->commandBuffers.size());
    if (vkAllocateCommandBuffers(vk->device, &ai, vk->commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    for (size_t i = 0; i < vk->commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        if (vkBeginCommandBuffer(vk->commandBuffers[i], &bi) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass = vk->renderPass;
        rpbi.framebuffer = vk->swapchainFramebuffers[i];
        rpbi.renderArea.offset = {0,0};
        rpbi.renderArea.extent = vk->swapchainExtent;
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        rpbi.clearValueCount = 1;
        rpbi.pClearValues = &clearColor;

        vkCmdBeginRenderPass(vk->commandBuffers[i], &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(vk->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk->graphicsPipeline);
        vkCmdDraw(vk->commandBuffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(vk->commandBuffers[i]);

        if (vkEndCommandBuffer(vk->commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer");
        }
    }
}

void Renderer::createSyncObjects(VkObjects* vk) {
    size_t maxFrames = vk->swapchainFramebuffers.size();
    vk->imageAvailableSemaphores.resize(maxFrames);
    vk->renderFinishedSemaphores.resize(maxFrames);
    vk->inFlightFences.resize(maxFrames);

    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < maxFrames; ++i) {
        if (vkCreateSemaphore(vk->device, &sci, nullptr, &vk->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vk->device, &sci, nullptr, &vk->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vk->device, &fci, nullptr, &vk->inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame");
        }
    }
}

void Renderer::drawFrame(VkObjects* vk, GLFWwindow* window) {
    vkWaitForFences(vk->device, 1, &vk->inFlightFences[vk->currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(vk->device, vk->swapchain, UINT64_MAX, vk->imageAvailableSemaphores[vk->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "Renderer::drawFrame - acquire returned OUT_OF_DATE, recreating..." << std::endl;
        // Recreate swapchain and renderer resources
        vulkan::SwapchainManager::recreateSwapchain(vk, window);
        vulkan::Renderer::recreate(vk, window);
        return;
    } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkSemaphore waitSemaphores[] = { vk->imageAvailableSemaphores[vk->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk->commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = { vk->renderFinishedSemaphores[vk->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(vk->device, 1, &vk->inFlightFences[vk->currentFrame]);
    if (vkQueueSubmit(vk->graphicsQueue, 1, &submitInfo, vk->inFlightFences[vk->currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vk->swapchain;
    presentInfo.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(vk->graphicsQueue, &presentInfo);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        std::cout << "Renderer::drawFrame - present returned OUT_OF_DATE/SUBOPTIMAL, recreating..." << std::endl;
        vulkan::SwapchainManager::recreateSwapchain(vk, window);
        vulkan::Renderer::recreate(vk, window);
    } else if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    vk->currentFrame = (vk->currentFrame + 1) % vk->inFlightFences.size();
}

void Renderer::cleanupRenderer(VkObjects* vk) {
    if (!vk) return;
    for (auto s : vk->renderFinishedSemaphores) if (s) vkDestroySemaphore(vk->device, s, nullptr);
    for (auto s : vk->imageAvailableSemaphores) if (s) vkDestroySemaphore(vk->device, s, nullptr);
    for (auto f : vk->inFlightFences) if (f) vkDestroyFence(vk->device, f, nullptr);

    if (vk->commandPool) vkDestroyCommandPool(vk->device, vk->commandPool, nullptr);

    // Note: framebuffers, image views, swapchain destroyed by SwapchainManager

    if (vk->graphicsPipeline) { vkDestroyPipeline(vk->device, vk->graphicsPipeline, nullptr); vk->graphicsPipeline = VK_NULL_HANDLE; }
    if (vk->pipelineLayout) { vkDestroyPipelineLayout(vk->device, vk->pipelineLayout, nullptr); vk->pipelineLayout = VK_NULL_HANDLE; }
    if (vk->renderPass) { vkDestroyRenderPass(vk->device, vk->renderPass, nullptr); vk->renderPass = VK_NULL_HANDLE; }
}

void Renderer::recreate(VkObjects* vk, GLFWwindow* window) {
    // wait idle
    if (vk->device) vkDeviceWaitIdle(vk->device);
    std::cout << "Renderer: recreate() start" << std::endl;

    // cleanup renderer specific resources
    std::cout << "Renderer: cleaning up renderer resources" << std::endl;
    cleanupRenderer(vk);

    // swapchain/framebuffers handled by SwapchainManager; ensure they are valid
    // recreate renderpass/pipeline
    std::cout << "Renderer: creating render pass" << std::endl;
    createRenderPass(vk);
    std::cout << "Renderer: creating graphics pipeline" << std::endl;
    createGraphicsPipeline(vk);
    // now that new render pass/pipeline exist, rebuild swapchain framebuffers
    std::cout << "Renderer: creating framebuffers" << std::endl;
    vulkan::SwapchainManager::createFramebuffers(vk);

    // recreate framebuffers for new swapchain extent
    // command buffers and sync objects need to be recreated
    // command pool recreated
    if (vk->commandPool) {
        std::cout << "Renderer: destroying old command pool" << std::endl;
        vkDestroyCommandPool(vk->device, vk->commandPool, nullptr);
    }
    std::cout << "Renderer: creating command pool" << std::endl;
    createCommandPool(vk);
    std::cout << "Renderer: creating command buffers" << std::endl;
    createCommandBuffers(vk);
    std::cout << "Renderer: creating sync objects" << std::endl;
    createSyncObjects(vk);
    std::cout << "Renderer: recreate() complete" << std::endl;
}

} // namespace vulkan
