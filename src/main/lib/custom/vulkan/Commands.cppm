module;
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

export module CustomVulkan.Commands;
import CustomVulkan.Sync;
import CustomVulkan.Common;

export namespace CustomVulkan {
    struct CreateVulkanCommandBuffer {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanCommandBuffer() const -> CreateVulkanSyncObjects {
                const vk::CommandPool commandPool = ctx->vulkanContext.commandPool.get();
                const vk::Device device = ctx->vulkanContext.device.get();
                vk::CommandBufferAllocateInfo allocInfo{};
                allocInfo.commandPool = commandPool;
                allocInfo.level = vk::CommandBufferLevel::ePrimary;
                allocInfo.commandBufferCount = VulkanSettings::maxFramesInFlight;

                try {
                    const auto &buffers = device.allocateCommandBuffers(allocInfo);
                    ctx->vulkanContext.commandBuffer = VulkanResource<std::vector<vk::CommandBuffer>>(
                            buffers,
                            [device, pool = ctx->vulkanContext.commandPool.get()](const std::vector<vk::CommandBuffer> &bufs) {
                                if (!bufs.empty()) {
                                    device.freeCommandBuffers(pool, bufs);
                                }
                                std::cout << "[Vulkan 销毁信息]: 成功批量释放多帧命令缓冲区!\n";
                            });
                } catch (const std::exception &e) {
                    throw std::runtime_error("分配命令缓冲区失败: " + std::string(e.what()));
                }
                return {ctx};
            }
    };

    struct CreateVulkanCommandPool {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanCommandPool() const -> CreateVulkanCommandBuffer {
                const vk::PhysicalDevice physicalDevice = ctx->vulkanContext.physicalDevice;
                const vk::Device device = ctx->vulkanContext.device.get();
                const vk::SurfaceKHR surface = ctx->vulkanContext.surface.get();
                const auto [graphicsFamily, presentFamily] = VulkanTools::findQueueFamilies(physicalDevice, surface);

                vk::CommandPoolCreateInfo poolInfo{};
                poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
                poolInfo.queueFamilyIndex = graphicsFamily.value();

                try {
                    ctx->vulkanContext.commandPool = VulkanResource<vk::CommandPool>(
                            device.createCommandPool(poolInfo),
                            [device](const vk::CommandPool &cp) {
                                device.destroyCommandPool(cp);
                                std::cout << "[Vulkan 销毁信息]: 销毁命令池(commandPool)!\n";
                            });
                } catch (const std::exception &e) {
                    throw std::runtime_error("创建创建命令池失败: " + std::string(e.what()));
                }
                return {ctx};
            }
    };

    struct CreateVulkanFramebuffers {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanFramebuffers() const -> CreateVulkanCommandPool {
                const std::vector<vk::ImageView> swapChainImageViews = ctx->vulkanContext.swapChainImageViews.get();
                const auto swapChainExtent = ctx->vulkanContext.swapChainExtent;
                const auto device = ctx->vulkanContext.device.get();

                std::vector<vk::Framebuffer> framebuffers(swapChainImageViews.size());
                for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                    const vk::ImageView attachments[] = {
                            swapChainImageViews[i]};

                    vk::FramebufferCreateInfo framebufferInfo{};
                    framebufferInfo.renderPass = ctx->vulkanContext.renderPass.get();
                    framebufferInfo.attachmentCount = 1;
                    framebufferInfo.pAttachments = attachments;
                    framebufferInfo.width = swapChainExtent.width;
                    framebufferInfo.height = swapChainExtent.height;
                    framebufferInfo.layers = 1;

                    try {
                        framebuffers[i] = device.createFramebuffer(framebufferInfo);
                    } catch (const vk::SystemError &e) {
                        throw std::runtime_error("创建 Framebuffer 失败: " + std::string(e.what()));
                    }
                }
                ctx->vulkanContext.swapChainFramebuffers = VulkanResource<std::vector<vk::Framebuffer>>{
                        std::move(framebuffers),
                        [device](const std::vector<vk::Framebuffer> &fbs) {
                            for (const auto fb: fbs) {
                                device.destroyFramebuffer(fb);
                            }
                            std::cout << "[Vulkan 销毁信息]: 成功批量销毁 " << fbs.size() << " 个 swapChainFramebuffer!\n";
                        }};
                return {ctx};
            }
    };
}
