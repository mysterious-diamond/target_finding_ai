cmake_minimum_required(VERSION 3.10)
project(digit-draw)

set(CMAKE_CXX_STANDARD 14)

add_executable(digit-draw
    src/main.cpp
)

target_include_directories(digit-draw PRIVATE
    C:/raylib/raylib/src
)

target_link_libraries(digit-draw PRIVATE
    C:/raylib/raylib/src/libraylib.a
    opengl32
    gdi32
    winmm
)
