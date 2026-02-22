cmake_minimum_required(VERSION 3.10)
project(crt_overlay)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XFIXES REQUIRED xfixes)

add_executable(crt_overlay 
    src/main.cpp
    src/shader/Shader.cpp
    src/window/OverlayWindow.cpp
)

target_include_directories(crt_overlay PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
    ${XFIXES_INCLUDE_DIRS}
)

target_link_libraries(crt_overlay PRIVATE
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${X11_LIBRARIES}
    ${XFIXES_LIBRARIES}
)
