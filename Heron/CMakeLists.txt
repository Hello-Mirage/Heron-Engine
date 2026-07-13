project(Heron)

# === GLAD ===
# We build glad directly into Heron so we can properly export its function pointers.

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
    "vendor/glad/src/gl.c"
    "vendor/stb_image/stb_image.cpp"
)
set_source_files_properties("vendor/glad/src/gl.c" PROPERTIES LANGUAGE CXX)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(Heron SHARED ${HERON_SOURCES} ${IMGUI_SOURCES})

target_compile_definitions(Heron PUBLIC HRN_PLATFORM_WINDOWS GLAD_API_CALL_EXPORT)
target_compile_definitions(Heron PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD GLAD_API_CALL_EXPORT_BUILD)
target_compile_options(Heron PUBLIC /utf-8)

target_include_directories(Heron
    PUBLIC
        src
        vendor/spdlog/include
        vendor/glfw/include
        vendor/entt/src
        vendor/imgui
        vendor/glad/include
        vendor/stb_image
)

# === GLFW ===
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

target_link_libraries(Heron PUBLIC box3d glfw glm::glm yaml-cpp)
target_link_libraries(Heron PUBLIC opengl32 gdi32 user32 shell32)

# Precompiled Header
target_precompile_headers(Heron PRIVATE src/hrnpch.h)
