cmake_minimum_required(VERSION 3.16)
project(n-body LANGUAGES C CXX)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT Python_EXECUTABLE)
    message(STATUS "Defaulting to project's Python environment")
    set(Python_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3" CACHE STRING "" FORCE)
endif()

# n-body
add_executable(${PROJECT_NAME} WIN32
    src/main.h
    src/main.c
    src/simulation.c
    src/gui.c
    src/graphics.c
    src/camera.c
    src/ghost.c
    src/prediction.c
)

target_include_directories(${PROJECT_NAME} PRIVATE lib)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,GNU>>:-fsanitize=address>
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:Clang,GNU>>:-fsanitize=address>
)

# shaders
set(SHADER_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(SHADER_STAMP "${CMAKE_CURRENT_BINARY_DIR}/shaders.compiled")

file(
    GLOB_RECURSE SHADER_FILES
    CONFIGURE_DEPENDS "${SHADER_INPUT_DIR}/*"
)

add_custom_command(
    OUTPUT "${SHADER_STAMP}"
    COMMAND "${Python_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compile_shaders.py" "${SHADER_INPUT_DIR}" "${SHADER_OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" "-E" "touch" "${SHADER_STAMP}"
    DEPENDS ${SHADER_FILES}
    COMMENT "Compiling shaders"
)

add_custom_target(compile_shaders ALL DEPENDS "${SHADER_STAMP}")
add_dependencies(${PROJECT_NAME} compile_shaders)

# SDL3 + SDL_shadercross
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
    GIT_TAG "release-3.4.0"
)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Cross.git"
    GIT_TAG "main"
)

FetchContent_MakeAvailable(spirv_cross)
add_library(spirv_cross_c ALIAS spirv-cross-c)

FetchContent_Declare(
    SDL_shadercross
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL_shadercross.git"
    GIT_TAG "main"
)

set(SDLSHADERCROSS_DXC OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL_shadercross)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static SDL3_shadercross-static)
target_include_directories(${PROJECT_NAME} PRIVATE "${SDL3_SOURCE_DIR}/include" "${SDL_shadercross_SOURCE_DIR}/include")

# Dear ImGui + dear_bindings
FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "docking"
)

FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    dear_bindings
    GIT_REPOSITORY "https://github.com/dearimgui/dear_bindings.git"
    GIT_TAG "main"
)

FetchContent_MakeAvailable(dear_bindings)

file(MAKE_DIRECTORY "${dear_bindings_SOURCE_DIR}/generated/backends")
execute_process(
    COMMAND "${Python_EXECUTABLE}" "dear_bindings.py" "-o" "generated/dcimgui" "${imgui_SOURCE_DIR}/imgui.h"
    COMMAND "${Python_EXECUTABLE}" "dear_bindings.py" "--backend" "--include" "${imgui_SOURCE_DIR}/imgui.h" "--imconfig-path" "${imgui_SOURCE_DIR}/imconfig.h" "-o" "generated/backends/dcimgui_impl_sdl3" "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h"
    COMMAND "${Python_EXECUTABLE}" "dear_bindings.py" "--backend" "--include" "${imgui_SOURCE_DIR}/imgui.h" "--imconfig-path" "${imgui_SOURCE_DIR}/imconfig.h" "-o" "generated/backends/dcimgui_impl_sdlgpu3" "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.h"
    WORKING_DIRECTORY "${dear_bindings_SOURCE_DIR}"
)

add_library(imgui STATIC
    # imgui source
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"

    # imgui backend
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp"

    # dear_binding
    "${dear_bindings_SOURCE_DIR}/generated/dcimgui.cpp"
    "${dear_bindings_SOURCE_DIR}/generated/backends/dcimgui_impl_sdl3.cpp"
    "${dear_bindings_SOURCE_DIR}/generated/backends/dcimgui_impl_sdlgpu3.cpp"
)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_include_directories(imgui PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${dear_bindings_SOURCE_DIR}/generated"
    "${SDL3_SOURCE_DIR}/include"
)
