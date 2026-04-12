module;
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <filesystem>
#include <memory>
#include <string>

export module CustomVulkan.Context;

import CustomVulkan.Common;
import CustomVulkan.DeviceSelection;

export namespace CustomVulkan {
    template <typename Tag>
    struct VulkanSurface {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanSurface() const -> VulkanPhysicalDevice<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;
    };

    template <typename Tag>
    struct VulkanDebugMessenger {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto setupVulkanDebugMessenger() const -> VulkanSurface<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;
    };

    template <typename Tag>
    struct VulkanInstance {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanInstance() const -> VulkanDebugMessenger<IVulkanInit>
                requires std::is_same_v<Tag, IVulkanInit>;

        private:

            static auto setEnvironmentVariable(const std::string& name, std::filesystem::path path) -> void;

            // 检查验证层支持
            static auto checkValidationLayerSupport() -> bool;
    };

    auto CreateGlfwWindow(int width, int height, const char* title) -> VulkanInstance<IVulkanInit>;
} // namespace CustomVulkan
