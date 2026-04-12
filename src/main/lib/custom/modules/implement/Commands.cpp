module;
#include <iostream>
#include <vulkan/vulkan.hpp>

module CustomVulkan.Commands;

namespace CustomVulkan
{
    template <typename Tag>
    [[nodiscard]] auto VulkanCommandBuffer<Tag>::createVulkanCommandBuffer() const -> VulkanSyncObjects<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        initVulkanCommandBuffer(this->ctx);
        return {ctx};
    }

    template <typename Tag>
    auto VulkanCommandBuffer<Tag>::recreateVulkanCommandBuffer() const -> void requires std::is_same_v<
        Tag, IVulkanRecreate>
    {
        initVulkanCommandBuffer(this->ctx);
    }

    template <typename Tag>
    void VulkanCommandBuffer<Tag>::initVulkanCommandBuffer(const std::shared_ptr<GlfwContext>& ctx)
    {
        const vk::CommandPool commandPool = ctx->vulkanContext.commandPool.get();
        const vk::Device device = ctx->vulkanContext.device.get();
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandPool(commandPool)
                 .setLevel(vk::CommandBufferLevel::ePrimary)
                 .setCommandBufferCount(VulkanSettings::maxFramesInFlight);

        try
        {
            const auto& buffers = device.allocateCommandBuffers(allocInfo);
            ctx->vulkanContext.commandBuffer = VulkanResource<std::vector<vk::CommandBuffer>>(
                buffers,
                [device, pool = ctx->vulkanContext.commandPool.get()](const std::vector<vk::CommandBuffer>& bufs)
                {
                    if (!bufs.empty())
                    {
                        device.freeCommandBuffers(pool, bufs);
                    }
                    std::cout << "[Vulkan 销毁信息]: 成功批量释放命令缓冲区(commandBuffer)!\n";
                });
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("分配命令缓冲区失败: " + std::string(e.what()));
        }
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanVertexBuffer<Tag>::createVertexBuffer() const -> VulkanCommandBuffer<IVulkanInit> requires
        std::is_same_v<Tag, IVulkanInit>
    {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo
            .setSize(sizeof(vertices[0]) * vertices.size())
            .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
            .setSharingMode(vk::SharingMode::eExclusive);
        const auto& device = ctx->vulkanContext.device.get();
        try
        {
            ctx->vulkanContext.vertexBuffer = VulkanResource<vk::Buffer>(
                device.createBuffer(bufferInfo),
                [device](const vk::Buffer& buffer)
                {
                    device.destroyBuffer(buffer);
                    std::cout << "[Vulkan 销毁信息]: 销毁顶点缓冲(vertexBuffer)!\n";
                }
            );
        }
        catch (const vk::SystemError& err)
        {
            throw std::runtime_error("[Vulkan 错误信息] 无法创建顶点缓冲区!" + std::string(err.what()));
        }
        const vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(
            ctx->vulkanContext.vertexBuffer.get());
        vk::MemoryAllocateInfo allocateInfo{};
        allocateInfo
            .setAllocationSize(memoryRequirements.size)
            .setMemoryTypeIndex(
                findMemoryType(memoryRequirements.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
            );
        try
        {
            ctx->vulkanContext.vertexBufferMemory = VulkanResource<vk::DeviceMemory>(
                device.allocateMemory(allocateInfo),
                [device](const vk::DeviceMemory& dm)
                {
                    device.freeMemory(dm);
                    std::cout << "[Vulkan 释放信息]: 释放顶点缓冲内存(vertexBufferMemory)!\n";
                }
            );
        }
        catch (const vk::SystemError& err)
        {
            throw std::runtime_error("[Vulkan 错误信息] 分配顶点内存失败!" + std::string(err.what()));
        }
        const auto vertexBufferMemory = ctx->vulkanContext.vertexBufferMemory.get();
        device.bindBufferMemory(
            ctx->vulkanContext.vertexBuffer.get(),
            ctx->vulkanContext.vertexBufferMemory.get(),
            0
        );
        const auto data = device.mapMemory(vertexBufferMemory, 0, bufferInfo.size);
        memcpy(data, vertices.data(), bufferInfo.size);
        device.unmapMemory(vertexBufferMemory);
        return {ctx};
    }

    template <typename Tag>
    auto VulkanVertexBuffer<Tag>::findMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags properties)
    const -> uint32_t
    {
        const vk::PhysicalDeviceMemoryProperties memoryProperties = ctx->vulkanContext.physicalDevice.
                                                                         getMemoryProperties();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("未能找到合适的内存类型!");
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanCommandPool<Tag>::createVulkanCommandPool() const -> VulkanVertexBuffer<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        initVulkanCommandPool(this->ctx);
        return {ctx};
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanCommandPool<Tag>::cleanVulkanCommandPool() const -> VulkanCommandBuffer<IVulkanRecreate>
        requires std::is_same_v<Tag, IVulkanRecreate>
    {
        ctx->vulkanContext.device.get().resetCommandPool(ctx->vulkanContext.commandPool.get());

        return {ctx};
    }

    template <typename Tag>
    void VulkanCommandPool<Tag>::initVulkanCommandPool(const std::shared_ptr<GlfwContext>& ctx)
    {
        const vk::PhysicalDevice physicalDevice = ctx->vulkanContext.physicalDevice;
        const vk::Device device = ctx->vulkanContext.device.get();
        const vk::SurfaceKHR surface = ctx->vulkanContext.surface.get();
        const auto [graphicsFamily, presentFamily] = VulkanTools::findQueueFamilies(physicalDevice, surface);

        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = graphicsFamily.value();

        try
        {
            ctx->vulkanContext.commandPool = VulkanResource<vk::CommandPool>(
                device.createCommandPool(poolInfo), [device](const vk::CommandPool& cp)
                {
                    device.destroyCommandPool(cp);
                    std::cout << "[Vulkan 销毁信息]: 销毁命令池(commandPool)!\n";
                });
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("创建创建命令池失败: " + std::string(e.what()));
        }
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanFramebuffers<Tag>::createVulkanFramebuffers() const -> VulkanCommandPool<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        initVulkanFramebuffers(this->ctx);
        return {ctx};
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanFramebuffers<Tag>::recreateVulkanFramebuffers() const -> VulkanCommandPool<IVulkanRecreate>
        requires std::is_same_v<Tag, IVulkanRecreate>
    {
        initVulkanFramebuffers(this->ctx);
        return {ctx};
    }

    template <typename Tag>
    void VulkanFramebuffers<Tag>::initVulkanFramebuffers(const std::shared_ptr<GlfwContext>& ctx)
    {
        const std::vector<vk::ImageView> swapChainImageViews = ctx->vulkanContext.swapChainImageViews.get();
        const auto swapChainExtent = ctx->vulkanContext.swapChainExtent;
        const auto device = ctx->vulkanContext.device.get();

        std::vector<vk::Framebuffer> framebuffers(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            const vk::ImageView attachments[] = {swapChainImageViews[i]};

            vk::FramebufferCreateInfo framebufferInfo{};
            framebufferInfo.setRenderPass(ctx->vulkanContext.renderPass.get())
                           .setAttachmentCount(1)
                           .setPAttachments(attachments)
                           .setWidth(swapChainExtent.width)
                           .setHeight(swapChainExtent.height)
                           .setLayers(1);

            try
            {
                framebuffers[i] = device.createFramebuffer(framebufferInfo);
            }
            catch (const vk::SystemError& e)
            {
                throw std::runtime_error("创建 Framebuffer 失败: " + std::string(e.what()));
            }
        }
        ctx->vulkanContext.swapChainFramebuffers = VulkanResource<std::vector<vk::Framebuffer>>{
            std::move(framebuffers), [device](const std::vector<vk::Framebuffer>& fbs)
            {
                for (const auto fb : fbs)
                {
                    device.destroyFramebuffer(fb);
                }
                std::cout << "[Vulkan 销毁信息]: 成功批量销毁 " << fbs.size() << " 个 swapChainFramebuffer!\n";
            }
        };
    }

    template struct VulkanSyncObjects<IVulkanInit>;
    template struct VulkanCommandBuffer<IVulkanInit>;
    template struct VulkanCommandBuffer<IVulkanRecreate>;
    template struct VulkanVertexBuffer<IVulkanInit>;
    template struct VulkanCommandPool<IVulkanInit>;
    template struct VulkanCommandPool<IVulkanRecreate>;
    template struct VulkanFramebuffers<IVulkanInit>;
    template struct VulkanFramebuffers<IVulkanRecreate>;
} // namespace CustomVulkan
