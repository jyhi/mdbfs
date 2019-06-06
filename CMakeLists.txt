cmake_minimum_required(VERSION 3.15)
project(mdbfs
        VERSION 0.1.0
        DESCRIPTION "Mapping DataBases into a File System"
        HOMEPAGE_URL "https://github.com/lmy441900"
        LANGUAGES C)

# Custom CMake modules for package finding
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Project options for customized build
option(BUILD_DOCUMENTATION "Enable API documetation build using Doxygen" ON)

# Find dependencies
find_package(FUSE 3.2 REQUIRED)

if(BUILD_DOCUMENTATION)
  find_package(Doxygen)
endif()

# Build sub-directories
add_subdirectory(src)
