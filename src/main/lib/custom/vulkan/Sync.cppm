module;
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
export module CustomVulkan.Sync;

import CustomVulkan.Common;

export namespace CustomVulkan {
    struct CreateVulkanSyncObjects {
            std::shared_ptr<GlfwContext> ctx;

            [[nodiscard]] auto createVulkanSyncObjects() const -> std::shared_ptr<GlfwContext> {
                const auto device = ctx->vulkanContext.device.get();
                std::vector<vk::Semaphore> imgSems(VulkanSettings::maxFramesInFlight);
                std::vector<vk::Semaphore> renSems(VulkanSettings::maxFramesInFlight);
                std::vector<vk::Fence> fences(VulkanSettings::maxFramesInFlight);
                constexpr vk::SemaphoreCreateInfo semaphoreInfo{};
                vk::FenceCreateInfo fenceInfo{};
                fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
                try {
                    for (size_t i = 0; i < VulkanSettings::maxFramesInFlight; i++) {
                        imgSems[i] = device.createSemaphore(semaphoreInfo);
                        renSems[i] = device.createSemaphore(semaphoreInfo);
                        fences[i] = device.createFence(fenceInfo);
                    }
                    ctx->vulkanContext.imageAvailableSemaphores = VulkanResource<std::vector<vk::Semaphore>>(
                            imgSems,
                            [device](const std::vector<vk::Semaphore> &semaphores) {
                                for (const auto &semaphore: semaphores) {
                                    device.destroySemaphore(semaphore);
                                }
                                std::cout << "[Vulkan 销毁信息]: 销毁图像可用信号量(imageAvailableSemaphore)!\n";
                            });
                    ctx->vulkanContext.renderFinishedSemaphores = VulkanResource<std::vector<vk::Semaphore>>(
                            renSems,
                            [device](const std::vector<vk::Semaphore> &semaphores) {
                                for (const auto &semaphore: semaphores) {
                                    device.destroySemaphore(semaphore);
                                }
                                std::cout << "[Vulkan 销毁信息]: 销毁渲染完成信号量(renderFinishedSemaphore)!\n";
                            });
                    ctx->vulkanContext.inFlightFences = VulkanResource<std::vector<vk::Fence>>(
                            fences,
                            [device](const std::vector<vk::Fence> &fs) {
                                for (const auto &fence: fs) {
                                    device.destroyFence(fence);
                                }
                                std::cout << "[Vulkan 销毁信息]: 销毁围栏(inFlightFence)!\n";
                            });
                } catch (const vk::SystemError &e) {
                    throw std::runtime_error("创建同步对象失败: " + std::string(e.what()));
                }
                return ctx;
            }
    };
}