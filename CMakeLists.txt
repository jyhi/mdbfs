cmake_minimum_required(VERSION 3.15)
project(mdbfs
        VERSION 0.1.0
        DESCRIPTION "Mapping DataBases into a File System"
        HOMEPAGE_URL "https://github.com/lmy441900"
        LANGUAGES C)

# Project options for customized build
option(BUILD_DOCUMENTATION "Enable API documetation build using Doxygen" ON)

# Find dependencies
if(BUILD_DOCUMENTATION)
  find_package(Doxygen)

  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is required to build API documentation")
  endif(NOT DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

# Build sub-directories
add_subdirectory(src)
