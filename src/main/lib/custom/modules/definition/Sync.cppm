module;
#include <iostream>
#include <memory>
export module CustomVulkan.Sync;

import CustomVulkan.Common;

export namespace CustomVulkan {
    template <typename Tag>
    struct VulkanSyncObjects {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanSyncObjects() const -> std::shared_ptr<GlfwContext>
                requires std::is_same_v<Tag, IVulkanInit>;
    };
} // namespace CustomVulkan
