module;

#include <vulkan/vulkan.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
export module CustomVulkan.Pipeline;
import CustomVulkan.Common;
import CustomVulkan.Commands;
export namespace CustomVulkan {
    struct CreateVulkanGraphicsPipeline {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanGraphicsPipeline() const -> CreateVulkanFramebuffers {
                const auto vertShaderCode = readFile("resource/shader/vert.spv");
                const auto fragShaderCode = readFile("resource/shader/frag.spv");

                const vk::Device device = ctx->vulkanContext.device.get();
                const auto vertModuleRes = createShaderModule(device, vertShaderCode);
                const auto fragModuleRes = createShaderModule(device, fragShaderCode);
                const vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
                        {},
                        vk::ShaderStageFlagBits::eVertex,
                        vertModuleRes.get(),
                        "main"};
                const vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
                        {},
                        vk::ShaderStageFlagBits::eFragment,
                        fragModuleRes.get(),
                        "main"};

                const vk::PipelineShaderStageCreateInfo shaderStages[] = {
                        vertShaderStageInfo,
                        fragShaderStageInfo};

                constexpr vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
                        {},
                        0,
                        nullptr,
                        0,
                        nullptr,
                        nullptr};

                constexpr vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
                        {},
                        vk::PrimitiveTopology::eTriangleList,
                        false,
                        nullptr};

                constexpr vk::PipelineViewportStateCreateInfo viewportState{
                        {},
                        1,
                        nullptr,
                        1,
                        nullptr,
                        nullptr};

                constexpr vk::PipelineRasterizationStateCreateInfo rasterizer{
                        {},
                        false,
                        false,
                        vk::PolygonMode::eFill,
                        vk::CullModeFlagBits::eBack,
                        vk::FrontFace::eClockwise,
                        false,
                        0.0f,
                        0.0f,
                        0.0f,
                        1.0f};

                constexpr vk::PipelineMultisampleStateCreateInfo multisampling{
                        {},
                        vk::SampleCountFlagBits::e1,
                        false};

                constexpr vk::PipelineColorBlendAttachmentState colorBlendAttachment{
                        false,
                        vk::BlendFactor::eZero,
                        vk::BlendFactor::eZero,
                        vk::BlendOp::eAdd,
                        vk::BlendFactor::eZero,
                        vk::BlendFactor::eZero,
                        vk::BlendOp::eAdd,
                        {vk::ColorComponentFlagBits::eR |
                         vk::ColorComponentFlagBits::eG |
                         vk::ColorComponentFlagBits::eB |
                         vk::ColorComponentFlagBits::eA}};

                vk::PipelineColorBlendStateCreateInfo colorBlending{
                        {},
                        false,
                        vk::LogicOp::eCopy,
                        1,
                        &colorBlendAttachment,
                        {0.0f, 0.0f, 0.0f, 0.0f},
                        nullptr};

                const std::vector dynamicStates = {
                        vk::DynamicState::eViewport,
                        vk::DynamicState::eScissor};

                const vk::PipelineDynamicStateCreateInfo dynamicState{
                        {},
                        static_cast<uint32_t>(dynamicStates.size()),
                        dynamicStates.data()};

                constexpr vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
                        {},
                        0,
                        nullptr,
                        0,
                        nullptr,
                        nullptr};

                try {
                    ctx->vulkanContext.pipelineLayout = VulkanResource<vk::PipelineLayout>(
                            device.createPipelineLayout(pipelineLayoutInfo),
                            [device](const vk::PipelineLayout &py) {
                                device.destroyPipelineLayout(py);
                                std::cout << "[Vulkan 销毁信息]: 销毁管道布局(pipelineLayout)!\n";
                            });
                } catch (const vk::SystemError &err) {
                    throw std::runtime_error("创建管道布局失败: " + std::string(err.what()));
                }

                vk::GraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.pDynamicState = &dynamicState;
                pipelineInfo.layout = ctx->vulkanContext.pipelineLayout.get();
                pipelineInfo.renderPass = ctx->vulkanContext.renderPass.get();
                pipelineInfo.subpass = 0;
                pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

                try {
                    ctx->vulkanContext.graphicsPipeline = VulkanResource<vk::Pipeline>(
                            device.createGraphicsPipelines(nullptr, {pipelineInfo}).value[0],
                            [device](const vk::Pipeline &p) {
                                device.destroyPipeline(p);
                                std::cout << "[Vulkan 销毁信息]: 销毁图形管线(graphicsPipeline)!\n";
                            });
                } catch (const vk::SystemError &err) {
                    throw std::runtime_error("创建图形管线失败: " + std::string(err.what()));
                }

                return {ctx};
            }

        private:
            static auto readFile(const std::string &filename) -> std::vector<char> {
                std::ifstream file{
                        filename,
                        std::ios::ate | std::ios::binary};
                if (!file.is_open()) {
                    throw std::runtime_error("无法打开文件");
                }
                const std::streamsize fileSize = file.tellg();
                std::vector<char> buffer(fileSize);
                file.seekg(0);
                file.read(buffer.data(), fileSize);
                file.close();
                return buffer;
            }

            static auto createShaderModule(
                    const vk::Device &device,
                    const std::vector<char> &code) -> VulkanResource<vk::ShaderModule> {
                const vk::ShaderModuleCreateInfo createInfo{
                        {},
                        code.size(),
                        reinterpret_cast<const uint32_t *>(code.data())};

                vk::ShaderModule handle = nullptr;

                try {
                    handle = device.createShaderModule(createInfo);
                } catch (const vk::SystemError &err) {
                    std::cerr << "[Vulkan 错误]: ShaderModule 创建失败: " << err.what() << std::endl;
                }
                return {
                        handle,
                        [device](const vk::ShaderModule &sm) {
                            device.destroyShaderModule(sm);
                            std::cout << "[Vulkan 销毁]: ShaderModule 已自动释放\n";
                        }};
            }
    };

    struct CreateVulkanRenderPass {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] CreateVulkanGraphicsPipeline createVulkanRenderPass() const {
                const vk::AttachmentDescription colorAttachment{
                        {},
                        ctx->vulkanContext.swapChainImageFormat,
                        vk::SampleCountFlagBits::e1,
                        vk::AttachmentLoadOp::eClear,
                        vk::AttachmentStoreOp::eStore,
                        vk::AttachmentLoadOp::eDontCare,
                        vk::AttachmentStoreOp::eDontCare,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::ePresentSrcKHR};

                constexpr vk::AttachmentReference colorAttachmentRef{
                        0,
                        vk::ImageLayout::eColorAttachmentOptimal};

                // ReSharper disable once CppVariableCanBeMadeConstexpr
                const vk::SubpassDescription subpass{
                        {},
                        vk::PipelineBindPoint::eGraphics,
                        0,
                        nullptr,
                        1,
                        &colorAttachmentRef};

                const vk::RenderPassCreateInfo renderPassInfo{
                        {},
                        1,
                        &colorAttachment,
                        1,
                        &subpass};

                try {
                    const vk::Device device = ctx->vulkanContext.device.get();
                    ctx->vulkanContext.renderPass = VulkanResource<vk::RenderPass>(
                            device.createRenderPass(renderPassInfo, nullptr),
                            [device](const vk::RenderPass &rp) {
                                device.destroyRenderPass(rp);
                                std::cout << "[Vulkan 销毁信息]: 销毁渲染流程(renderPass)!\n";
                            });
                } catch (const vk::SystemError &err) {
                    throw std::runtime_error("无法创建渲染通道: " + std::string(err.what()));
                }
                return {ctx};
            }
    };
}
