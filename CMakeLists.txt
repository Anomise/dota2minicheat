cmake_minimum_required(VERSION 3.20)
project(dota2minicheat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB IMGUI_SOURCES
    "external/imgui/*.cpp"
    "external/imgui/backends/imgui_impl_dx11.cpp"
    "external/imgui/backends/imgui_impl_win32.cpp"
)

add_subdirectory(external/minhook)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "src"
    "external/imgui"
    "external/imgui/backends"
    "external/minhook/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    minhook
    d3d11
    dxgi
    d3dcompiler
    Psapi
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

# Injector
add_executable(injector injector/main.cpp)
target_compile_definitions(injector PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
