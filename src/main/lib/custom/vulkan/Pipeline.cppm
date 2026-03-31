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

        [[nodiscard]] auto
        createVulkanGraphicsPipeline() const -> CreateVulkanFramebuffers {
            const auto vertShaderCode = readFile("resource/shader/vert.spv");
            const auto fragShaderCode = readFile("resource/shader/frag.spv");

            const vk::Device device  = ctx->vulkanContext.device.get();
            const auto vertModuleRes = createShaderModule(
                    device,
                    vertShaderCode
                    );
            const auto fragModuleRes = createShaderModule(
                    device,
                    fragShaderCode
                    );
            vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo
                    .setStage(vk::ShaderStageFlagBits::eVertex)
                    .setModule(vertModuleRes.get())
                    .setPName("main");

            vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo
                    .setStage(vk::ShaderStageFlagBits::eFragment)
                    .setModule(fragModuleRes.get())
                    .setPName("main");

            const vk::PipelineShaderStageCreateInfo shaderStages[] = {
                    vertShaderStageInfo,
                    fragShaderStageInfo
            };

            vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo
                    .setVertexAttributeDescriptionCount(0)
                    .setPVertexAttributeDescriptions(nullptr)
                    .setVertexBindingDescriptionCount(0)
                    .setVertexBindingDescriptions(nullptr)
                    .setPNext(nullptr);

            vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly
                    .setTopology(vk::PrimitiveTopology::eTriangleList)
                    .setPrimitiveRestartEnable(vk::False)
                    .setPNext(nullptr);

            vk::PipelineViewportStateCreateInfo viewportState{};
            viewportState
                    .setViewportCount(1)
                    .setScissorCount(1);

            vk::PipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer
                    .setDepthBiasEnable(vk::False)
                    .setRasterizerDiscardEnable(vk::False)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eBack)
                    .setFrontFace(vk::FrontFace::eClockwise)
                    .setDepthBiasEnable(vk::False)
                    .setDepthBiasConstantFactor(0.0f)
                    .setDepthBiasClamp(0.0f)
                    .setDepthBiasSlopeFactor(0.0f)
                    .setLineWidth(1.0f);

            vk::PipelineMultisampleStateCreateInfo multisampling{};
            multisampling
                    .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                    .setSampleShadingEnable(vk::False);

            vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment
                    .setBlendEnable(vk::True)
                    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                    .setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                    .setAlphaBlendOp(vk::BlendOp::eAdd)
                    .setColorWriteMask(
                            {
                                    vk::ColorComponentFlagBits::eR |
                                    vk::ColorComponentFlagBits::eG |
                                    vk::ColorComponentFlagBits::eB |
                                    vk::ColorComponentFlagBits::eA
                            }
                            );

            vk::PipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending
                    .setLogicOp(vk::LogicOp::eCopy)
                    .setLogicOpEnable(vk::False)
                    .setAttachmentCount(1)
                    .setPAttachments(&colorBlendAttachment)
                    .setBlendConstants(vk::ArrayWrapper1D<float, 4>({0.0f, 0.0f, 0.0f, 0.0f}))
                    .setPNext(nullptr);

            const std::vector dynamicStates = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor
            };

            vk::PipelineDynamicStateCreateInfo dynamicState{};
            dynamicState
                    .setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
                    .setPDynamicStates(dynamicStates.data());

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo
                    .setSetLayoutCount(0)
                    .setPSetLayouts(nullptr)
                    .setPushConstantRangeCount(0)
                    .setPPushConstantRanges(nullptr)
                    .setPNext(nullptr);

            try {
                ctx->vulkanContext.pipelineLayout = VulkanResource<
                    vk::PipelineLayout>(
                        device.createPipelineLayout(pipelineLayoutInfo),
                        [device](const vk::PipelineLayout &py) {
                            device.destroyPipelineLayout(py);
                            std::cout << "[Vulkan 销毁信息]: 销毁管道布局(pipelineLayout)!\n";
                        }
                        );
            } catch (const vk::SystemError &err) {
                throw std::runtime_error(
                        "创建管道布局失败: " + std::string(err.what())
                        );
            }

            vk::GraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo
                    .setStageCount(2)
                    .setStages(shaderStages)
                    .setPVertexInputState(&vertexInputInfo)
                    .setPInputAssemblyState(&inputAssembly)
                    .setPViewportState(&viewportState)
                    .setPRasterizationState(&rasterizer)
                    .setPMultisampleState(&multisampling)
                    .setPColorBlendState(&colorBlending)
                    .setPDynamicState(&dynamicState)
                    .setLayout(ctx->vulkanContext.pipelineLayout.get())
                    .setRenderPass(ctx->vulkanContext.renderPass.get())
                    .setSubpass(0)
                    .setBasePipelineHandle(nullptr);

            try {
                ctx->vulkanContext.graphicsPipeline = VulkanResource<
                    vk::Pipeline>(
                        device.createGraphicsPipelines(nullptr, {pipelineInfo}).
                               value[0],
                        [device](const vk::Pipeline &p) {
                            device.destroyPipeline(p);
                            std::cout <<
                                    "[Vulkan 销毁信息]: 销毁图形管线(graphicsPipeline)!\n";
                        }
                        );
            } catch (const vk::SystemError &err) {
                throw std::runtime_error(
                        "创建图形管线失败: " + std::string(err.what())
                        );
            }

            return {ctx};
        }

        private:

            static auto readFile(
                    const std::string &filename
                    ) -> std::vector<char> {
                std::ifstream file{filename, std::ios::ate | std::ios::binary};
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
                    const std::vector<char> &code
                    ) -> VulkanResource<vk::ShaderModule> {
                const vk::ShaderModuleCreateInfo createInfo{
                        {},
                        code.size(),
                        reinterpret_cast<const uint32_t *>(code.data())};

                vk::ShaderModule handle = nullptr;

                try {
                    handle = device.createShaderModule(createInfo);
                } catch (const vk::SystemError &err) {
                    std::cerr << "[Vulkan 错误]: ShaderModule 创建失败: " << err.
                            what() << std::endl;
                }
                return {handle,
                        [device](const vk::ShaderModule &sm) {
                            device.destroyShaderModule(sm);
                            std::cout << "[Vulkan 销毁]: ShaderModule 已自动释放\n";
                        }};
            }
    };

    struct CreateVulkanRenderPass {
        std::shared_ptr<GlfwContext> ctx;

        [[nodiscard]] CreateVulkanGraphicsPipeline
        createVulkanRenderPass() const {
            vk::AttachmentDescription colorAttachment{};
            colorAttachment
                    .setFormat(ctx->vulkanContext.swapChainImageFormat)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

            vk::AttachmentReference colorAttachmentRef{};
            colorAttachmentRef
                    .setAttachment(0)
                    .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

            vk::SubpassDescription subpass{};
            subpass
                    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                    .setColorAttachmentCount(1)
                    .setPColorAttachments(&colorAttachmentRef);

            vk::RenderPassCreateInfo renderPassInfo{};
            renderPassInfo
                    .setAttachmentCount(1)
                    .setPAttachments(&colorAttachment)
                    .setSubpassCount(1)
                    .setPSubpasses(&subpass);

            try {
                const vk::Device device       = ctx->vulkanContext.device.get();
                ctx->vulkanContext.renderPass = VulkanResource<vk::RenderPass>(
                        device.createRenderPass(renderPassInfo, nullptr),
                        [device](const vk::RenderPass &rp) {
                            device.destroyRenderPass(rp);
                            std::cout << "[Vulkan 销毁信息]: 销毁渲染流程(renderPass)!\n";
                        }
                        );
            } catch (const vk::SystemError &err) {
                throw std::runtime_error(
                        "无法创建渲染通道: " + std::string(err.what())
                        );
            }
            return {ctx};
        }
    };
}
