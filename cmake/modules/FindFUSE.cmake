# FindFUSE3.cmake: Find Package LibFUSE 2 / 3
# Copyright (c) 2019 Junde Yhi <lmy441900@aosc.xyz>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.0)

# This module defines the following IMPORTED target:
#
# - FUSE::FUSE
#
# The following variables will also be defined for future use:
#
# - FUSE_FOUND: whether FUSE is found
# - FUSE_INCLUDE_DIR: where fuse.h is placed
# - FUSE_LIBRARY: path to the found libfuse(|3).so
# - FUSE_DEFINITIONS: additional preprocessor definitions by FUSE
# - FUSE_VERSION: full version string of FUSE
# - FUSE_MAJOR_VERSION: major version number of FUSE
# - FUSE_MINOR_VERSION: minor version number of FUSE

# Since, compared with FUSE 2, FUSE 3 is a breaking change, file names FUSE 3
# are appended with "3" to allow co-existance of the two versions. Different
# search pattern is required to be prepared before searching for the correct
# library.
if(FUSE_FIND_VERSION_MAJOR LESS_EQUAL 2)
  set(FUSE_NAME "fuse")
elseif(FUSE_FIND_VERSION_MAJOR EQUAL 3)
  set(FUSE_NAME "fuse3")
else()
  message(FATAL_MESSAGE "FUSE with version larger than 3 is not supported.")
endif()

# Find include path (-I)
find_path(
    FUSE_INCLUDE_DIR
    "fuse.h"
    PATH_SUFFIXES ${FUSE_NAME}
)

# Find library (-l)
find_library(
    FUSE_LIBRARY
    ${FUSE_NAME}
)

# FUSE 2 additionally adds -D_FILE_OFFSET_BITS=64 to CFLAGS, but not in 3.
if(FUSE_FINDVERSION_MAJOR LESS_EQUAL 2)
  set(FUSE_DEFINITIONS "-D_FILE_OFFSET_BITS=64")
endif()

# Extract FUSE version from the C header for comparison
# Derived from the one in iovisor/bcc-fuse:
# https://github.com/iovisor/bcc-fuse/blob/master/cmake/Modules/Findfuse.cmake#L98-L101
if(FUSE_INCLUDE_DIR)
  if(EXISTS "${FUSE_INCLUDE_DIR}/fuse_common.h")
    file(READ "${FUSE_INCLUDE_DIR}/fuse_common.h" FUSE_COMMON_H)
  elseif(EXISTS "${FUSE_INCLUDE_DIR}/fuse/fuse_common.h")
    file(READ "${FUSE_INCLUDE_DIR}/fuse/fuse_common.h" FUSE_COMMON_H)
  else()
    message(FATAL_MESSAGE "Cannot find fuse_common.h for version checking.")
  endif()

  string(REGEX REPLACE ".*# *define *FUSE_MAJOR_VERSION *([0-9]+).*" "\\1" FUSE_MAJOR_VERSION "${FUSE_COMMON_H}")
  string(REGEX REPLACE ".*# *define *FUSE_MINOR_VERSION *([0-9]+).*" "\\1" FUSE_MINOR_VERSION "${FUSE_COMMON_H}")

  set(FUSE_VERSION "${FUSE_MAJOR_VERSION}.${FUSE_MINOR_VERSION}")
endif()

# Handle CMake find_package() arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    "FUSE"
    REQUIRED_VARS FUSE_INCLUDE_DIR FUSE_LIBRARY
    VERSION_VAR FUSE_VERSION
)

add_library(FUSE::FUSE SHARED IMPORTED)
set_property(TARGET FUSE::FUSE PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${FUSE_INCLUDE_DIR})
set_property(TARGET FUSE::FUSE PROPERTY IMPORTED_CONFIGURATIONS ${FUSE_CONFIGURATIONS})
set_property(TARGET FUSE::FUSE PROPERTY IMPORTED_LOCATION ${FUSE_LIBRARY})

# Unset global variables
unset(FUSE_COMMON_H)
unset(FUSE_NAME)
