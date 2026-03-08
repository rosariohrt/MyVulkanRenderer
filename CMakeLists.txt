cmake_minimum_required(VERSION 3.16)
project(MyVulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)

# --- Shader Compilers ---
find_program(GLSLC_EXECUTABLE glslc HINTS ${Vulkan_GLSLC_EXECUTABLE} $ENV{VULKAN_SDK}/bin)
if (NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Set VULKAN_SDK environment variable.")
endif ()

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin)
if (NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc not found. Set VULKAN_SDK environment variable.")
endif ()

# --- GLSL Shader Compilation (.vert, .frag -> .spv) ---
function(add_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)

    foreach (SHADER_SOURCE ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
        set(SHADER_OUTPUT ${SHADERS_DIR}/${SHADER_NAME}.spv)

        add_custom_command(
                OUTPUT ${SHADER_OUTPUT}
                COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling GLSL shader: ${SHADER_NAME}"
                VERBATIM
        )
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach ()

    add_custom_target(${TARGET} DEPENDS ${SHADER_OUTPUTS})
endfunction()

# --- Slang Shader Compilation (.slang -> .spv) ---
function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES;ENTRY_POINTS" ${ARGN})
    set(SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)

    # Default entry points if not specified
    if (NOT SHADER_ENTRY_POINTS)
        set(SHADER_ENTRY_POINTS -entry vertMain -entry fragMain)
    endif ()

    add_custom_command(
            OUTPUT ${SHADERS_DIR}/slang.spv
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES}
                    -target spirv -profile spirv_1_4
                    -emit-spirv-directly -fvk-use-entrypoint-name
                    ${SHADER_ENTRY_POINTS}
                    -o slang.spv
            WORKING_DIRECTORY ${SHADERS_DIR}
            DEPENDS ${SHADER_SOURCES}
            COMMENT "Compiling Slang shaders"
            VERBATIM
    )

    add_custom_target(${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

# --- Gather shader sources ---
file(GLOB SHADER_GLSL_SOURCES shaders/*.vert shaders/*.frag)
file(GLOB SHADER_SLANG_SOURCES shaders/*.slang)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw)

# GLSL shaders
add_shader_target(GlslShaders SOURCES ${SHADER_GLSL_SOURCES})
add_dependencies(${PROJECT_NAME} GlslShaders)

# Slang shaders (only if .slang files exist)
if (SHADER_SLANG_SOURCES)
    add_slang_shader_target(SlangShaders SOURCES ${SHADER_SLANG_SOURCES})
    add_dependencies(${PROJECT_NAME} SlangShaders)
endif ()
