module;
#include <memory>
#include <vulkan/vulkan.hpp>

export module CustomVulkan.Encapsulation;

import CustomVulkan.Common;
import CustomVulkan.Sync;
import CustomVulkan.Context;
import CustomVulkan.Swapchain;
import CustomVulkan.Commands;

export namespace CustomVulkan {
    class VulkanInit {
        public:

            static void run();

        private:

            static void mainLoop(const std::shared_ptr<GlfwContext>& ctx);

            static void drawFrame(const std::shared_ptr<GlfwContext>& ctx);

            static void recordCommandBuffer(const std::shared_ptr<GlfwContext>& ctx, uint32_t imageIndex);

            static void recreateSwapChain(const std::shared_ptr<GlfwContext>& ctx);
    };
} // namespace CustomVulkan
