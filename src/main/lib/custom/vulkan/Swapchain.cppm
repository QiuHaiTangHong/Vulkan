module;
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <memory>

export module CustomVulkan.Swapchain;
import CustomVulkan.Common;
import CustomVulkan.Pipeline;
export namespace CustomVulkan {
    struct CreateVulkanImageViews {
        std::shared_ptr<GlfwContext> ctx;

        [[nodiscard]] CreateVulkanRenderPass createVulkanImageViews() const {
            const std::vector<vk::Image> swapChainImages = ctx->vulkanContext.swapChainImages;
            std::vector<vk::ImageView> tempImageViews;
            tempImageViews.reserve(swapChainImages.size());
            vk::Device device = ctx->vulkanContext.device.get();
            try {
                for (const auto &swapChainImage: swapChainImages) {
                    vk::ImageViewCreateInfo createInfo{};
                    createInfo
                            .setImage(swapChainImage)
                            .setViewType(vk::ImageViewType::e2D)
                            .setFormat(ctx->vulkanContext.swapChainImageFormat)
                            .setComponents(
                                    vk::ComponentMapping{
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity
                                    }
                                    )
                            .setSubresourceRange(
                                    vk::ImageSubresourceRange{
                                            vk::ImageAspectFlagBits::eColor,
                                            0,
                                            1,
                                            0,
                                            1
                                    }
                                    )
                            .setPNext(nullptr);
                    tempImageViews.push_back(device.createImageView(createInfo));
                }
                ctx->vulkanContext.swapChainImageViews = VulkanResource<std::vector<vk::ImageView> >(
                        std::move(tempImageViews),
                        [device](const std::vector<vk::ImageView> &views) {
                            for (auto v: views) {
                                if (v) {
                                    device.destroyImageView(v);
                                }
                            }
                            std::cout << "[Vulkan 销毁信息]: 成功批量销毁 " << views.size() << " 个 ImageView!\n";
                        }
                        );
            } catch (const vk::SystemError &err) {
                throw std::runtime_error("创建 Image Views 失败: " + std::string(err.what()));
            }
            return {ctx};
        }
    };

    struct CreateVulkanSwapChain {
        std::shared_ptr<GlfwContext> ctx;

        [[nodiscard]] CreateVulkanImageViews createVulkanSwapChain() const {
            auto [capabilities, formats, presentModes] = querySwapChainSupport(
                    ctx->vulkanContext.physicalDevice,
                    ctx->vulkanContext.surface.get()
                    );
            const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
            const vk::PresentModeKHR presentMode     = chooseSwapPresentMode(presentModes);
            const vk::Extent2D extent                = chooseSwapExtent(capabilities, ctx->window.get());
            uint32_t imageCount                      = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }

            vk::SwapchainCreateInfoKHR createInfo{};
            createInfo
                    .setSurface(ctx->vulkanContext.surface.get())
                    .setMinImageCount(imageCount)
                    .setImageFormat(surfaceFormat.format)
                    .setImageColorSpace(surfaceFormat.colorSpace)
                    .setImageExtent(extent)
                    .setImageArrayLayers(1)
                    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

            const auto [graphicsFamily, presentFamily] = VulkanTools::findQueueFamilies(
                    ctx->vulkanContext.physicalDevice,
                    ctx->vulkanContext.surface.get()
                    );
            const uint32_t queueFamilyIndices[] = {graphicsFamily.value(), presentFamily.value()};

            if (graphicsFamily != presentFamily) {
                createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
                          .setQueueFamilyIndexCount(2)
                          .setPQueueFamilyIndices(queueFamilyIndices);
            } else {
                createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
            }

            auto compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) {
                compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
            } else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) {
                compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
            } else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) {
                compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eInherit;
            }
            createInfo
                    .setPreTransform(capabilities.currentTransform)
                    .setCompositeAlpha(compositeAlpha)
                    .setPresentMode(presentMode)
                    .setClipped(true)
                    .setOldSwapchain(nullptr);

            try {
                const vk::Device device      = ctx->vulkanContext.device.get();
                ctx->vulkanContext.swapChain = VulkanResource<vk::SwapchainKHR>(
                        device.createSwapchainKHR(createInfo),
                        [device](const vk::SwapchainKHR s) {
                            if (s) {
                                device.destroySwapchainKHR(s);
                                std::cout << "[Vulkan 销毁信息]: 销毁交换链(swapChain)!\n";
                            }
                        }
                        );
                ctx->vulkanContext.swapChainImages = device.getSwapchainImagesKHR(ctx->vulkanContext.swapChain.get());
                ctx->vulkanContext.swapChainImageFormat = createInfo.imageFormat;
                ctx->vulkanContext.swapChainExtent = createInfo.imageExtent;
            } catch (const vk::SystemError &err) {
                throw std::runtime_error("创建交换链失败: " + std::string(err.what()));
            } catch (const std::exception &e) {
                throw std::runtime_error(std::string("创建交换链时发生异常: ") + e.what());
            }
            return {ctx};
        }

        private:

            // 查询交换链支持
            static SwapChainSupportDetails querySwapChainSupport(
                    const vk::PhysicalDevice device,
                    const vk::SurfaceKHR surface
                    ) {
                return SwapChainSupportDetails{
                        device.getSurfaceCapabilitiesKHR(surface),
                        device.getSurfaceFormatsKHR(surface),
                        device.getSurfacePresentModesKHR(surface)
                };
            }

            // 选择交换链表面格式
            static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
                    const std::vector<vk::SurfaceFormatKHR> &availableFormats
                    ) {
                for (const auto &availableFormat: availableFormats) {
                    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
                        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                        return availableFormat;
                    }
                }
                return availableFormats[0];
            }

            // 选择交换链当前模式
            static vk::PresentModeKHR chooseSwapPresentMode(
                    const std::vector<vk::PresentModeKHR> &availablePresentModes
                    ) {
                for (const auto &availablePresentMode: availablePresentModes) {
                    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                        return availablePresentMode;
                    }
                }

                return vk::PresentModeKHR::eFifo;
            }

            // 选择交换链范围
            static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window) {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                    return capabilities.currentExtent;
                }
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actualExtent = {
                        static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(
                        actualExtent.width,
                        capabilities.minImageExtent.width,
                        capabilities.maxImageExtent.width
                        );
                actualExtent.height = std::clamp(
                        actualExtent.height,
                        capabilities.minImageExtent.height,
                        capabilities.maxImageExtent.height
                        );

                return actualExtent;
            }
    };
}
