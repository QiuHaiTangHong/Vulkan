module;
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

export module CustomVulkan.Context;

import CustomVulkan.Common;
import CustomVulkan.DeviceSelection;

export namespace CustomVulkan {
    struct CreateVulkanSurface {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] PickVulkanPhysicalDevice createVulkanSurface() const {
                VkSurfaceKHR tempSurface;
                if (glfwCreateWindowSurface(
                            ctx->vulkanContext.instance.get(),
                            ctx->window.get(),
                            nullptr,
                            &tempSurface) != VK_SUCCESS) {
                    throw std::runtime_error("创建窗口表面失败!");
                }
                vk::Instance instHandle = ctx->vulkanContext.instance.get();
                ctx->vulkanContext.surface = VulkanResource<vk::SurfaceKHR>(
                        tempSurface,
                        [instHandle](const vk::SurfaceKHR s) {
                            if (instHandle != nullptr && s != nullptr) {
                                instHandle.destroySurfaceKHR(s);
                            }
                            std::cout << "[Vulkan 销毁信息]: 销毁窗口表面(surface)!\n";
                        });
                return {ctx};
            }
    };

    struct SetupVulkanDebugMessenger {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] CreateVulkanSurface
                    setupVulkanDebugMessenger() const {
                if constexpr (!VulkanSettings::enableValidationLayers) {
                    // ReSharper disable once CppDFAUnreachableCode
                    return {ctx};
                }
                vk::DebugUtilsMessengerCreateInfoEXT createInfo;
                VulkanTools::populateDebugMessengerCreateInfo(createInfo);
                vk::Instance instance = ctx->vulkanContext.instance.get();
                ctx->vulkanContext.debugMessenger =
                        VulkanResource<vk::DebugUtilsMessengerEXT>(
                                instance.createDebugUtilsMessengerEXT(
                                        createInfo),
                                [instance](const vk::DebugUtilsMessengerEXT m) {
                                    instance.destroyDebugUtilsMessengerEXT(m);
                                });
                return {ctx};
            }
    };

    struct CreateVulkanInstance {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] SetupVulkanDebugMessenger
                    createVulkanInstance() const {
                VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
                // ReSharper disable once CppRedundantBooleanExpressionArgument
                if (VulkanSettings::enableValidationLayers &&
                    !checkValidationLayerSupport()) {
                    throw std::runtime_error("已请求验证层, 但不可用!");
                }

                constexpr vk::ApplicationInfo appInfo{
                        "Hello Triangle",
                        vk::makeVersion(1, 0, 0),
                        "No Engine",
                        vk::makeVersion(1, 0, 0),
                        vk::makeApiVersion(0, 1, 0, 0)};

                auto extensions = VulkanTools::getRequiredExtensions();
                vk::InstanceCreateInfo createInfo{};
                createInfo.setPApplicationInfo(&appInfo);
                createInfo.setEnabledExtensionCount(
                        static_cast<uint32_t>(extensions.size()));
                createInfo.setPEnabledExtensionNames(extensions);

                vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
                if (VulkanSettings::enableValidationLayers) {
                    createInfo.enabledLayerCount = static_cast<uint32_t>(
                            VulkanSettings::validationLayers.size());
                    createInfo.ppEnabledLayerNames =
                            std::begin(VulkanSettings::validationLayers);
                    VulkanTools::populateDebugMessengerCreateInfo(
                            debugCreateInfo);
                    createInfo.pNext = reinterpret_cast<
                            VkDebugUtilsMessengerCreateInfoEXT *>(
                            &debugCreateInfo);
                }

                try {
                    const vk::Instance instance =
                            vk::createInstance(createInfo);
                    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
                    ctx->vulkanContext.instance =
                            VulkanResource<vk::Instance>(
                                    instance,
                                    [](const vk::Instance i) {
                                        vkDestroyInstance(i, nullptr);
                                        std::cout << "[Vulkan 销毁信息]: 销毁 Vulkan 实例(instance)!\n";
                                    });
                } catch (const vk::SystemError &err) {
                    throw std::runtime_error(
                            std::string("创建实例失败: ") + err.what());
                }

                return {ctx};
            }

        private:
            // 检查验证层支持
            static bool checkValidationLayerSupport() {
                const auto layerProperties = vk::enumerateInstanceLayerProperties();
                for (const char *layerName: VulkanSettings::validationLayers) {
                    bool layerFound = false;
                    for (const auto &layerProperty: layerProperties) {
                        if (strcmp(
                                    layerName,
                                    layerProperty.layerName) == 0) {
                            layerFound = true;
                            break;
                        }
                    }

                    if (!layerFound) {
                        return false;
                    }
                }
                return true;
            }
    };

    inline CreateVulkanInstance CreateGlfwWindow(
            const int width,
            const int height,
            const char *title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        const auto ctx = std::make_shared<GlfwContext>();
        ctx->window.reset(
                glfwCreateWindow(width, height, title, nullptr, nullptr));
        return {ctx};
    }
}