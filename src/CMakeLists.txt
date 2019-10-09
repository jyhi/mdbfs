# Dependencies
find_package(FUSE 3.2 REQUIRED)

# Source code to be built
set(
  SRCS
  backend.c
  main.c
)

# Configuration header to generate
configure_file(mdbfs-config.h.in mdbfs-config.h @ONLY)

# Include this directory (src and target) for project-wide use
include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Sub-targets
add_subdirectory(utils)
add_subdirectory(backends)

# Main executable target
add_executable(mdbfs ${SRCS})
install(TARGETS mdbfs)

# Link against targets
target_link_libraries(
  mdbfs
  PRIVATE mdbfs-backends
  PRIVATE mdbfs-utils
)

# Link against FUSE
target_compile_definitions(mdbfs PRIVATE ${FUSE_DEFINITIONS})
target_include_directories(mdbfs PRIVATE ${FUSE_INCLUDE_DIRS})
target_link_libraries(mdbfs PRIVATE FUSE::FUSE)

# API Documentation
if(BUILD_DOCUMENTATION)
  find_package(Doxygen)

  set(DOXYGEN_JAVADOC_AUTOBRIEF true)
  set(DOXYGEN_QT_AUTOBRIEF true)
  set(DOXYGEN_TAB_SIZE "2")
  set(DOXYGEN_VERBATIM_HEADERS false)

  set(DOXYGEN_GENERATE_HTML YES)

  doxygen_add_docs(
    docs
    COMMENT "Build API documentation using Doxygen"
  )
endif()
