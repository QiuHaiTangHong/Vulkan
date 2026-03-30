module;
#include <utility>
// clang-format off
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// clang-format on
#include <functional>
#include <iostream>
#include <memory>

export module CustomVulkan.Common;
#ifdef NDEBUG
constexpr bool isDebug = false;
#else
constexpr bool isDebug = true;
#endif
export namespace CustomVulkan {
    struct VulkanSettings {
            static constexpr int width = 800;

            static constexpr int height = 600;

            static constexpr bool enableValidationLayers = isDebug;
            static constexpr auto validationLayers = {
                    "VK_LAYER_KHRONOS_validation"};

            static constexpr auto deviceExtensions = {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            static constexpr uint32_t maxFramesInFlight = 3;
    };

    struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;

            std::optional<uint32_t> presentFamily;

            [[nodiscard]] bool isComplete() const {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
    };

    struct SwapChainSupportDetails {
            vk::SurfaceCapabilitiesKHR capabilities;
            std::vector<vk::SurfaceFormatKHR> formats;
            std::vector<vk::PresentModeKHR> presentModes;
    };

    template<typename T>
    class VulkanResource {
        public:
            using Deleter = std::function<void(T)>;

            VulkanResource() : handle(nullptr) {
            }

            VulkanResource(T handle, Deleter deleter) :
                handle(handle),
                deleter(std::move(deleter)) {
            }

            ~VulkanResource() {
                this->cleanup();
            }

            VulkanResource(const VulkanResource &) = delete;

            auto operator=(const VulkanResource &) -> VulkanResource & = delete;

            VulkanResource(VulkanResource &&other) noexcept {
                this->handle = other.handle;
                this->deleter = std::move(other.deleter);
                other.handle = nullptr;
                other.deleter = nullptr;
            }

            auto operator=(VulkanResource &&other) noexcept -> VulkanResource & {
                if (this != std::addressof(other)) {
                    this->cleanup();
                    this->handle = other.handle;
                    this->deleter = std::move(other.deleter);
                    other.handle = nullptr;
                    other.deleter = nullptr;
                }
                return *this;
            }

            auto cleanup() -> void {
                if (this->handle != nullptr && this->deleter) {
                    this->deleter(handle);
                    handle = nullptr;
                }
            }

            [[nodiscard]] T get() const {
                return this->handle;
            }

            explicit operator T() const {
                return this->handle;
            }

            auto operator&() -> T * = delete; // NOLINT(*-runtime-operator)

        private:
            T handle;
            Deleter deleter;
    };

    template<typename T>
    class VulkanResource<std::vector<T>> {
        public:
            using Deleter = std::function<void(std::vector<T> &)>;

            VulkanResource() = default;

            VulkanResource(std::vector<T> handles, Deleter deleter) : handles(std::move(handles)), deleter(std::move(deleter)) {
            }

            ~VulkanResource() {
                this->cleanup();
            }

            VulkanResource(const VulkanResource &) = delete;

            VulkanResource &operator=(const VulkanResource &) = delete;

            VulkanResource(VulkanResource &&other) noexcept
                : handles(std::move(other.handles)), deleter(std::move(other.deleter)) {
                other.deleter = nullptr;
            }

            VulkanResource &operator=(VulkanResource &&other) noexcept {
                if (this != std::addressof(other)) {
                    this->cleanup();
                    this->handles = std::move(other.handles);
                    this->deleter = std::move(other.deleter);
                    other.deleter = nullptr;
                }
                return *this;
            }

            void cleanup() {
                if (!handles.empty() && deleter) {
                    deleter(handles);
                    handles.clear();
                }
            }

            [[nodiscard]] const std::vector<T> &get() const {
                return handles;
            }

            [[nodiscard]] std::vector<T> &get() {
                return handles;
            }

            const T &operator[](size_t i) const {
                return handles[i];
            }

            [[nodiscard]] size_t size() const {
                return handles.size();
            }

        private:
            std::vector<T> handles;
            Deleter deleter;
    };

    struct VulkanTools {
            // 获取必须扩展
            static std::vector<const char *> getRequiredExtensions() {
                uint32_t glfwExtensionCount = 0;
                const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

                std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

                if (VulkanSettings::enableValidationLayers) {
                    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                }
                return extensions;
            }

            // 填充调试信息
            static void populateDebugMessengerCreateInfo(
                    vk::DebugUtilsMessengerCreateInfoEXT &createInfo) {
                createInfo.setMessageSeverity(
                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
                createInfo.setMessageType(
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
                // ReSharper disable once CppDeprecatedEntity
                createInfo.setPfnUserCallback(debugCallback);
            }

            // 查找队列族
            static QueueFamilyIndices findQueueFamilies(
                    const vk::PhysicalDevice physicalDevice,
                    const vk::SurfaceKHR surface) {
                QueueFamilyIndices indices;
                const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

                for (uint32_t i = 0; i < queueFamilies.size(); i++) {
                    if (queueFamilies[i].queueFlags &
                        vk::QueueFlagBits::eGraphics) {
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

        private:
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                    [[maybe_unused]] void *pUserData) {
                std::cerr << "[Vulkan 调试信息]: " << pCallbackData->pMessage << std::endl;
                return VK_FALSE;
            }
    };

    struct VulkanContext {
            VulkanResource<vk::Instance> instance{};
            VulkanResource<vk::DebugUtilsMessengerEXT> debugMessenger{};
            vk::PhysicalDevice physicalDevice{};
            VulkanResource<vk::Device> device{};
            vk::Queue graphicsQueue{};
            vk::Queue presentQueue{};
            VulkanResource<vk::SurfaceKHR> surface{};
            VulkanResource<vk::SwapchainKHR> swapChain{};
            std::vector<vk::Image> swapChainImages{};
            vk::Format swapChainImageFormat{};
            vk::Extent2D swapChainExtent{};
            VulkanResource<std::vector<vk::ImageView>> swapChainImageViews{};
            VulkanResource<vk::RenderPass> renderPass{};
            VulkanResource<vk::PipelineLayout> pipelineLayout{};
            VulkanResource<vk::Pipeline> graphicsPipeline{};
            VulkanResource<std::vector<vk::Framebuffer>> swapChainFramebuffers{};
            VulkanResource<vk::CommandPool> commandPool{};
            VulkanResource<std::vector<vk::CommandBuffer>> commandBuffer{};
            VulkanResource<std::vector<vk::Semaphore>> imageAvailableSemaphores{};
            VulkanResource<std::vector<vk::Semaphore>> renderFinishedSemaphores{};
            VulkanResource<std::vector<vk::Fence>> inFlightFences{};
            uint32_t currentFrame = 0;
    };

    struct GlfwContext {
            std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window{
                    nullptr,
                    [](GLFWwindow *p) {
                        if (p) {
                            glfwDestroyWindow(p);
                            std::cout << "窗口已销毁\n";
                        }
                    }};
            VulkanContext vulkanContext;
    };
} // namespace CustomVulkan
