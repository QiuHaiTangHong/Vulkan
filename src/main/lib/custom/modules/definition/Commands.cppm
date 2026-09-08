module;
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

export module CustomVulkan.Commands;
import CustomVulkan.Sync;
import CustomVulkan.Common;

export namespace CustomVulkan
{
    template <typename Tag>
    struct VulkanCommandBuffer
    {
        std::shared_ptr<GlfwContext> ctx;

        [[nodiscard]] auto createVulkanCommandBuffer() const -> VulkanSyncObjects<IVulkanInit> requires std::is_same_v<
            Tag, IVulkanInit>;

        auto recreateVulkanCommandBuffer() const -> void requires std::is_same_v<Tag, IVulkanRecreate>;

    private:
        static void initVulkanCommandBuffer(const std::shared_ptr<GlfwContext>& ctx);
    };

    struct VertexTool
    {
        glm::vec2 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription{};
            bindingDescription.setBinding(0).setStride(sizeof(VertexTool)).setInputRate(vk::VertexInputRate::eVertex);
            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 2> attributeDescription{};
            attributeDescription[0]
                .setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32Sfloat);
            attributeDescription[1]
                .setBinding(0)
                .setLocation(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(offsetof(VertexTool, color));
            return attributeDescription;
        }
    };

    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const std::vector<VertexTool> vertices = {
        {
            {0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}
        },
        {
            {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}
        },
        {
            {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}
        }
    };

    template <typename Tag>
    struct VulkanVertexBuffer
    {
        std::shared_ptr<GlfwContext> ctx;
        [[nodiscard]] auto createVertexBuffer() const -> VulkanCommandBuffer<IVulkanInit> requires std::is_same_v<
            Tag, IVulkanInit>;

    private:
        [[nodiscard]] auto findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const -> uint32_t;
    };

    template <typename Tag>
    struct VulkanCommandPool
    {
        std::shared_ptr<GlfwContext> ctx;

        [[nodiscard]] auto createVulkanCommandPool() const -> VulkanVertexBuffer<IVulkanInit> requires std::is_same_v<
            Tag, IVulkanInit>;

        [[nodiscard]] auto cleanVulkanCommandPool() const -> VulkanCommandBuffer<IVulkanRecreate> requires
            std::is_same_v<Tag, IVulkanRecreate>;

    private:
        static void initVulkanCommandPool(const std::shared_ptr<GlfwContext>& ctx);
    };

    template <typename Tag>
    struct VulkanFramebuffers
    {
        std::shared_ptr<GlfwContext> ctx;

        [[nodiscard]] auto createVulkanFramebuffers() const -> VulkanCommandPool<IVulkanInit> requires std::is_same_v<
            Tag, IVulkanInit>;

        [[nodiscard]] auto recreateVulkanFramebuffers() const -> VulkanCommandPool<IVulkanRecreate> requires
            std::is_same_v<Tag, IVulkanRecreate>;

    private:
        static void initVulkanFramebuffers(const std::shared_ptr<GlfwContext>& ctx);
    };
} // namespace CustomVulkan
