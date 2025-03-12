#include "vsdl_pipeline.h"
#include <SDL3/SDL_log.h>
#include <fstream>
#include <stdexcept>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open file: %s", filename.c_str());
        throw std::runtime_error("Failed to open shader file");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

void vsdl_create_pipeline(VSDL_Context& ctx) {
    auto vertShaderCode = readFile("shaders/tri.vert.spv");
    auto fragShaderCode = readFile("shaders/tri.frag.spv");

    VkShaderModule vertShaderModule, fragShaderModule;
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = vertShaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
    if (vkCreateShaderModule(ctx.device, &createInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex shader module");
        throw std::runtime_error("Shader module creation failed");
    }
    createInfo.codeSize = fragShaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
    if (vkCreateShaderModule(ctx.device, &createInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create fragment shader module");
        throw std::runtime_error("Shader module creation failed");
    }

    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)ctx.swapchainExtent.width;
    viewport.height = (float)ctx.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = ctx.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &ctx.pipelineLayout) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create pipeline layout");
        throw std::runtime_error("Pipeline layout creation failed");
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = ctx.swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.renderPass) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create render pass");
        throw std::runtime_error("Render pass creation failed");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = ctx.pipelineLayout;
    pipelineInfo.renderPass = ctx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx.graphicsPipeline) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create graphics pipeline");
        throw std::runtime_error("Graphics pipeline creation failed");
    }

    vkDestroyShaderModule(ctx.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);

    ctx.framebuffers.resize(ctx.swapchainImageViews.size());
    for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {
        VkImageView attachments[] = { ctx.swapchainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = ctx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = ctx.swapchainExtent.width;
        framebufferInfo.height = ctx.swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &ctx.framebuffers[i]) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create framebuffer %zu", i);
            throw std::runtime_error("Framebuffer creation failed");
        }
    }

    // ImGui initialization after render pass creation
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(ctx.window);

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &ctx.imguiDescriptorPool) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create ImGui descriptor pool");
        throw std::runtime_error("Failed to create ImGui descriptor pool");
    }

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = ctx.instance;
    initInfo.PhysicalDevice = ctx.physicalDevice;
    initInfo.Device = ctx.device;
    initInfo.QueueFamily = 0; // Assuming graphics queue family is 0
    initInfo.Queue = ctx.graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = ctx.imguiDescriptorPool;
    initInfo.RenderPass = ctx.renderPass; // Set the render pass here
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = static_cast<uint32_t>(ctx.swapchainImages.size());
    initInfo.CheckVkResultFn = nullptr;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize ImGui Vulkan backend");
        throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
    }

    // Create a temporary command pool for font uploading
    VkCommandPoolCreateInfo tempPoolInfo = {};
    tempPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    tempPoolInfo.queueFamilyIndex = 0; // Assuming graphics queue family is 0
    tempPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    VkCommandPool tempCommandPool;
    if (vkCreateCommandPool(ctx.device, &tempPoolInfo, nullptr, &tempCommandPool) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create temporary command pool for ImGui");
        throw std::runtime_error("Failed to create temporary command pool for ImGui");
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tempCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer fontCmdBuffer;
    vkAllocateCommandBuffers(ctx.device, &allocInfo, &fontCmdBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(fontCmdBuffer, &beginInfo);

    ImGui_ImplVulkan_CreateFontsTexture();

    vkEndCommandBuffer(fontCmdBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &fontCmdBuffer;
    vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue);

    vkFreeCommandBuffers(ctx.device, tempCommandPool, 1, &fontCmdBuffer);
    vkDestroyCommandPool(ctx.device, tempCommandPool, nullptr);
}