cmake_minimum_required(VERSION 3.15)
project(mdbfs
        VERSION 0.1.0
        DESCRIPTION "Mapping DataBases into a File System"
        HOMEPAGE_URL "https://github.com/lmy441900/mdbfs"
        LANGUAGES C CXX)

# Project-wide configurations
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Custom CMake modules for package finding
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Project options for customized build
option(BUILD_DOCUMENTATION "Enable API documetation build using Doxygen" OFF)

# Build sub-directories
add_subdirectory(src)
