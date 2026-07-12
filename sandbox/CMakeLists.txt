project(sandbox)

add_executable(sandbox src/SandboxApp.cpp)

target_link_libraries(sandbox PRIVATE Heron)
