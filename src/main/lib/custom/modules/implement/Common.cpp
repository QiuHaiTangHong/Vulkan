module;
// clang-format off
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// clang-format on
#include <functional>
#include <iostream>
#include <memory>

module CustomVulkan.Common;

namespace CustomVulkan {
    auto VulkanTools::getRequiredExtensions() -> std::vector<const char*> {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (VulkanSettings::enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }

    auto VulkanTools::populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) -> void {
        createInfo
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(debugCallback);
    }

    auto VulkanTools::findQueueFamilies(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
        -> QueueFamilyIndices {
        QueueFamilyIndices indices;
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }

            if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
        }

        return indices;
    }

    auto VulkanTools::debugCallback([[maybe_unused]] vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                    [[maybe_unused]] void* pUserData) -> vk::Bool32 {
        std::cerr << "[Vulkan 调试信息]: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }
} // namespace CustomVulkan
