project(Heron)

# === GLAD ===
add_library(glad STATIC vendor/glad/src/gl.c)
target_include_directories(glad PUBLIC vendor/glad/include)

# === Box3D (Real) ===
set(BOX3D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX3D_TESTS OFF CACHE BOOL "" FORCE)
set(BOX3D_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/box3d)

# === GLM ===
add_subdirectory(vendor/glm)

# === YAML-CPP ===
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml-cpp tools" FORCE)
add_subdirectory(vendor/yaml-cpp)

# === Heron Engine ===
file(GLOB_RECURSE HERON_SOURCES "src/*.cpp" "src/*.h")
file(GLOB IMGUI_SOURCES 
    "vendor/imgui/*.cpp" 
)
list(APPEND IMGUI_SOURCES
    "vendor/imgui/backends/imgui_impl_glfw.cpp"
    "vendor/imgui/backends/imgui_impl_opengl3.cpp"
)

add_library(Heron STATIC ${HERON_SOURCES} ${IMGUI_SOURCES})

target_compile_definitions(Heron PUBLIC HRN_PLATFORM_WINDOWS)
target_compile_definitions(Heron PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_compile_options(Heron PUBLIC /utf-8)

target_include_directories(Heron
    PUBLIC
        src
        vendor/spdlog/include
        vendor/glfw/include
        vendor/entt/src
        vendor/imgui
)

# === GLFW ===
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

target_link_libraries(Heron PUBLIC glad box3d glfw glm::glm yaml-cpp)
target_link_libraries(Heron PUBLIC opengl32 gdi32 user32 shell32)

# Precompiled Header
target_precompile_headers(Heron PRIVATE src/hrnpch.h)
