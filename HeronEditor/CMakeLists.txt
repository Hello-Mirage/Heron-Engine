project(HeronEditor)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets REQUIRED)

add_executable(HeronEditor
    src/main.cpp
    src/EditorWindow.h
    src/EditorWindow.cpp
    src/ViewportWidget.h
    src/ViewportWidget.cpp
    src/SceneHierarchyPanel.h
    src/SceneHierarchyPanel.cpp
    src/PropertiesPanel.h
    src/PropertiesPanel.cpp
)

target_link_libraries(HeronEditor PRIVATE
    Heron
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

target_compile_options(HeronEditor PRIVATE /Zc:__cplusplus /utf-8)
target_include_directories(HeronEditor PRIVATE ../Heron/vendor/glad/include)
