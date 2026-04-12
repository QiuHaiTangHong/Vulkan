module;
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

module CustomVulkan.DeviceSelection;

namespace CustomVulkan {
    template <typename Tag>
    [[nodiscard]] auto VulkanLogicalDevice<Tag>::createVulkanLogicalDevice() const -> VulkanSwapChain<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        const auto [graphicsFamily, presentFamily] =
            VulkanTools::findQueueFamilies(ctx->vulkanContext.physicalDevice, ctx->vulkanContext.surface.get());

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        const std::set uniqueQueueFamilies = {graphicsFamily.value(), presentFamily.value()};

        auto queuePriority = 1.0f;
        queueCreateInfos.reserve(uniqueQueueFamilies.size());
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            queueCreateInfos.push_back({{}, queueFamily, 1, &queuePriority});
        }

        vk::PhysicalDeviceFeatures deviceFeatures{};

        const vk::DeviceCreateInfo createInfo{{},
                                              static_cast<uint32_t>(queueCreateInfos.size()),
                                              queueCreateInfos.data(),
                                              0,
                                              nullptr,
                                              static_cast<uint32_t>(VulkanSettings::deviceExtensions.size()),
                                              std::begin(VulkanSettings::deviceExtensions),
                                              &deviceFeatures};

        ctx->vulkanContext.device =
            VulkanResource<vk::Device>(ctx->vulkanContext.physicalDevice.createDevice(createInfo),
                                       [](const VkDevice d) { // NOLINT(*-misplaced-const)
                                           vkDestroyDevice(d, nullptr);
                                           std::cout << "[Vulkan 销毁信息]: 销毁逻辑设备(device)!\n";
                                       });
        ctx->vulkanContext.graphicsQueue = ctx->vulkanContext.device.get().getQueue(graphicsFamily.value(), 0);
        ctx->vulkanContext.presentQueue = ctx->vulkanContext.device.get().getQueue(presentFamily.value(), 0);
        return {ctx};
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanPhysicalDevice<Tag>::pickVulkanPhysicalDevice() const -> VulkanLogicalDevice<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        const std::vector<vk::PhysicalDevice> devices = ctx->vulkanContext.instance.get().enumeratePhysicalDevices();
        if (devices.empty()) {
            throw std::runtime_error("找不到支持 Vulkan 的GPU!");
        }

        // 检查物理设备, 并设置使用第一个物理设备
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                ctx->vulkanContext.physicalDevice = device;
                break;
            }
        }

        if (ctx->vulkanContext.physicalDevice == nullptr) {
            throw std::runtime_error("没有找到可用的 GPU!");
        }
        return {ctx};
    }

    template <typename Tag>
    bool VulkanPhysicalDevice<Tag>::isDeviceSuitable(const VkPhysicalDevice device) const { // NOLINT(*-misplaced-const)
        return VulkanTools::findQueueFamilies(device, ctx->vulkanContext.surface.get()).isComplete();
    }

    template struct VulkanPhysicalDevice<IVulkanInit>;
    template struct VulkanLogicalDevice<IVulkanInit>;

} // namespace CustomVulkan
