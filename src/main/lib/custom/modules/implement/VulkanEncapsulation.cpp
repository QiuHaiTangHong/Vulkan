module;
#include <GLFW/glfw3.h>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

module CustomVulkan.Encapsulation;

namespace CustomVulkan
{
    void VulkanInit::run()
    {
        const auto finalContext = CreateGlfwWindow(VulkanSettings::width, VulkanSettings::height, "Vulkan Engine")
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
                                  .createVertexBuffer()
                                  .createVulkanCommandBuffer()
                                  .createVulkanSyncObjects();

        // 进入主循环
        mainLoop(finalContext);
    }

    void VulkanInit::mainLoop(const std::shared_ptr<GlfwContext>& ctx)
    {
        while (!glfwWindowShouldClose(ctx->window.get()))
        {
            glfwPollEvents();
            drawFrame(ctx);
        }
        ctx->vulkanContext.device.get().waitIdle();
    }

    void VulkanInit::drawFrame(const std::shared_ptr<GlfwContext>& ctx)
    {
        const auto& device = ctx->vulkanContext.device.get();
        const auto& graphicsQueue = ctx->vulkanContext.graphicsQueue;
        const auto& presentQueue = ctx->vulkanContext.presentQueue;
        const auto& swapChain = ctx->vulkanContext.swapChain.get();
        const auto& inFlightFence = ctx->vulkanContext.inFlightFences.get()[ctx->vulkanContext.currentFrame];
        const auto& imageAvailableSemaphore =
            ctx->vulkanContext.imageAvailableSemaphores.get()[ctx->vulkanContext.currentFrame];
        const auto& currentCommandBuffer = ctx->vulkanContext.commandBuffer.get()[ctx->vulkanContext.currentFrame];

        if (device.waitForFences(1, &inFlightFence, true, UINT64_MAX) != vk::Result::eSuccess)
        {
            throw std::runtime_error("等待 Fence 超时或失败！");
        }

        uint32_t imageIndex{};
        try
        {
            const auto acquireResult =
                device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, nullptr, &imageIndex);
            if (acquireResult == vk::Result::eErrorOutOfDateKHR)
            {
                recreateSwapChain(ctx);
                return;
            }
            if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
            {
                throw std::runtime_error("获取交换链图像失败！");
            }
        }
        catch ([[maybe_unused]] const vk::OutOfDateKHRError& err)
        {
            recreateSwapChain(ctx);
            return;
        }

        if (const auto& resetResult = device.resetFences(1, &inFlightFence); resetResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("无法重置 Fence 原因: " + to_string(resetResult));
        }

        recordCommandBuffer(ctx, imageIndex);
        const auto& renderFinishedSemaphore = ctx->vulkanContext.renderFinishedSemaphores.get()[imageIndex];

        vk::SubmitInfo submitInfo{};
        const vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore};
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

        try
        {
            if (const auto& presentResult = presentQueue.presentKHR(presentInfo);
                presentResult == vk::Result::eSuboptimalKHR || presentResult == vk::Result::eErrorOutOfDateKHR ||
                ctx->vulkanContext.framebufferResized)
            {
                ctx->vulkanContext.framebufferResized = false;
                recreateSwapChain(ctx);
            }
            ctx->vulkanContext.currentFrame = (ctx->vulkanContext.currentFrame + 1) % VulkanSettings::maxFramesInFlight;
        }
        catch ([[maybe_unused]] const vk::OutOfDateKHRError& err)
        {
            ctx->vulkanContext.framebufferResized = false;
            recreateSwapChain(ctx);
        }
        catch (const vk::SystemError& err)
        {
            throw std::runtime_error("呈现时发生系统错误: " + std::string(err.what()));
        }
    }

    void VulkanInit::recordCommandBuffer(const std::shared_ptr<GlfwContext>& ctx, const uint32_t imageIndex)
    {
        const auto& commandBuffer = ctx->vulkanContext.commandBuffer.get()[ctx->vulkanContext.currentFrame];
        commandBuffer.reset();
        const auto& swapChainFramebuffers = ctx->vulkanContext.swapChainFramebuffers.get();
        const auto& swapChainExtent = ctx->vulkanContext.swapChainExtent;
        const auto& graphicsPipeline = ctx->vulkanContext.graphicsPipeline.get();
        const auto& renderPass = ctx->vulkanContext.renderPass.get();

        constexpr vk::CommandBufferBeginInfo allocInfo{};
        try
        {
            commandBuffer.begin(allocInfo);
        }
        catch (const vk::SystemError& err)
        {
            throw std::runtime_error("未能开始记录命令缓冲区: " + std::string(err.what()));
        }
        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        constexpr vk::ClearValue clearColor = vk::ClearColorValue(std::array{0.0f, 0.0f, 0.0f, 0.0f});
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        const vk::Buffer vertexBuffer = ctx->vulkanContext.vertexBuffer.get();
        vk::DeviceSize offsets[] = {0};
        commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.setViewport(0, 1, &viewport);
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = swapChainExtent;
        commandBuffer.setScissor(0, 1, &scissor);
        commandBuffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);

        commandBuffer.endRenderPass();

        try
        {
            commandBuffer.end();
        }
        catch (const vk::SystemError& err)
        {
            throw std::runtime_error("结束录制命令缓冲区失败: " + std::string(err.what()));
        }
    }

    void VulkanInit::recreateSwapChain(const std::shared_ptr<GlfwContext>& ctx)
    {
        auto width = 0, height = 0;
        glfwGetFramebufferSize(ctx->window.get(), &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(ctx->window.get(), &width, &height);
            glfwWaitEvents();
        }

        ctx->vulkanContext.device.get().waitIdle();

        VulkanSwapChain<IVulkanRecreate>{ctx}
            .recreateSwapChain()
            .recreateVulkanImageViews()
            .recreateVulkanRenderPass()
            .recreateVulkanGraphicsPipeline()
            .recreateVulkanFramebuffers()
            .cleanVulkanCommandPool()
            .recreateVulkanCommandBuffer();
    }
} // namespace CustomVulkan
