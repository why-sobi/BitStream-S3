cmake_minimum_required(VERSION 3.20)
project(BitStream_S3_Root)

# Set up shared includes globally if needed
include_directories(shared/include)

# Add host application build
add_subdirectory(host)