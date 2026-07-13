project(GameProject)

add_library(GameProject SHARED src/Game.cpp)
set_target_properties(GameProject PROPERTIES PREFIX "")

target_link_libraries(GameProject PRIVATE Heron)
