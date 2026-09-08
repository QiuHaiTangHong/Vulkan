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
            -Werror # 将所有警告视为错误
            -Wshadow # 当局部变量遮蔽全局变量时警告
            -Wnon-virtual-dtor # 如果类有虚函数但没有虚析构函数时警告
            -Wold-style-cast # 使用 C 风格强制转换时警告
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
        "${GLOBAL_LIB_ROOT_PATH}/Vulkan-Utility-Libraries"
        "${GLOBAL_LIB_ROOT_PATH}/Vulkan-Loader"
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

set(MODULES_ROOT_PATH "${PROJECT_CUSTOM_LIB_PATH}/custom/modules")
set(MODULE_DEF_PATH "${MODULES_ROOT_PATH}/definition")
set(MODULE_IMPL_PATH "${MODULES_ROOT_PATH}/implement")

# 定义资源目录
set(RESOURCE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/resource")
set(RESOURCE_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")

# 定义 Shader 文件路径
set(SHADER_SRC_DIR "${RESOURCE_SRC_DIR}/shaders")
set(SHADER_DST_DIR "${RESOURCE_DST_DIR}/shaders")

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
find_package(VulkanUtilityLibraries REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)

# 定义主目标
file(
        GLOB_RECURSE
        PROJECT_CUSTOM_LIB_IMPL
        "${PROJECT_CUSTOM_LIB_PATH}/src/*.cpp"
)
add_executable(
        Vulkan
        ${PROJECT_ROOT_PATH}/main.cpp
)

# 查找 Shader 编译工具.
find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(GLSLANG_VALIDATOR)
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_COMPILE_FLAGS "-V") # -V 代表生成 SPIR-V
else()
    message(FATAL_ERROR "未找到 Shader 编译器 glslangValidator！")
endif()

# 收集并编译 Shader 文件.
file(MAKE_DIRECTORY ${SHADER_DST_DIR})
file(GLOB_RECURSE SHADER_FILES
        "${SHADER_SRC_DIR}/*.vert"
        "${SHADER_SRC_DIR}/*.frag"
        "${SHADER_SRC_DIR}/*.comp"
        "${SHADER_SRC_DIR}/*.geom"
)
message(STATUS "------------------------------------------------")
if(SHADER_FILES)
    message(STATUS "找到以下 Shader 文件:")
    foreach(FILE_PATH ${SHADER_FILES})
        message(STATUS "${FILE_PATH}")
    endforeach()
else()
    message(WARNING "警告: 在目录 ${SHADER_SRC_DIR} 中没有找到任何 Shader 文件！")
endif()
message(STATUS "------------------------------------------------")

set(SPIRV_BINARY_FILES "")
foreach(SHADER_PATH ${SHADER_FILES})
    get_filename_component(RAW_NAME "${SHADER_PATH}" NAME)
    string(REPLACE "." "_" TEMP_NAME "${RAW_NAME}")
    set(NEW_NAME "${TEMP_NAME}.spv")
    set(SPIRV_PATH "${SHADER_DST_DIR}/${NEW_NAME}")
    add_custom_command(
            OUTPUT ${SPIRV_PATH}
            COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAGS} ${SHADER_PATH} -o ${SPIRV_PATH}
            DEPENDS ${SHADER_PATH}
            COMMENT "正在编译 Shader: ${RAW_NAME} -> ${NEW_NAME}"
            VERBATIM
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV_PATH})
endforeach()
add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})

# 处理其它资源.
set(FILTERED_RESOURCES "")
foreach(ITEM ${RESOURCE_ITEMS})
    get_filename_component(ABS_ITEM "${ITEM}" ABSOLUTE)
    get_filename_component(ABS_SHADER_DIR "${SHADER_SRC_DIR}" ABSOLUTE)

    if(NOT ABS_ITEM STREQUAL ABS_SHADER_DIR)
        list(APPEND FILTERED_RESOURCES "${ITEM}")
    endif()
endforeach()
if(FILTERED_RESOURCES)
    add_custom_command(
            OUTPUT "${RESOURCE_DST_DIR}/.log"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RESOURCE_DST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy ${FILTERED_RESOURCES} "${RESOURCE_DST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E touch "${RESOURCE_DST_DIR}/.log"
            DEPENDS ${FILTERED_RESOURCES}
            COMMENT "正在同步通用资源..."
            VERBATIM
    )
else()
    add_custom_command(
            OUTPUT "${RESOURCE_DST_DIR}/.log"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RESOURCE_DST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E touch "${RESOURCE_DST_DIR}/.log"
            COMMENT "没有额外资源需要同步"
            VERBATIM
    )
endif()
add_custom_target(CopyResources ALL DEPENDS "${RESOURCE_DST_DIR}/.log")

add_dependencies(Vulkan CopyResources)
add_dependencies(Vulkan CompileShaders)

target_compile_definitions(
        Vulkan PRIVATE
        RESOURCE_DIR="${RESOURCE_DST_DIR}"
)

target_compile_definitions(
        Vulkan PRIVATE
        VVL_JSON_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

file(GLOB ALL_MODULE_DEFS "${MODULE_DEF_PATH}/*.cppm")
target_sources(Vulkan
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${PROJECT_CUSTOM_LIB_PATH}"
        FILES ${ALL_MODULE_DEFS}
)

file(GLOB ALL_MODULE_IMPLS "${MODULE_IMPL_PATH}/*.cpp")
target_sources(Vulkan
        PRIVATE
        ${ALL_MODULE_IMPLS}
        ${IMGUI_SOURCES}
        ${IMGUI_BACKENDS}
        ${PROJECT_CUSTOM_LIB_IMPL}
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
        Vulkan::LayerSettings
        Vulkan::UtilityHeaders
)