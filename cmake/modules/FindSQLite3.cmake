# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindSQLite3
-----------

Find the SQLite libraries, v3

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``SQLite::SQLite3``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``SQLite3_INCLUDE_DIRS``
  where to find sqlite3.h, etc.
``SQLite3_LIBRARIES``
  the libraries to link against to use SQLite3.
``SQLite3_VERSION``
  version of the SQLite3 library found
``SQLite3_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(SQLite3_INCLUDE_DIR NAMES sqlite3.h)
mark_as_advanced(SQLite3_INCLUDE_DIR)

# Look for the necessary library
if(STATIC_SQLITE3)
    find_library(SQLite3_LIBRARY NAMES
        ${CMAKE_STATIC_LIBRARY_PREFIX}sqlite3${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_STATIC_LIBRARY_PREFIX}sqlite${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
    find_library(SQLite3_LIBRARY NAMES sqlite3 sqlite)
endif()
mark_as_advanced(SQLite3_LIBRARY)

# Additionally find dependencies for static linking
if(STATIC_SQLITE3)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    if(NOT LIBDL_FOUND)
        find_library(LIBDL dl)
        if(LIBDL)
            message("-- Found libdl: ${LIBDL}")
            set(LIBDL_FOUND TRUE)
        else()
            message(FATAL_ERROR "libdl not found, which is required by FUSE when statically linked")
        endif()
    endif()
endif()

# Extract version information from the header file
if(SQLite3_INCLUDE_DIR)
    file(STRINGS ${SQLite3_INCLUDE_DIR}/sqlite3.h _ver_line
         REGEX "^#define SQLITE_VERSION  *\"[0-9]+\\.[0-9]+\\.[0-9]+\""
         LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+"
           SQLite3_VERSION "${_ver_line}")
    unset(_ver_line)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
    REQUIRED_VARS SQLite3_INCLUDE_DIR SQLite3_LIBRARY
    VERSION_VAR SQLite3_VERSION)

# Create the imported target
if(SQLite3_FOUND)
    set(SQLite3_INCLUDE_DIRS ${SQLite3_INCLUDE_DIR})
    set(SQLite3_LIBRARIES ${SQLite3_LIBRARY})
    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 UNKNOWN IMPORTED)

        if(STATIC_SQLITE3)
          set_target_properties(SQLite::SQLite3 PROPERTIES
              IMPORTED_LOCATION             "${SQLite3_LIBRARY}"
              INTERFACE_LINK_LIBRARIES      "${CMAKE_THREAD_LIBS_INIT} ${LIBDL}"
              INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}")
        else()
          set_target_properties(SQLite::SQLite3 PROPERTIES
              IMPORTED_LOCATION             "${SQLite3_LIBRARY}"
              INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}")
        endif()
    endif()
endif()
