module;
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

export module CustomVulkan.Swapchain;
import CustomVulkan.Common;
import CustomVulkan.Pipeline;
export namespace CustomVulkan {
    template <typename Tag>
    struct VulkanImageViews {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanImageViews() const -> VulkanRenderPass<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;

            [[nodiscard]] auto recreateVulkanImageViews() const -> VulkanRenderPass<IVulkanRecreate>
                requires std::is_same_v<Tag, IVulkanRecreate>;

        private:

            static void initVulkanImageViews(const std::shared_ptr<GlfwContext>& ctx);
    };

    template <typename Tag>
    struct VulkanSwapChain {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanSwapChain() const -> VulkanImageViews<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;

            [[nodiscard]] auto recreateSwapChain() const -> VulkanImageViews<IVulkanRecreate>
                requires std::is_same_v<Tag, IVulkanRecreate>;

        private:

            static void initSwapChain(const std::shared_ptr<GlfwContext>& ctx);

            // 查询交换链支持
            static SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

            // 选择交换链表面格式
            static vk::SurfaceFormatKHR
            chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

            // 选择交换链当前模式
            static vk::PresentModeKHR
            chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

            // 选择交换链范围
            static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    };
} // namespace CustomVulkan
