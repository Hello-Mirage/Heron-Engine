
cmake_minimum_required(VERSION 3.20)
project(HeronEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)

add_subdirectory(Heron)
add_subdirectory(HeronEditor)
add_subdirectory(GameProject)
add_subdirectory(sandbox)

