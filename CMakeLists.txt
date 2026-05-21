cmake_minimum_required(VERSION 3.20)

project(PVB
    VERSION 0.0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

option(PVB_FETCH_DEPS "Download third-party GUI dependencies with FetchContent" ON)
if(PVB_FETCH_DEPS)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.9b
    )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw imgui)
else()
    find_package(glfw3 CONFIG REQUIRED)
    find_path(IMGUI_SOURCE_DIR imgui.h REQUIRED)
endif()

if(NOT DEFINED imgui_SOURCE_DIR)
    set(imgui_SOURCE_DIR "${IMGUI_SOURCE_DIR}")
endif()

find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
)

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_library(imgui_color_text_edit STATIC
    third_party/ImGuiColorTextEdit/TextEditor.cpp
)
target_include_directories(imgui_color_text_edit PUBLIC
    third_party/ImGuiColorTextEdit
)
target_link_libraries(imgui_color_text_edit PUBLIC imgui)

add_executable(pvb
    src/main.cpp
    src/ui.cpp
    src/build.cpp
    src/app.cpp
    src/log.cpp
    src/codegen.cpp
)

target_include_directories(pvb PRIVATE src)
target_link_libraries(pvb PRIVATE imgui imgui_color_text_edit)

if(MSVC)
    target_compile_options(pvb PRIVATE /W4 /permissive-)
else()
    target_compile_options(pvb PRIVATE -Wall -Wextra)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(pvb PRIVATE /Zi /Od)
    else()
        target_compile_options(pvb PRIVATE -g -O0)
    endif()
endif()
