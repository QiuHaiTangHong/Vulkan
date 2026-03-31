module;
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <array>
#include <string>

export module CustomVulkan.Encapsulation;

import CustomVulkan.Common;
import CustomVulkan.Sync;
import CustomVulkan.Context;

export namespace CustomVulkan {
    class VulkanInit {
        public:

            static void run() {
                const auto finalContext = CreateGlfwWindow(
                                                  VulkanSettings::width,
                                                  VulkanSettings::height,
                                                  "Vulkan Engine"
                                                  )
                                          .createVulkanInstance()
                                          .setupVulkanDebugMessenger()
                                          .createVulkanSurface()
                                          .pickVulkanPhysicalDevice()
                                          .createVulkanLogicalDevice()
                                          .createVulkanSwapChain()
                                          .createVulkanImageViews()
                                          .createVulkanRenderPass()
                                          .createVulkanGraphicsPipeline()
                                          .createVulkanFramebuffers()
                                          .createVulkanCommandPool()
                                          .createVulkanCommandBuffer()
                                          .createVulkanSyncObjects();

                // 进入主循环
                mainLoop(finalContext);
            }

        private:

            static void mainLoop(const std::shared_ptr<GlfwContext> &ctx) {
                while (!glfwWindowShouldClose(ctx->window.get())) {
                    glfwPollEvents();
                    drawFrame(ctx);
                }
                ctx->vulkanContext.device.get().waitIdle();
            }

            static void drawFrame(const std::shared_ptr<GlfwContext> &ctx) {
                const auto &device = ctx->vulkanContext.device.get();
                const auto &graphicsQueue = ctx->vulkanContext.graphicsQueue;
                const auto &presentQueue = ctx->vulkanContext.presentQueue;
                const auto &swapChain = ctx->vulkanContext.swapChain.get();
                const auto &inFlightFence = ctx->vulkanContext.inFlightFences.get()[ctx->vulkanContext.currentFrame];
                const auto &imageAvailableSemaphore = ctx->vulkanContext.imageAvailableSemaphores.get()[ctx->
                    vulkanContext.currentFrame];
                const auto &currentCommandBuffer = ctx->vulkanContext.commandBuffer.get()[ctx->vulkanContext.
                    currentFrame];

                if (device.waitForFences(1, &inFlightFence, true, UINT64_MAX) != vk::Result::eSuccess) {
                    throw std::runtime_error("等待 Fence 超时或失败！");
                }

                uint32_t imageIndex{};
                try {
                    const auto acquireResult = device.acquireNextImageKHR(
                            swapChain,
                            UINT64_MAX,
                            imageAvailableSemaphore,
                            nullptr,
                            &imageIndex
                            );
                    if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
                        return;
                    }
                    if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR) {
                        throw std::runtime_error("获取交换链图像失败！");
                    }
                } catch ([[maybe_unused]] const vk::OutOfDateKHRError &err) {
                    return;
                }

                if (const auto &resetResult = device.resetFences(1, &inFlightFence);
                    resetResult != vk::Result::eSuccess) {
                    throw std::runtime_error("无法重置 Fence 原因: " + to_string(resetResult));
                }

                recordCommandBuffer(ctx, imageIndex);
                const auto &renderFinishedSemaphore = ctx->vulkanContext.renderFinishedSemaphores.get()[imageIndex];

                vk::SubmitInfo submitInfo{};
                const vk::Semaphore waitSemaphores[]          = {imageAvailableSemaphore};
                constexpr vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
                submitInfo.setWaitSemaphores(waitSemaphores);
                submitInfo.setWaitDstStageMask(waitStages);

                submitInfo.setCommandBuffers(currentCommandBuffer);

                const vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore};
                submitInfo.setSignalSemaphores(signalSemaphores);

                graphicsQueue.submit(submitInfo, inFlightFence);

                vk::PresentInfoKHR presentInfo{};
                presentInfo.setWaitSemaphores(signalSemaphores);

                const vk::SwapchainKHR swapChains[] = {swapChain};
                presentInfo.setSwapchains(swapChains);
                presentInfo.setImageIndices(imageIndex);

                try {
                    if (const auto &presentResult = presentQueue.presentKHR(presentInfo);
                        presentResult == vk::Result::eSuboptimalKHR) {
                    }
                    ctx->vulkanContext.currentFrame =
                            (ctx->vulkanContext.currentFrame + 1) % VulkanSettings::maxFramesInFlight;
                } catch ([[maybe_unused]] const vk::OutOfDateKHRError &err) {

                } catch (const vk::SystemError &err) {
                    throw std::runtime_error("呈现时发生系统错误: " + std::string(err.what()));
                }
            }

            static void recordCommandBuffer(const std::shared_ptr<GlfwContext> &ctx, const uint32_t imageIndex) {
                const auto &commandBuffer = ctx->vulkanContext.commandBuffer.get()[ctx->vulkanContext.currentFrame];
                commandBuffer.reset();
                const auto &swapChainFramebuffers = ctx->vulkanContext.swapChainFramebuffers.get();
                const auto &swapChainExtent       = ctx->vulkanContext.swapChainExtent;
                const auto &graphicsPipeline      = ctx->vulkanContext.graphicsPipeline.get();
                const auto &renderPass            = ctx->vulkanContext.renderPass.get();

                constexpr vk::CommandBufferBeginInfo allocInfo{};
                try {
                    commandBuffer.begin(allocInfo);
                } catch (const vk::SystemError &err) {
                    throw std::runtime_error("未能开始记录命令缓冲区: " + std::string(err.what()));
                }
                vk::RenderPassBeginInfo renderPassInfo{};
                renderPassInfo.renderPass        = renderPass;
                renderPassInfo.framebuffer       = swapChainFramebuffers[imageIndex];
                renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
                renderPassInfo.renderArea.extent = swapChainExtent;

                constexpr vk::ClearValue clearColor = vk::ClearColorValue(std::array{0.0f, 0.0f, 0.0f, 0.0f});
                renderPassInfo.clearValueCount      = 1;
                renderPassInfo.pClearValues         = &clearColor;

                commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

                vk::Viewport viewport{};
                viewport.x        = 0.0f;
                viewport.y        = 0.0f;
                viewport.width    = static_cast<float>(swapChainExtent.width);
                viewport.height   = static_cast<float>(swapChainExtent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                commandBuffer.setViewport(0, 1, &viewport);

                vk::Rect2D scissor{};
                scissor.offset = vk::Offset2D{0, 0};
                scissor.extent = swapChainExtent;
                commandBuffer.setScissor(0, 1, &scissor);

                commandBuffer.draw(3, 1, 0, 0);

                commandBuffer.endRenderPass();

                try {
                    commandBuffer.end();
                } catch (const vk::SystemError &err) {
                    throw std::runtime_error("结束录制命令缓冲区失败: " + std::string(err.what()));
                }
            }
    };
} // namespace CustomVulkan
