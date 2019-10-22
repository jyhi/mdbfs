# FindBerkeleyDB.cmake: Find Package Berkeley DB ("DB")
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
# - BerkeleyDB::BerkeleyDB
#
# The following variables will also be defined for future use:
#
# - BERKELEY_DB_FOUND: whether Berkeley DB is found
# - BERKELEY_DB_INCLUDE_DIRS: where db.h is placed
# - BERKELEY_DB_LIBRARY: path to the found libdb.so
# - BERKELEY_DB_VERSION: full version string of Berkeley DB
# - BERKELEY_DB_MAJOR_VERSION: major version number of Berkeley DB
# - BERKELEY_DB_MINOR_VERSION: minor version number of Berkeley DB

# Find include path (-I)
find_path(BERKELEY_DB_INCLUDE_DIRS "db.h")

# Find library (-l)
if(STATIC_BERKELEY_DB)
  find_library(
    BERKELEY_DB_LIBRARY
    "${CMAKE_STATIC_LIBRARY_PREFIX}db${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
else()
  find_library(BERKELEY_DB_LIBRARY "db")
endif()

# Additionally find dependencies for static linking
if(STATIC_BERKELEY_DB)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  if(NOT LIBDL_FOUND)
    find_library(LIBDL dl)
    if(LIBDL)
      message("-- Found libdl: ${LIBDL}")
      set(LIBDL_FOUND TRUE)
    else()
      message(FATAL_ERROR "libdl not found, which is required by Berkeley DB when statically linked")
    endif()
  endif()

  find_package(OpenSSL REQUIRED)
endif()

# Extract FUSE version from the C header for comparison
if(BERKELEY_DB_INCLUDE_DIRS)
  if(EXISTS "${BERKELEY_DB_INCLUDE_DIRS}/db.h")
    file(READ "${BERKELEY_DB_INCLUDE_DIRS}/db.h" DB_H)
  else()
    message(FATAL_MESSAGE "Cannot find db.h for version checking.")
  endif()

  # The regular expressions are borrowed from
  # https://gitlab.com/sum01/FindBerkeleyDB/blob/master/FindBerkeleyDB.cmake
  string(REGEX REPLACE ".*DB_VERSION_MAJOR	*([0-9]+).*" "\\1" BERKELEY_DB_MAJOR_VERSION "${DB_H}")
  string(REGEX REPLACE ".*DB_VERSION_MINOR	*([0-9]+).*" "\\1" BERKELEY_DB_MINOR_VERSION "${DB_H}")
  string(REGEX REPLACE ".*DB_VERSION_PATCH	*([0-9]+(NC)?).*" "\\1" BERKELEY_DB_PATCH_VERSION "${DB_H}")

  set(BERKELEY_DB_VERSION "${BERKELEY_DB_MAJOR_VERSION}.${BERKELEY_DB_MINOR_VERSION}.${BERKELEY_DB_PATCH_VERSION}")
endif()

# Handle CMake find_package() arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  "BerkeleyDB"
  REQUIRED_VARS BERKELEY_DB_LIBRARY BERKELEY_DB_INCLUDE_DIRS
  VERSION_VAR BERKELEY_DB_VERSION
)

# Add imported target for easier future use
add_library(BerkeleyDB::BerkeleyDB UNKNOWN IMPORTED)
if(STATIC_FUSE)
  set_target_properties(
    BerkeleyDB::BerkeleyDB
    PROPERTIES
    IMPORTED_LOCATION "${BERKELEY_DB_LIBRARY}"
    INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT} ${LIBDL} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${BERKELEY_DB_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS "${BERKELEY_DB_DEFINITIONS}"
  )
else()
  set_target_properties(
    BerkeleyDB::BerkeleyDB
    PROPERTIES
    IMPORTED_LOCATION "${BERKELEY_DB_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${BERKELEY_DB_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS "${BERKELEY_DB_DEFINITIONS}"
  )
endif()

# Unset global variables
unset(DB_H)
