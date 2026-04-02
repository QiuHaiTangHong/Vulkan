cmake_minimum_required(VERSION 3.28)
project(Vulkan VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
            -Wall # 开启几乎所有常用警告
            -Wextra # 开启额外的补充警告
            -Wpedantic # 严格遵守 ISO C++ 标准，禁止禁用非标准的扩展
            # -Werror # 将所有警告视为错误（只要有警告，编译就失败）
            -Wshadow # 当局部变量遮蔽全局变量时警告
            -Wnon-virtual-dtor # 如果类有虚函数但没有虚析构函数时警告
            -Wold-style-cast # 使用 C 风格强制转换时警告（建议用 static_cast 等）
            -Wcast-align # 指针转换导致对齐问题时警告
            -Wunused # 未使用的变量、函数等
            -Woverloaded-virtual # 虚函数重载不一致时警告
    )

    if (NOT WIN32)
        add_compile_options(-stdlib=libc++)
        add_link_options(-stdlib=libc++)
    endif ()

    if (WIN32)
        add_compile_options(-Wno-unused-command-line-argument)
    endif ()
endif ()

string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFROM_NAME)

set(
        GLOBAL_LIB_ROOT_PATH
        "${CMAKE_SOURCE_DIR}/lib/${PLATFROM_NAME}"
)

set(
        PROJECT_ROOT_PATH
        "${CMAKE_SOURCE_DIR}/src/main"
)

set(
        PROJECT_CUSTOM_LIB_PATH
        "${PROJECT_ROOT_PATH}/lib"
)

set(
        CMAKE_PREFIX_PATH
        "${GLOBAL_LIB_ROOT_PATH}/glfw"
        "${GLOBAL_LIB_ROOT_PATH}/glm"
        "${GLOBAL_LIB_ROOT_PATH}/Vulkan-Headers"
        "${GLOBAL_LIB_ROOT_PATH}/Vulkan-ValidationLayers"
        "${GLOBAL_LIB_ROOT_PATH}/glslang"
)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFROM_NAME}/imgui")

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
)

set(IMGUI_BACKENDS
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

set(MODULES_ROOT_PATH "${PROJECT_CUSTOM_LIB_PATH}/custom/vulkan")

set(RESOURCE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/resource")
set(RESOURCE_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")

set(VVL_SETTING_PATH "${PROJECT_CUSTOM_LIB_PATH}/custom/settings")
set(VVL_LIB_PATH "${GLOBAL_LIB_ROOT_PATH}/Vulkan-ValidationLayers")
if (WIN32)
    set(VVL_SO_NAME "VkLayer_khronos_validation.dll")
    set(VVL_JSON_DIR "${VVL_LIB_PATH}/bin")
else ()
    set(VVL_SO_NAME "libVkLayer_khronos_validation.so")
    set(VVL_JSON_DIR "${VVL_LIB_PATH}/lib")
endif ()
set(VVL_SO_ABSOLUTE_PATH "${VVL_JSON_DIR}/${VVL_SO_NAME}")
configure_file(
        "${VVL_SETTING_PATH}/VkLayer_khronos_validation.json.in"
        "${CMAKE_CURRENT_BINARY_DIR}/VkLayer_khronos_validation.json"
        @ONLY
)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)

file(
        GLOB_RECURSE
        PROJECT_CUSTOM_LIB_IMPL
        "${PROJECT_CUSTOM_LIB_PATH}/src/*.cpp"
)

set(MODULES_ROOT_PATH "${PROJECT_CUSTOM_LIB_PATH}/custom/vulkan")
file(GLOB ALL_MODULE_FILES "${MODULES_ROOT_PATH}/*.cppm")

add_executable(
        Vulkan
        ${PROJECT_ROOT_PATH}/main.cpp
        ${IMGUI_SOURCES}
        ${IMGUI_BACKENDS}
        ${PROJECT_CUSTOM_LIB_IMPL}
)

add_custom_command(
        OUTPUT "${RESOURCE_DST_DIR}/.log"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${RESOURCE_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${RESOURCE_SRC_DIR}" "${RESOURCE_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${RESOURCE_DST_DIR}/.log"
        DEPENDS "${RESOURCE_SRC_DIR}"
        COMMENT "正在同步整个资源目录: ${RESOURCE_SRC_DIR} -> ${RESOURCE_DST_DIR}"
        VERBATIM
)
add_custom_target(CopyResources ALL DEPENDS "${RESOURCE_DST_DIR}/.log")
add_dependencies(Vulkan CopyResources)
target_compile_definitions(
        Vulkan PRIVATE
        RESOURCE_DIR="${RESOURCE_DST_DIR}"
)

target_compile_definitions(
        Vulkan PRIVATE
        VVL_JSON_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_sources(Vulkan
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${PROJECT_CUSTOM_LIB_PATH}"
        FILES ${ALL_MODULE_FILES}
)

target_include_directories(
        Vulkan PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${PROJECT_CUSTOM_LIB_PATH}
        ${VulkanHeaders_INCLUDE_DIRS}
)

target_link_libraries(
        Vulkan PRIVATE
        glfw
        glm::glm
        Vulkan::Vulkan
)