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
#include "vulkan/BufferUtils.h"
#include <cstring>

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

    // Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = vk->depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 2;
    rpci.pAttachments = attachments;
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

    // Vertex input (position + color)
    VkVertexInputBindingDescription binding{}; binding.binding = 0; binding.stride = sizeof(float)*5; binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].binding = 0; attrs[0].location = 0; attrs[0].format = VK_FORMAT_R32G32_SFLOAT; attrs[0].offset = 0; // pos
    attrs[1].binding = 0; attrs[1].location = 1; attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[1].offset = sizeof(float)*2; // color
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

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
    // descriptor set layout will be created later; keep placeholder if not set yet
    if (vk->descriptorSetLayout) {
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &vk->descriptorSetLayout;
    } else {
        plci.setLayoutCount = 0;
    }
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
}

void Renderer::recordCommandBuffer(VkObjects* vk, VkCommandBuffer cmd, uint32_t imageIndex) {
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) throw std::runtime_error("Failed to begin command buffer");

    VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpbi.renderPass = vk->renderPass;
    rpbi.framebuffer = vk->swapchainFramebuffers[imageIndex];
    rpbi.renderArea.offset = {0,0};
    rpbi.renderArea.extent = vk->swapchainExtent;
    VkClearValue clearValues[2];
    clearValues[0].color = {{0.02f,0.02f,0.025f,1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    rpbi.clearValueCount = 1; // depth is handled by separate layout; keep one clear for color currently
    rpbi.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->graphicsPipeline);
    if (!vk->descriptorSets.empty()) {
        VkDescriptorSet set = vk->descriptorSets[imageIndex % vk->descriptorSets.size()];
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &set, 0, nullptr);
    }
    if (vk->vertexBuffer) {
        VkBuffer buffers[] = { vk->vertexBuffer };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        if (vk->indexBuffer && vk->indexCount > 0) {
            vkCmdBindIndexBuffer(cmd, vk->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, vk->indexCount, 1, 0, 0, 0);
        } else {
            vkCmdDraw(cmd, vk->vertexCount > 0 ? vk->vertexCount : 3, 1, 0, 0);
        }
    } else {
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
    // UI callback (records additional draw calls)
    if (vk->uiRecordCallback) {
        vk->uiRecordCallback(cmd, imageIndex);
    }
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) throw std::runtime_error("Failed to record command buffer");
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

    // Update uniform buffer (MVP) for this image using optional callback
    if (imageIndex < vk->uniformBuffersMapped.size() && vk->uniformBuffersMapped[imageIndex]) {
        if (vk->uniformUpdater) {
            vk->uniformUpdater(imageIndex, vk->uniformBuffersMapped[imageIndex]);
        } else {
            float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            std::memcpy(vk->uniformBuffersMapped[imageIndex], identity, sizeof(identity));
        }
    }

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkSemaphore waitSemaphores[] = { vk->imageAvailableSemaphores[vk->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    // Re-record command buffer for this frame
    VkCommandBuffer cmd = vk->commandBuffers[imageIndex];
    // Reset command buffer before recording (pool was created with RESET flag)
    vkResetCommandBuffer(cmd, 0);
    recordCommandBuffer(vk, cmd, imageIndex);
    submitInfo.pCommandBuffers = &cmd;
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

    res = vkQueuePresentKHR(vk->presentQueue, &presentInfo);
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
    for (size_t i=0;i<vk->uniformBuffers.size();++i){ if(vk->uniformBuffers[i]) vkDestroyBuffer(vk->device, vk->uniformBuffers[i], nullptr); if(vk->uniformBuffersMemory[i]) vkFreeMemory(vk->device, vk->uniformBuffersMemory[i], nullptr);}    
    vk->uniformBuffers.clear(); vk->uniformBuffersMemory.clear(); vk->uniformBuffersMapped.clear();
    if (vk->descriptorPool) { vkDestroyDescriptorPool(vk->device, vk->descriptorPool, nullptr); vk->descriptorPool = VK_NULL_HANDLE; }
    if (vk->descriptorSetLayout) { vkDestroyDescriptorSetLayout(vk->device, vk->descriptorSetLayout, nullptr); vk->descriptorSetLayout = VK_NULL_HANDLE; }
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
    // Descriptor set layout (single uniform buffer binding 0)
    VkDescriptorSetLayoutBinding uboBinding{}; uboBinding.binding = 0; uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; uboBinding.descriptorCount = 1; uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; lci.bindingCount = 1; lci.pBindings = &uboBinding;
    if (vkCreateDescriptorSetLayout(vk->device, &lci, nullptr, &vk->descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
    // Uniform buffers per swapchain image
    VkDeviceSize uboSize = sizeof(float)*16; // Mat4 MVP placeholder
    vk->uniformBuffers.resize(vk->swapchainFramebuffers.size());
    vk->uniformBuffersMemory.resize(vk->swapchainFramebuffers.size());
    vk->uniformBuffersMapped.resize(vk->swapchainFramebuffers.size());
    for (size_t i=0;i<vk->uniformBuffers.size();++i){
        vkbuf::createBuffer(vk->device, vk->physicalDevice, uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vk->uniformBuffers[i], vk->uniformBuffersMemory[i]);
        vkMapMemory(vk->device, vk->uniformBuffersMemory[i], 0, uboSize, 0, &vk->uniformBuffersMapped[i]);
        std::memset(vk->uniformBuffersMapped[i], 0, static_cast<size_t>(uboSize));
    }
    // Descriptor pool
    VkDescriptorPoolSize poolSize{}; poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; poolSize.descriptorCount = static_cast<uint32_t>(vk->uniformBuffers.size());
    VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; dpci.maxSets = static_cast<uint32_t>(vk->uniformBuffers.size()); dpci.poolSizeCount = 1; dpci.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(vk->device, &dpci, nullptr, &vk->descriptorPool) != VK_SUCCESS) throw std::runtime_error("Failed to create descriptor pool");
    // Allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(vk->uniformBuffers.size(), vk->descriptorSetLayout);
    VkDescriptorSetAllocateInfo dsai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; dsai.descriptorPool = vk->descriptorPool; dsai.descriptorSetCount = static_cast<uint32_t>(layouts.size()); dsai.pSetLayouts = layouts.data();
    vk->descriptorSets.resize(layouts.size());
    if (vkAllocateDescriptorSets(vk->device, &dsai, vk->descriptorSets.data()) != VK_SUCCESS) throw std::runtime_error("Failed to allocate descriptor sets");
    for (size_t i=0;i<vk->descriptorSets.size();++i){
        VkDescriptorBufferInfo dbi{}; dbi.buffer = vk->uniformBuffers[i]; dbi.offset = 0; dbi.range = uboSize;
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; w.dstSet = vk->descriptorSets[i]; w.dstBinding = 0; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w.pBufferInfo = &dbi;
        vkUpdateDescriptorSets(vk->device, 1, &w, 0, nullptr);
    }
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
