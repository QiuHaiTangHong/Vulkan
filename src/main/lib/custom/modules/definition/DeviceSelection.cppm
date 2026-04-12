module;
#include <memory>
#include <vulkan/vulkan.hpp>

export module CustomVulkan.DeviceSelection;

import CustomVulkan.Common;
import CustomVulkan.Swapchain;

export namespace CustomVulkan {
    template <typename Tag>
    struct VulkanLogicalDevice {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanLogicalDevice() const -> VulkanSwapChain<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;
    };

    template <typename Tag>
    struct VulkanPhysicalDevice {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto pickVulkanPhysicalDevice() const -> VulkanLogicalDevice<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;

        private:

            bool isDeviceSuitable(const VkPhysicalDevice device) const;
    };
} // namespace CustomVulkan
