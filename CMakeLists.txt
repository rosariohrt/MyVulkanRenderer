cmake_minimum_required(VERSION 3.16)
project(MyVulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw)

# --- Shader Compilation ---

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)

find_program(GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE} $ENV{VULKAN_SDK}/bin)
if (NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Set VULKAN_SDK environment variable.")
endif ()

# GLSL (.vert, .frag -> .spv)
file(GLOB GLSL_SOURCES ${SHADER_SRC_DIR}/*.vert ${SHADER_SRC_DIR}/*.frag)

foreach (SHADER ${GLSL_SOURCES})
    get_filename_component(NAME ${SHADER} NAME)
    set(OUTPUT ${SHADER_OUT_DIR}/${NAME}.spv)

    add_custom_command(
            OUTPUT ${OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
            COMMAND ${GLSLC} ${SHADER} -o ${OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling GLSL: ${NAME}"
            VERBATIM
    )
    list(APPEND SPV_OUTPUTS ${OUTPUT})
endforeach ()

# Slang (.slang -> .spv)
file(GLOB SLANG_SOURCES ${SHADER_SRC_DIR}/*.slang)

if (SLANG_SOURCES)
    find_program(SLANGC slangc HINTS $ENV{VULKAN_SDK}/bin)
    if (NOT SLANGC)
        message(FATAL_ERROR "slangc not found. Set VULKAN_SDK environment variable.")
    endif ()

    foreach (SHADER ${SLANG_SOURCES})
        get_filename_component(NAME ${SHADER} NAME_WE)
        set(OUTPUT ${SHADER_OUT_DIR}/${NAME}.spv)

        add_custom_command(
                OUTPUT ${OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
                COMMAND ${SLANGC} ${SHADER}
                        -target spirv -profile spirv_1_4
                        -emit-spirv-directly -fvk-use-entrypoint-name
                        -entry vertMain -entry fragMain
                        -o ${OUTPUT}
                DEPENDS ${SHADER}
                COMMENT "Compiling Slang: ${NAME}.slang"
                VERBATIM
        )
        list(APPEND SPV_OUTPUTS ${OUTPUT})
    endforeach ()
endif ()

add_custom_target(Shaders DEPENDS ${SPV_OUTPUTS})
add_dependencies(${PROJECT_NAME} Shaders)
