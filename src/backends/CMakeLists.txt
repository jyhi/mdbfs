set(
  SRCS
  dispatcher.c
)

# Add the common include directory for header searching
include_directories(include)

# Sub-directories (backends) to be built
add_subdirectory(sqlite)

# Target
add_library(mdbfs-dispatcher STATIC ${SRCS})
set_property(TARGET mdbfs-dispatcher PROPERTY POSITION_INDEPENDENT_CODE ON)

# Link with backends
target_link_libraries(
  mdbfs-dispatcher
  PRIVATE mdbfs-sqlite
)

# Link with FUSE
target_compile_definitions(mdbfs-dispatcher PRIVATE ${FUSE_DEFINITIONS})
target_include_directories(mdbfs-dispatcher PRIVATE ${FUSE_INCLUDE_DIRS})
target_link_libraries(mdbfs-dispatcher PRIVATE FUSE::FUSE)
