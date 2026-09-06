cmake_minimum_required(VERSION 3.18)

project(hurdygurdy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(MSVC)
    add_compile_options(/permissive- /GR- /EHs-c- /D_CRT_SECURE_NO_WARNINGS)
    # C4702 fires spuriously in the Asset<T>::Asset constructor initializer on
    # optimized builds (MSVC proves pool.alloc() never returns null, then warns
    # about the "unreachable" else-branch that doesn't exist)
    # C4127 flags conditionals which could be compile time, but are at runtime
    add_compile_options(/wd4702 /wd4127)
    set(STRICT_FLAGS /W4 /WX)
    set(TESTS_EXCEPTION_FLAG /EHsc)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wno-cpp")
    set(STRICT_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror)
    set(TESTS_EXCEPTION_FLAG -fexceptions)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CLANG_VENDOR_FLAGS
            -Wno-nullability-extension
            -Wno-nullability-completeness
        )
    endif()

    find_program(MOLD_PROGRAM mold)
    if(MOLD_PROGRAM)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    endif()
endif()

set(VulkanHeaders_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Vulkan-Headers/include)
set(SDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/include)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends
    ${SDL_INCLUDE_DIR}
)

add_executable(embed src/embed.c)

set(SHADERS
    shaders/render2d.vert
    shaders/render2d.frag
    shaders/debug2d.frag
    shaders/skybox.vert
    shaders/skybox.frag
    shaders/sprite.vert
    shaders/sprite.frag
    shaders/model.vert
    shaders/model.frag
    test/compute.comp
    test/tri.vert
    test/tri.frag
    test/sampler.frag
    test/mul.comp
    test/buf_to_img.comp
    test/uniform.vert
    test/uniform.frag
    test/storage_img.comp
    test/depth.vert
    test/depth.frag
    test/instance.vert
    test/instance.frag
    test/blur.comp
    test/invert.frag
)

set(SHADER_HEADERS)
foreach(shader ${SHADERS})
    set(spv ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv)
    set(spv_h ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv.h)

    add_custom_command(
        OUTPUT ${spv_h}
        COMMAND glslc -o ${spv} ${CMAKE_CURRENT_SOURCE_DIR}/src/${shader} -I${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMAND embed ${spv} ${shader}.spv ${spv_h}
        DEPENDS embed
            ${CMAKE_CURRENT_SOURCE_DIR}/src/${shader}
            ${CMAKE_CURRENT_SOURCE_DIR}/include/hurdygurdy.glsl
        COMMENT "Compiling ${shader}"
    )

    list(APPEND SHADER_HEADERS ${spv_h})
endforeach()

add_custom_target(shaders DEPENDS ${SHADER_HEADERS})

file(GLOB IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/*.cpp)
set(IMGUI_BACKEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_vulkan.cpp
)

set(VULKAN_SOURCES
    src/vulkan/loader.cpp
    src/vulkan/vulkan.cpp
    src/vulkan/gpu.cpp
)

set(SDL_SOURCES
    src/sdl/loader.cpp
    src/sdl/sdl.cpp
    src/sdl/window.cpp
    src/sdl/audio.cpp
)

set(CORE_SOURCES
    src/error.cpp
    src/init.cpp
    src/memory.cpp
    src/concurrency.cpp
    src/math.cpp
    src/geometry2d.cpp
    src/geometry3d.cpp
    src/noise.cpp
    src/strings.cpp
    src/binary.cpp
    src/pool.cpp
    src/serialization.cpp
    src/time.cpp
    src/audio.cpp
    src/assets.cpp
    src/dynlib.cpp
    src/render2d.cpp
    src/imgui.cpp
    ${SDL_SOURCES}
    ${VULKAN_SOURCES}
)

set(MISC_VENDOR_SOURCES
    src/vk_mem_alloc.cpp
    src/stb.c
)

add_library(hurdygurdy_core OBJECT ${CORE_SOURCES})
add_dependencies(hurdygurdy_core shaders)
target_include_directories(hurdygurdy_core
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_compile_options(hurdygurdy_core PRIVATE ${STRICT_FLAGS} ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy_imgui OBJECT ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
target_include_directories(hurdygurdy_imgui
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_compile_definitions(hurdygurdy_imgui PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_options(hurdygurdy_imgui PRIVATE ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy_misc OBJECT ${MISC_VENDOR_SOURCES})
target_include_directories(hurdygurdy_misc
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_compile_options(hurdygurdy_misc PRIVATE ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy STATIC)
target_link_libraries(hurdygurdy PUBLIC hurdygurdy_core hurdygurdy_imgui hurdygurdy_misc)

add_executable(tests
    src/test/tests.cpp
    src/test/span.cpp
    src/test/product.cpp
    src/test/sum.cpp
    src/test/maybe.cpp
    src/test/utility.cpp
    src/test/memory.cpp
    src/test/concurrency.cpp
    src/test/math.cpp
    src/test/geometry2d.cpp
    src/test/geometry3d.cpp
    src/test/noise.cpp
    src/test/strings.cpp
    src/test/binary.cpp
    src/test/smart_ptr.cpp
    src/test/array.cpp
    src/test/array_temp.cpp
    src/test/queue.cpp
    src/test/set.cpp
    src/test/map.cpp
    src/test/hash.cpp
    src/test/pool.cpp
    src/test/assets.cpp
    src/test/serialization.cpp
    src/test/gpu.cpp
    src/test/render2d.cpp
    src/test/ecs.cpp
    src/test/ecs_foreach.cpp
    src/test/ecs_serialize.cpp
)
target_link_libraries(tests hurdygurdy)
target_precompile_headers(tests PRIVATE
    <hurdygurdy.hpp>
)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(tests PRIVATE ${STRICT_FLAGS} ${TESTS_EXCEPTION_FLAG})
target_compile_definitions(tests PRIVATE HG_LOGGING=1)

foreach(tgt minimal editor)
    add_executable(${tgt} src/${tgt}.cpp)
    target_link_libraries(${tgt} hurdygurdy)
    target_include_directories(${tgt} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_options(${tgt} PRIVATE ${STRICT_FLAGS})
endforeach()

install(TARGETS hurdygurdy
    ARCHIVE DESTINATION lib
)

install(TARGETS tests minimal editor
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.glsl"
)

