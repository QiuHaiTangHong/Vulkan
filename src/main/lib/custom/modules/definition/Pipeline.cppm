module;
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
export module CustomVulkan.Pipeline;
import CustomVulkan.Common;
import CustomVulkan.Commands;
export namespace CustomVulkan {
    template <typename Tag>
    struct VulkanGraphicsPipeline {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanGraphicsPipeline() const -> VulkanFramebuffers<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;

            [[nodiscard]] auto recreateVulkanGraphicsPipeline() const -> VulkanFramebuffers<IVulkanRecreate>
                requires std::is_same_v<Tag, IVulkanRecreate>;

        private:

            static void initVulkanGraphicsPipeline(const std::shared_ptr<GlfwContext>& ctx);

            static auto readFile(const std::string& filename) -> std::vector<char>;

            static auto createShaderModule(const vk::Device& device, const std::vector<char>& code)
                -> VulkanResource<vk::ShaderModule>;
    };

    template <typename Tag>
    struct VulkanRenderPass {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanRenderPass() const -> VulkanGraphicsPipeline<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;

            [[nodiscard]] auto recreateVulkanRenderPass() const -> VulkanGraphicsPipeline<IVulkanRecreate>
                requires std::is_same_v<Tag, IVulkanRecreate>;

        private:

            static void initVulkanRenderPass(const std::shared_ptr<GlfwContext>& ctx);
    };
} // namespace CustomVulkan
