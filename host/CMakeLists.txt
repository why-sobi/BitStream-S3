cmake_minimum_required(VERSION 3.20)
project(BitStreamer-S3-Host CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 1. Disable shared libs & extra build tools so it builds fast and static
set(ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_EXECUTABLES OFF CACHE BOOL "" FORCE)

set(LIBS_DIR "D:/Working/C++/Libraries")

add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(ASIO_STANDALONE)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

# 1. Add the submodule CMake target
add_subdirectory(external/dxgi-capture)
# 2. Add your executable
add_executable(BitStreamer-S3-Host 
    src/main.cpp
    src/impl.cpp
    src/host.cpp
    src/network.cpp
)

target_compile_definitions(BitStreamer-S3-Host PRIVATE BITSTREAM_BUILD_HOST)

target_include_directories(BitStreamer-S3-Host PRIVATE 
    ${LIBS_DIR}/stb
    ${LIBS_DIR}/asio/include
    ${LIBS_DIR}/libjpeg-turbo/src
    ${LIBS_DIR}/libjpeg-turbo/build
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)


# Windows requires linking Winsock2 explicitly
if(WIN32)
    target_link_libraries(BitStreamer-S3-Host PRIVATE ws2_32 mswsock)
endif()

if(EXISTS "${LIBS_DIR}/libjpeg-turbo/build/Release/turbojpeg-static.lib")
    set(TURBOJPEG_LIB "${LIBS_DIR}/libjpeg-turbo/build/Release/turbojpeg-static.lib")
else()
    set(TURBOJPEG_LIB "${LIBS_DIR}/libjpeg-turbo/build/turbojpeg-static.lib")
endif()

target_link_libraries(BitStreamer-S3-Host PRIVATE
    dxgi_capture
    "${TURBOJPEG_LIB}"
)