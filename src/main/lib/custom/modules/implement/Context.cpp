module;
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

module CustomVulkan.Context;

namespace CustomVulkan {
    template <typename Tag>
    [[nodiscard]] auto VulkanSurface<Tag>::createVulkanSurface() const -> VulkanPhysicalDevice<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        VkSurfaceKHR tempSurface;
        if (glfwCreateWindowSurface(ctx->vulkanContext.instance.get(), ctx->window.get(), nullptr, &tempSurface) !=
            VK_SUCCESS) {
            throw std::runtime_error("创建窗口表面失败!");
        }
        vk::Instance instHandle = ctx->vulkanContext.instance.get();
        ctx->vulkanContext.surface = VulkanResource<vk::SurfaceKHR>(tempSurface, [instHandle](const vk::SurfaceKHR s) {
            if (instHandle != nullptr && s != nullptr) {
                instHandle.destroySurfaceKHR(s);
            }
            std::cout << "[Vulkan 销毁信息]: 销毁窗口表面(surface)!\n";
        });
        return {ctx};
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanDebugMessenger<Tag>::setupVulkanDebugMessenger() const -> VulkanSurface<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        if constexpr (!VulkanSettings::enableValidationLayers) {
            // ReSharper disable once CppDFAUnreachableCode
            return {ctx};
        }
        vk::DebugUtilsMessengerCreateInfoEXT createInfo;
        VulkanTools::populateDebugMessengerCreateInfo(createInfo);
        vk::Instance instance = ctx->vulkanContext.instance.get();
        ctx->vulkanContext.debugMessenger = VulkanResource<vk::DebugUtilsMessengerEXT>(
            instance.createDebugUtilsMessengerEXT(createInfo),
            [instance](const vk::DebugUtilsMessengerEXT m) { instance.destroyDebugUtilsMessengerEXT(m); });
        return {ctx};
    }

    template <typename Tag>
    [[nodiscard]] auto VulkanInstance<Tag>::createVulkanInstance() const -> VulkanDebugMessenger<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>
    {
        setEnvironmentVariable("VK_LAYER_PATH", std::filesystem::current_path());
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        // ReSharper disable once CppRedundantBooleanExpressionArgument
        if (VulkanSettings::enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("已请求验证层, 但不可用!");
        }

        constexpr vk::ApplicationInfo appInfo{"Hello Triangle", vk::makeVersion(1, 0, 0), "No Engine",
                                              vk::makeVersion(1, 0, 0), vk::makeApiVersion(0, 1, 0, 0)};

        auto extensions = VulkanTools::getRequiredExtensions();
        vk::InstanceCreateInfo createInfo{};
        createInfo.setPApplicationInfo(&appInfo);
        createInfo.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()));
        createInfo.setPEnabledExtensionNames(extensions);

        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (VulkanSettings::enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanSettings::validationLayers.size());
            createInfo.ppEnabledLayerNames = std::begin(VulkanSettings::validationLayers);
            VulkanTools::populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
        }

        try {
            const vk::Instance instance = createInstance(createInfo);
            VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
            ctx->vulkanContext.instance = VulkanResource<vk::Instance>(instance, [](const vk::Instance i) {
                vkDestroyInstance(i, nullptr);
                std::cout << "[Vulkan 销毁信息]: 销毁 Vulkan 实例(instance)!\n";
            });
        }
        catch (const vk::SystemError& err) {
            throw std::runtime_error(std::string("创建实例失败: ") + err.what());
        }

        return {ctx};
    }

    template <typename Tag>
    auto VulkanInstance<Tag>::setEnvironmentVariable(const std::string &name, std::filesystem::path path) -> void {
        const auto &pathStr = path.make_preferred().string();
#ifdef _WIN32
        _putenv_s(name.c_str(), pathStr.c_str());
#else
        setenv(name.c_str(), pathStr.c_str(), 1);
#endif
    }

    // 检查验证层支持
    template <typename Tag>
    auto VulkanInstance<Tag>::checkValidationLayerSupport() -> bool {
        const auto layerProperties = vk::enumerateInstanceLayerProperties();
        for (const char *layerName: VulkanSettings::validationLayers) {
            auto layerFound = false;
            for (const auto &layerProperty: layerProperties) {
                if (strcmp(
                            layerName,
                            layerProperty.layerName
                            ) == 0) {
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

    auto CreateGlfwWindow(
            const int width,
            const int height,
            const char *title
            ) -> VulkanInstance<IVulkanInit> {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        const auto ctx = std::make_shared<GlfwContext>();
        ctx->window.reset(
                glfwCreateWindow(width, height, title, nullptr, nullptr)
                );
        glfwSetWindowUserPointer(ctx->window.get(), ctx.get());
        glfwSetFramebufferSizeCallback(
                ctx->window.get(),
                [](GLFWwindow *window, int, int) {
                    const auto context = static_cast<GlfwContext *>(glfwGetWindowUserPointer(window));
                    context->vulkanContext.framebufferResized = true;
                }
                );
        return {ctx};
    }

    template struct VulkanInstance<IVulkanInit>;
    template struct VulkanDebugMessenger<IVulkanInit>;
    template struct VulkanSurface<IVulkanInit>;

} // namespace CustomVulkan
