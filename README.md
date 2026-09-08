# CustomVulkan 学习笔记 / 项目记录

这是一个用来**学习 Vulkan 和 C++20 新特性**（主要是 C++20 Modules、Concepts 和 Tag 分发）的小练习项目。

---

## 💡 核心机制：C++20 Concepts (`requires`) + Tag 分发

这个项目最有意思的地方是利用 C++20 的 **`requires` 约束** 和 **Tag 结构体**（`IVulkanInit` / `IVulkanRecreate`），在**编译期**强制规定了 Vulkan 资源的创建顺序和流程。

### 1. 为什么用它？

Vulkan 初始化步骤非常繁琐且有严格的前后依赖（比如：必须先建 Instance，再选 Physical Device，再建 Device，再建 Swapchain...）。如果调错顺序，运行时就会直接崩溃。

利用 `requires` 约束，可以实现**“只能按照特定顺序调用下一个函数”**，如果在代码里把顺序写错了，**在编译阶段就会报错**，不需要等到运行才发现。

---

### 2. 代码是怎么实现的？

#### ① 定义 Tag 标记 (`Common.cppm`)
项目中定义了两个空结构体作为标记类型：
```cpp
struct IVulkanInit {};     // 首次初始化标记
struct IVulkanRecreate {}; // 交换链重建标记（比如调整窗口大小时）
```

#### ② 用 `requires` 限定函数调用约束 (`Swapchain.cppm` / `Pipeline.cppm` 等)
通过模板和 `requires std::is_same_v<Tag, ...>` 限制函数只有在特定 Tag 下才能被调用，并且**返回下一个步骤的包装类型**：

```cpp
// 交换链创建：只有当 Tag 为 IVulkanInit 时，才能调用 createVulkanSwapChain()
// 函数调用后返回下一个阶段的包装类型：VulkanImageViews<IVulkanInit>
template <typename Tag>
struct VulkanSwapChain {
    std::shared_ptr<GlfwContext> ctx;

    [[nodiscard]] auto createVulkanSwapChain() const -> VulkanImageViews<IVulkanInit>
        requires std::is_same_v<Tag, IVulkanInit>;

    // 重建交换链时使用 IVulkanRecreate 约束
    [[nodiscard]] auto recreateSwapChain() const -> VulkanImageViews<IVulkanRecreate>
        requires std::is_same_v<Tag, IVulkanRecreate>;
};
```

#### ③ 形成类型的“链式流水线”
从创建窗口开始，每一个步骤的函数都会返回下一个包含 `IVulkanInit` 类型的对象，链条大致如下：

```text
CreateGlfwWindow()
  ↓ (返回 VulkanInstance<IVulkanInit>)
createVulkanInstance()
  ↓ (返回 VulkanDebugMessenger<IVulkanInit>)
setupVulkanDebugMessenger()
  ↓ (返回 VulkanSurface<IVulkanInit>)
...
createVulkanSwapChain()
  ↓ (返回 VulkanImageViews<IVulkanInit>)
createVulkanImageViews()
  ...
```

如果少调或者跳过了某一步（例如想直接在 `VulkanInstance` 上调 `createVulkanSwapChain`），编译器就会发现类型不匹配或约束不满足，直接提示编译错误！

---

## 📂 文件结构一览

- **`Common.cppm`**: 定义 `VulkanResource` (RAII 自动销毁容器)、Tag 类型和基础结构体
- **`Context.cppm`**: 窗口、Instance、Debug 回调与 Surface
- **`DeviceSelection.cppm`**: 物理设备与逻辑设备选择
- **`Swapchain.cppm`**: 交换链与 ImageView
- **`Pipeline.cppm`**: RenderPass 与渲染管线
- **`Commands.cppm`**: 帧缓冲区、命令池、顶点缓冲区与 CommandBuffer 录制
- **`Sync.cppm`**: 信号量 (Semaphore) 与栅栏 (Fence) 同步对象
- **`VulkanEncapsulation.cppm`**: 顶层渲染主循环与交换链重建