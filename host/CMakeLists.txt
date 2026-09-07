cmake_minimum_required(VERSION 3.20)
project(BitStreamer-S3 CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIBS_DIR "D:/Working/C++/Libraries")

# 1. Add the submodule CMake target
add_subdirectory(external/dxgi-capture)

# 2. Add your executable (canvas.cpp is now managed inside dxgi_canvas!)
add_executable(BitStreamer-S3 
    src/main.cpp
    src/impl.cpp
)

target_include_directories(BitStreamer-S3 PRIVATE 
    ${LIBS_DIR}/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

# 3. Link against the dxgi_canvas library target
target_link_libraries(BitStreamer-S3 PRIVATE
    dxgi_capture
)