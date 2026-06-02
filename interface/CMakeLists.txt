cmake_minimum_required(VERSION 3.10)
project(target-find)

set(CMAKE_CXX_STANDARD 14)

add_executable(target-find
    src/main.cpp
)

target_include_directories(target-find PRIVATE
    C:/raylib/raylib/src
)

target_link_libraries(target-find PRIVATE
    C:/raylib/raylib/src/libraylib.a
    opengl32
    gdi32
    winmm
)
